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

#include "Gameplay/Stage.h"
#include "IO/UI.h"
#include "IO/Window.h"
#include "Net/Session.h"
#include "Util/HardwareInfo.h"
#include "Util/ScreenResolution.h"

#ifdef USE_NX
#include "Util/NxFiles.h"
#else
#include "Util/WzFiles.h"
#endif

namespace ms
{
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

	void draw(float alpha)
	{
		Window::get().begin();
		Stage::get().draw(alpha);
		UI::get().draw(alpha);
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

		int64_t period = 0;
		int32_t samples = 0;

		bool show_fps = Configuration::get().get_show_fps();

		// 120 FPS cap: ~8333 microseconds per frame
		constexpr int64_t FRAME_CAP_US = 1000000 / 120;

		while (running())
		{
			auto frame_start = std::chrono::high_resolution_clock::now();

			int64_t elapsed = Timer::get().stop();

			// Update game with constant timestep as many times as possible.
			for (accumulator += elapsed; accumulator >= timestep; accumulator -= timestep)
				update();

			// Draw the game. Interpolate to account for remaining time.
			float alpha = static_cast<float>(accumulator) / timestep;
			draw(alpha);

			if (show_fps)
			{
				if (samples < 100)
				{
					period += elapsed;
					samples++;
				}
				else if (period)
				{
					period = 0;
					samples = 0;
				}
			}

			// Cap framerate to 120 FPS
			auto frame_end = std::chrono::high_resolution_clock::now();
			auto frame_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();

			if (frame_us < FRAME_CAP_US)
				std::this_thread::sleep_for(std::chrono::microseconds(FRAME_CAP_US - frame_us));
		}

		Sound::close();
	}

	void start()
	{
		// Initialize and check for errors
		if (Error error = init())
		{
			std::cerr << "[Error] " << error.get_message() << error.get_args() << std::endl;

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
	ms::HardwareInfo();
	ms::ScreenResolution();
	ms::start();

	return 0;
}
