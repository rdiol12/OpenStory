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

#include <functional>
#include <string>

namespace ms
{
	// Single high-level entry point for all in-game notifications.
	// Each call:
	//   1. Spawns a transient toast above the status bar (~10s).
	//   2. Pushes a persistent entry to NotificationCenter so the
	//      status-bar drawer keeps it until the user resolves it.
	//   3. Activates the bell badge on the status bar.
	//
	// `resolver(true)` runs on Accept (toast click OR drawer Accept).
	// `resolver(false)` runs on drawer Decline. Pass an empty function
	// for purely informational entries with no follow-up action.
	namespace Notifications
	{
		void notify(std::string title, std::string body,
			std::function<void(bool yes)> resolver);

		// Convenience: informational notification with no Accept action.
		// Clicking the toast just dismisses it.
		void info(std::string title, std::string body);
	}
}
