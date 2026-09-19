//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <thread>

#include "Constants.h"
#include "Gameplay/Stage.h"
#include "Graphics/Text.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "Net/Session.h"
#include "Util/CrashLog.h"
#include "Util/HardwareInfo.h"
#include "Util/ScreenResolution.h"

#ifdef USE_NX
#include "Util/NxFiles.h"
#else
#include "Util/WzFiles.h"
#endif

// macOS (not iOS) only: everything below is compiled away on Windows, Linux
// and the iOS port, which keep their existing behaviour byte for byte.
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#define MS_MACOS_DATA_RESOLVER 1

#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>

#include <sys/stat.h>
#include <unistd.h>

#include <climits>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#endif
#endif

namespace ms
{
#ifdef MS_MACOS_DATA_RESOLVER
	// Filled in when the data directory could not be resolved, so the startup
	// error can name every path that was tried instead of just "Missing a game
	// file: Base.nx".
	std::string startup_diagnostic;

	namespace
	{
		// A directory qualifies if it holds the first file NxFiles wants.
		bool is_data_directory(const std::string& dir)
		{
			if (dir.empty())
				return false;

			struct stat st;

			return stat((dir + "/Base.nx").c_str(), &st) == 0 && S_ISREG(st.st_mode);
		}

		std::string parent_of(const std::string& path)
		{
			size_t slash = path.find_last_of('/');

			if (slash == std::string::npos)
				return "";

			return slash == 0 ? "/" : path.substr(0, slash);
		}

		// Directory holding the running executable, resolved through symlinks.
		std::string executable_directory()
		{
			char raw[PATH_MAX];
			uint32_t size = sizeof(raw);

			if (_NSGetExecutablePath(raw, &size) != 0)
				return "";

			char resolved[PATH_MAX];

			return parent_of(realpath(raw, resolved) != nullptr ? resolved : raw);
		}

		std::string current_directory()
		{
			char buffer[PATH_MAX];

			return getcwd(buffer, sizeof(buffer)) != nullptr ? std::string(buffer) : std::string();
		}

		// First non-empty line of a text file, trimmed.
		std::string read_path_file(const std::string& path)
		{
			std::ifstream file(path);
			std::string line;

			while (std::getline(file, line))
			{
				size_t end = line.find_last_not_of(" \t\r\n");

				if (end != std::string::npos)
					return line.substr(0, end + 1);
			}

			return "";
		}
	}

	// The client resolves every runtime path against the working directory -
	// the .nx assets, the "Settings" file, "buddymemo.txt", "screenshots/" and
	// the crash log. From a terminal the developer supplies that by cd'ing into
	// wz/, but a double-clicked .app is started in "/" instead and there is no
	// Info.plist key for "start me in this folder". So pick the directory here
	// and move into it before anything else touches the filesystem.
	//
	// Search order (first directory containing Base.nx wins) - keep this in
	// step with resource/macos/README.md:
	//   1. $OPENSTORY_DATA_DIR
	//   2. the current working directory  (every existing terminal workflow
	//      keeps working unchanged: if the cwd is already right, this is a
	//      no-op and nothing below is even considered)
	//   3. the directory holding the executable
	//   -- inside an .app bundle only:
	//   4. the path written in Contents/Resources/DataDirectory
	//   5. Contents/Resources/data
	//   6. ~/Library/Application Support/OpenStory
	//   7. the folder containing the .app
	void chdir_to_data_directory()
	{
		std::vector<std::string> candidates;

		if (const char* env = std::getenv("OPENSTORY_DATA_DIR"))
			if (env[0] != '\0')
				candidates.emplace_back(env);

		candidates.push_back(current_directory());

		const std::string exe_dir = executable_directory();

		if (!exe_dir.empty())
		{
			candidates.push_back(exe_dir);

			// .../OpenStory.app/Contents/MacOS -> .../OpenStory.app/Contents
			const std::string contents = parent_of(exe_dir);
			const bool bundled = exe_dir.size() > 15
				&& exe_dir.compare(exe_dir.size() - 15, 15, "/Contents/MacOS") == 0;

			if (bundled)
			{
				const std::string recorded = read_path_file(contents + "/Resources/DataDirectory");

				if (!recorded.empty())
					candidates.push_back(recorded);

				candidates.push_back(contents + "/Resources/data");
			}

			if (const char* home = std::getenv("HOME"))
				if (home[0] != '\0')
					candidates.push_back(std::string(home) + "/Library/Application Support/OpenStory");

			// .../OpenStory.app/Contents -> the folder holding OpenStory.app
			if (bundled)
				candidates.push_back(parent_of(parent_of(contents)));
		}

		for (const std::string& candidate : candidates)
		{
			if (!is_data_directory(candidate))
				continue;

			// Canonical path: the client writes back into this directory, and
			// relative paths (screenshots/, crashlog.txt) should be stable.
			char resolved[PATH_MAX];
			const std::string target = realpath(candidate.c_str(), resolved) != nullptr
				? std::string(resolved) : candidate;

			if (chdir(target.c_str()) == 0)
				return;
		}

		// Nothing found. Leave the working directory alone - NxFiles::init()
		// reports the missing file - but record where we looked so the failure
		// is not a mystery.
		startup_diagnostic = "No game data found. Looked in:\n";

		for (const std::string& candidate : candidates)
			startup_diagnostic += "  - " + candidate + "\n";

		startup_diagnostic += "\nSet OPENSTORY_DATA_DIR to the folder holding Base.nx"
			" and the other .nx files.";
	}

	// Startup errors have nowhere to go when the app is launched from Finder:
	// there is no console, and the retry prompt below reads std::cin, which is
	// immediately at EOF. Put the message on screen instead.
	void show_startup_alert(const std::string& text)
	{
		CFStringRef message = CFStringCreateWithCString(nullptr, text.c_str(), kCFStringEncodingUTF8);

		if (message == nullptr)
			return;

		// Bounded timeout rather than 0 (= wait forever): without a window
		// server to draw the alert this must not hang.
		CFUserNotificationDisplayAlert(120.0, kCFUserNotificationStopAlertLevel,
			nullptr, nullptr, nullptr, CFSTR("OpenStory could not start"), message,
			CFSTR("Quit"), nullptr, nullptr, nullptr);

		CFRelease(message);
	}
#endif

	Error init()
	{
		std::cout << "[Init] Connecting to server..." << std::endl;

		if (Error error = Session::get().init())
			return error;

		std::cout << "[Init] Loading game files..." << std::endl;

#ifdef USE_NX
		if (Error error = NxFiles::init())
			return error;
#else
		if (Error error = WzFiles::init())
			return error;
#endif

		std::cout << "[Init] Creating window..." << std::endl;

		if (Error error = Window::get().init())
			return error;

		std::cout << "[Init] Initializing audio..." << std::endl;

		if (Error error = Sound::init())
			return error;

		if (Error error = Music::init())
			return error;

		std::cout << "[Init] Loading game data..." << std::endl;

		Char::init();
		DamageNumber::init();
		MapPortals::init();
		Stage::get().init();
		UI::get().init();

		std::cout << "[Init] Ready." << std::endl;

		return Error::NONE;
	}

	void update()
	{
		Window::get().check_events();
		Window::get().update();
		Stage::get().update();
		UI::get().update();
		Session::get().read();
	}

	// Current frames-per-second, recomputed a few times a second in loop().
	int g_fps = 0;

	void draw(float alpha)
	{
		Window::get().begin();
		Stage::get().draw(alpha);
		UI::get().draw(alpha);

		// On-screen FPS counter (top-right, yellow).
		if (Configuration::get().get_show_fps())
		{
			static Text fpslabel(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::YELLOW);
			fpslabel.change_text("FPS " + std::to_string(g_fps));

			int16_t sw = static_cast<int16_t>(Constants::Constants::get().get_viewwidth());
			fpslabel.draw(DrawArgument(Point<int16_t>(static_cast<int16_t>(sw - 12), 6)));
		}

		Window::get().end();
	}

	bool running()
	{
		return Session::get().is_connected()
			&& UI::get().not_quitted()
			&& Window::get().not_closed();
	}

	void loop()
	{
		Timer::get().start();

		int64_t timestep = Constants::TIMESTEP * 1000;
		int64_t accumulator = timestep;

		// FPS counter accumulators.
		int64_t fps_accum = 0;
		int32_t fps_frames = 0;

		// Frame-rate cap from settings (FPSCap). 0 = uncapped.
		uint8_t fps_cap = Setting<FPSCap>::get().load();
		int64_t FRAME_CAP_US = fps_cap > 0 ? 1000000 / fps_cap : 0;

		while (running())
		{
			auto frame_start = std::chrono::high_resolution_clock::now();

			int64_t elapsed = Timer::get().stop();

			// Clamp the accumulated time so a single long/stalled frame can't
			// trigger a huge catch-up burst of update() steps. Without this,
			// any hitch fast-forwards physics many steps at once and controlled
			// mobs visibly "teleport" forward (and get reported to the server
			// at the jumped-to position, desyncing every other client).
			accumulator += elapsed;

			const int64_t MAX_ACCUM = timestep * 5;

			if (accumulator > MAX_ACCUM)
				accumulator = MAX_ACCUM;

			// Update game with constant timestep as many times as possible.
			for (; accumulator >= timestep; accumulator -= timestep)
				update();

			// Draw the game. Interpolate to account for remaining time.
			float alpha = static_cast<float>(accumulator) / timestep;
			draw(alpha);

			// Recompute the on-screen FPS ~4 times per second.
			fps_accum += elapsed;
			fps_frames++;

			if (fps_accum >= 250000)
			{
				g_fps = static_cast<int>(fps_frames * 1000000LL / fps_accum);
				fps_frames = 0;
				fps_accum = 0;
			}

			// Cap framerate to the configured FPS (skip entirely if uncapped).
			if (FRAME_CAP_US > 0)
			{
				auto frame_end = std::chrono::high_resolution_clock::now();
				auto frame_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();

				if (frame_us < FRAME_CAP_US)
					std::this_thread::sleep_for(std::chrono::microseconds(FRAME_CAP_US - frame_us));
			}
		}

		Sound::close();
	}

	void start()
	{
		// Initialize and check for errors
		if (Error error = init())
		{
			std::cerr << "[Error] " << error.get_message() << error.get_args() << std::endl;

#ifdef MS_MACOS_DATA_RESOLVER
			if (!startup_diagnostic.empty())
				std::cerr << startup_diagnostic << std::endl;

			// No console to prompt on (launched from Finder, or stdin piped):
			// offering an interactive retry would just read EOF and exit
			// without ever telling the user what went wrong.
			if (!isatty(STDIN_FILENO))
			{
				std::string message = std::string(error.get_message()) + error.get_args();

				if (!startup_diagnostic.empty())
					message += "\n\n" + startup_diagnostic;

				show_startup_alert(message);

				return;
			}
#endif

			bool can_retry = error.can_retry();

			if (can_retry)
			{
				std::cout << "Type 'retry' to try again: ";
				std::string command;
				std::cin >> command;

				if (command == "retry")
					start();
			}
			else
			{
				std::cout << "Press Enter to exit...";
				std::cin.get();
			}
		}
		else
		{
			loop();
		}
	}
}

int main()
{
#ifdef MS_MACOS_DATA_RESOLVER
	// Has to be the very first thing that runs: Configuration is a lazily
	// constructed singleton that reads "Settings" out of the working
	// directory, and install_crash_logger() below writes crashlog.txt there.
	ms::chdir_to_data_directory();
#endif

	ms::install_crash_logger();
	ms::HardwareInfo();
	ms::ScreenResolution();
	ms::start();

	return 0;
}
