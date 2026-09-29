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
#include <cstdint>

#ifdef PLATFORM_PS5
extern "C" int sceKernelUsleep(unsigned int);
#endif

#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_phase(const char*, std::uintptr_t);
static void startup_phase(const char* label, std::uintptr_t value = 0) { openstory_diagnostics_phase(label, value); }
#else
static void startup_phase(const char*, std::uintptr_t = 0) {}
#endif

#include "Configuration.h"
#include "Constants.h"
#include "Gameplay/Stage.h"
#include "Graphics/Text.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "Net/Session.h"
#include "Util/CrashLog.h"
#include "Util/Paths.h"
#ifndef OPENSTORY_SDL
#include "Util/HardwareInfo.h"
#include "Util/ScreenResolution.h"
#else
#include <filesystem>
#include <cstring>
#include <SDL.h>
#include "../platform/sdl/ConsoleMenu.h"
extern bool check_sdl_input();
extern bool check_sdl_audio();
extern bool init_sdl_identity();
#endif

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

	static void frame_sleep(int64_t microseconds)
	{
		openstory_diagnostics_checkpoint("frame-sleep");
#ifdef PLATFORM_PS5
		sceKernelUsleep(static_cast<unsigned int>(microseconds));
#else
		std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
#endif
	}

#ifdef PLATFORM_PS5
	static void connection_status(const Text& status)
	{
		Window::get().begin();
		status.draw(DrawArgument(Point<int16_t>(30, 60)));
		Window::get().end();
	}
#endif

	Error init(bool offline = false)
	{
		if (offline) startup_phase("offline-preview");
#ifndef OPENSTORY_SDL
		if (!offline) {
			std::cout << "[Init] Connecting to server..." << std::endl;
			if (Error error = Session::get().init()) return error;
		}
#endif
		startup_phase("game-files-load");
		std::cout << "[Init] Loading game files..." << std::endl;

#ifdef USE_NX
		if (Error error = NxFiles::init())
			return error;
#else
		if (Error error = WzFiles::init())
			return error;
#endif

		startup_phase("window-create");
		std::cout << "[Init] Creating window..." << std::endl;

		if (Error error = Window::get().init())
			return error;

#ifdef OPENSTORY_SDL
		if (!offline)
		{
#ifdef PLATFORM_PS5
			startup_phase("connection-status");
			Text status(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE, "Connecting to server...");
			connection_status(status);
#endif
			startup_phase("server-connect");
			std::cout << "[Init] Connecting to server..." << std::endl;
			if (Error error = Session::get().init()) {
				startup_phase("server-connect-failed");
#ifdef PLATFORM_PS5
				// Keep the error visible instead of returning to Home without an explanation.
				status.change_text("Cannot connect to server. Close OpenStory and check your server.");
				startup_phase("connection-error-text-ready");
				while (Window::get().not_closed() && UI::get().not_quitted()) {
					Window::get().check_events();
					connection_status(status);
					frame_sleep(16000);
				}
#endif
				return error;
			}
			startup_phase("server-connected");
		}
#endif

		startup_phase("sound-init");
		std::cout << "[Init] Initializing audio..." << std::endl;

		if (Error error = Sound::init())
			return error;

		startup_phase("music-init");
		if (Error error = Music::init())
			return error;

		std::cout << "[Init] Loading game data..." << std::endl;

		startup_phase("character-data");
		Char::init();
		startup_phase("damage-number-data");
		DamageNumber::init();
		startup_phase("portal-data");
		MapPortals::init();
		startup_phase("stage-init");
		Stage::get().init();
		startup_phase("ui-init");
		UI::get().init();
		startup_phase("game-ready");

		std::cout << "[Init] Ready." << std::endl;

		return Error::NONE;
	}

	void update(bool offline)
	{
		openstory_diagnostics_checkpoint("events");
		Window::get().check_events();
		openstory_diagnostics_checkpoint("window-update");
		Window::get().update();
		openstory_diagnostics_checkpoint("stage-update");
		Stage::get().update();
		openstory_diagnostics_checkpoint("ui-update");
		UI::get().update();
		openstory_diagnostics_checkpoint("network-update");
		if (!offline) Session::get().read();
	}

	// Current frames-per-second, recomputed a few times a second in loop().
	int g_fps = 0;

	void draw(float alpha, bool offline = false)
	{
		openstory_diagnostics_checkpoint("draw-begin");
		Window::get().begin();
		openstory_diagnostics_checkpoint("stage-draw");
		Stage::get().draw(alpha);
		openstory_diagnostics_checkpoint("ui-draw");
		UI::get().draw(alpha);
		openstory_diagnostics_checkpoint("overlay-draw");
		if (offline)
		{
			GraphicsGL::get().drawrectangle(8, 8, 390, 28, 0, 0, 0, 0.85f);
			static const Text label(Text::A13M, Text::LEFT, Color::Name::WHITE,
				"Offline preview - server not configured. Login disabled.");
			label.draw(DrawArgument(Point<int16_t>(14, 14)));
		}

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

	bool running(bool offline)
	{
		return (offline || Session::get().is_connected())
			&& UI::get().not_quitted()
			&& Window::get().not_closed();
	}

	void loop(bool offline)
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

		bool first_frame = true;
		unsigned frame = 0;
		startup_phase("game-loop");
		while (running(offline))
		{
			openstory_diagnostics_checkpoint("frame-clock", ++frame);
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
				update(offline);

			// Draw the game. Interpolate to account for remaining time.
			float alpha = static_cast<float>(accumulator) / timestep;
			draw(alpha, offline);
			openstory_diagnostics_checkpoint("frame-stats");
			if (first_frame) startup_phase("first-frame-presented");

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
					frame_sleep(FRAME_CAP_US - frame_us);
			}
			openstory_diagnostics_checkpoint("frame-complete");
			if (first_frame) startup_phase("first-frame-complete");
			first_frame = false;
		}

		startup_phase("loop-exit-offline", offline);
		startup_phase("loop-exit-ui-open", UI::get().not_quitted());
		startup_phase("loop-exit-window-open", Window::get().not_closed());
		startup_phase("loop-exit-frames", frame);
		std::cout << "[Exit] offline=" << offline
			<< " ui_open=" << UI::get().not_quitted()
			<< " window_open=" << Window::get().not_closed()
			<< " frames=" << frame << std::endl;
		openstory_diagnostics_checkpoint("audio-close");
		Sound::close();
	}

#ifdef OPENSTORY_SDL
	static bool select_server(bool& offline)
	{
		UI::get().remove_textfield();
		auto screen = UI::get().emplace<UIServerSelect>();
		startup_phase("server-selector-ready");
		while (Window::get().not_closed() && UI::get().not_quitted()) {
			Window::get().check_events();
			UI::get().update();
			const auto choice = screen->take_choice();
			if (choice == UIServerSelect::OFFLINE) {
				offline = true;
				break;
			}
			if (choice == UIServerSelect::CONNECT) {
				Setting<ServerIP>::get().save(screen->address());
				Setting<ServerPort>::get().save(screen->service());
				Setting<ServerConfigured>::get().save(true);
				Configuration::get().save();
				screen->message("Connecting... This can take up to 8 seconds.");
				draw(1, false);
				startup_phase("server-selector-connect");
				if (!Session::get().init()) { offline = false; break; }
				screen->message("Could not connect. Check the IP and port, retry, or choose offline.");
				startup_phase("server-selector-retry");
			}
			draw(1, false);
			frame_sleep(16000);
		}
		UI::get().remove(UIElement::SERVERSELECT);
		Setting<OfflinePreview>::get().save(offline);
		Configuration::get().save();
		return Window::get().not_closed() && UI::get().not_quitted();
	}
#endif

	int start(bool server_selector = false)
	{
#ifdef PLATFORM_PS5
		server_selector = true;
#endif
		bool offline = server_selector || Setting<OfflinePreview>::get().load();
		if (server_selector) Setting<OfflinePreview>::get().save(true);
		// Initialize and check for errors
		if (Error error = init(offline))
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

#ifndef OPENSTORY_SDL
			bool can_retry = error.can_retry();

			if (can_retry)
			{
				std::cout << "Type 'retry' to try again: ";
				std::string command;
				std::cin >> command;

				if (command == "retry")
					return start();
			}
			else
			{
				std::cout << "Press Enter to exit...";
				std::cin.get();
			}
#endif
			return 1;
		}
		else
		{
#ifdef OPENSTORY_SDL
			if (server_selector && !select_server(offline)) return 0;
#endif
			loop(offline);
		}
		return 0;
	}
}

int main(int argc, char** argv)
{
#ifdef MS_MACOS_DATA_RESOLVER
	// Has to be the very first thing that runs: install_crash_logger() below
	// writes crashlog.txt into the working directory.
	ms::chdir_to_data_directory();

	// Configuration is NOT lazily constructed. Singleton keeps `instance` as a
	// static data member (Template/Singleton.h), so it has static storage
	// duration and its constructor -- which calls load() and reads "Settings"
	// out of the working directory -- has already run before main() is
	// entered. The chdir above is therefore always too late for it, and
	// without this reload the client keeps the built-in defaults: launched
	// from anywhere but the data directory it would sit forever trying to
	// reach the default ServerIP. load() is documented as re-loadable.
	ms::Configuration::get().load();
#endif

	startup_phase("game-main-entry");
	ms::install_crash_logger();
#ifdef OPENSTORY_SDL
	const bool server_select = argc > 1 && std::strcmp(argv[1], "--server-select") == 0;
	const bool asset_check = argc > 1 && std::strcmp(argv[1], "--asset-check") == 0;
	if (argc > 1 && std::strcmp(argv[1], "--platform-check") == 0)
	{
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
		if (auto error = ms::Window::get().init()) {
			std::cerr << error.get_message() << '\n';
			return 1;
		}
		if (!check_sdl_input()) return 1;
		ms::Window::get().begin();
		ms::GraphicsGL::get().drawrectangle(0, 0, 64, 64, 1, 0, 0, 1);
		ms::Window::get().end();
		if (glGetError() != GL_NO_ERROR) return 1;
		std::cout << "SDL controller mapping and Core rendering passed\n";
		return 0;
	}
	startup_phase("game-arguments-ready");
	std::error_code error;
#ifndef PLATFORM_PS5
	const char* data_directory = (asset_check || server_select) ? (argc > 2 ? argv[2] : ".") : (argc > 1 ? argv[1] : ".");
	startup_phase("data-directory");
	std::filesystem::current_path(data_directory, error);
	if (error) {
		std::cerr << "Cannot open game data directory: " << data_directory << '\n';
		return 1;
	}
#endif
#ifdef PLATFORM_PS5
	startup_phase("settings-probe");
	const auto settings_path = ms::data_path("Settings");
	if (!std::filesystem::exists(settings_path) && std::filesystem::exists("/app0/Settings")) {
		startup_phase("settings-copy");
		std::filesystem::copy_file("/app0/Settings", settings_path, error);
		if (error) { std::cerr << "Cannot create Settings\n"; return 1; }
	}
#endif
	startup_phase("configuration-load");
	ms::Configuration::get().load();
	startup_phase("identity-load");
	if (!init_sdl_identity()) return 1;
	startup_phase("identity-ready");
	if (asset_check) {
		if (auto failure = ms::init(true)) {
			std::cerr << failure.get_message() << failure.get_args() << '\n';
			return 1;
		}
		bool passed = check_sdl_audio();
		for (int frame = 0; frame < 10; ++frame) {
			ms::UI::get().update();
			ms::draw(1);
		}
		const auto graphics_error = glGetError();
		if (graphics_error != GL_NO_ERROR) std::cerr << "NX rendering GL error: " << graphics_error << '\n';
		passed = passed && graphics_error == GL_NO_ERROR;
		ms::Window::get().take_screenshot();
		ms::Sound::close();
		std::cout << (passed ? "NX login rendering and audio passed\n" : "NX platform check failed\n");
		return passed ? 0 : 1;
	}
#else
	ms::HardwareInfo();
	ms::ScreenResolution();
#endif
#ifdef OPENSTORY_SDL
	return ms::start(server_select);
#else
	return ms::start();
#endif
}
