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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ms
{
	// Drawer-style notification queue feeding UINotificationList. Net
	// handlers (BuddyListHandler / PartyOperationHandler / GuildHandler
	// / etc.) push entries here when they receive an invite, in
	// addition to spawning the modal Yes/No popup. Clicking BT_NOTICE
	// on the status bar opens the list, where each entry can be
	// re-resolved (Accept / Decline).
	class NotificationCenter
	{
	public:
		struct Entry
		{
			int32_t id;
			std::string title;
			std::string body;
			std::function<void(bool yes)> resolver;
		};

		static NotificationCenter& get();

		// Push a new notification. Returns the entry id.
		int32_t push(std::string title, std::string body,
			std::function<void(bool yes)> resolver);

		// Resolve and remove an entry. `yes=true` invokes the Accept
		// path, `yes=false` invokes Decline.
		void resolve(int32_t id, bool yes);

		// Drop without invoking the resolver (e.g. user dismissed all).
		void dismiss(int32_t id);
		void clear();

		bool empty() const;
		const std::vector<Entry>& list() const;

	private:
		NotificationCenter() = default;

		int32_t next_id = 1;
		std::vector<Entry> pending;
	};
}
