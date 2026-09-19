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
#pragma once

#include "../Configuration.h"

#ifdef _WIN32
#include <windef.h>
#include <WinUser.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace ms
{
	class ScreenResolution
	{
	public:
		ScreenResolution()
		{
#ifdef _WIN32
			RECT desktop;

			// Get a handle to the desktop window
			const HWND hDesktop = GetDesktopWindow();

			// Get the size of screen to the variable desktop
			GetWindowRect(hDesktop, &desktop);

			// The top left corner will have coordinates (0, 0) and the bottom right corner will have coordinates (horizontal, vertical)
			Configuration::get().set_max_width(desktop.right);
			Configuration::get().set_max_height(desktop.bottom);
#elif defined(__APPLE__)
			// Runs before glfwInit (MapleStory.cpp calls this ahead of start()),
			// so the monitor cannot be queried through GLFW here.
			// CGDisplayPixelsWide/High report the mode in points rather than
			// backing pixels, which is what the Windows path yields too and what
			// the int16_t setters below can actually hold on a Retina display.
			const CGDirectDisplayID display = CGMainDisplayID();

			size_t cg_width = CGDisplayPixelsWide(display);
			size_t cg_height = CGDisplayPixelsHigh(display);

			// Both return 0 when there is no window-server connection (ssh, a
			// launchd context, no attached display). Storing the zeros makes
			// Window::toggle_fullscreen a permanent no-op (`width < 0` is never
			// true) and makes Window::check_events force fullscreen on the next
			// resolution change, so keep a usable desktop size instead.
			if (cg_width == 0 || cg_height == 0)
			{
				cg_width = 1920;
				cg_height = 1080;
			}

			// Clamped because the setters below are int16_t: an 8K panel
			// reported in pixels would otherwise wrap to a negative size.
			if (cg_width > 32767)
				cg_width = 32767;

			if (cg_height > 32767)
				cg_height = 32767;

			Configuration::get().set_max_width(static_cast<int16_t>(cg_width));
			Configuration::get().set_max_height(static_cast<int16_t>(cg_height));
#endif
		}
	};
}