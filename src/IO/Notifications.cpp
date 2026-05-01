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
#include "Notifications.h"

#include "NotificationCenter.h"
#include "UI.h"
#include "UITypes/UIStatusBar.h"
#include "UITypes/UIToastStack.h"

namespace ms
{
	namespace Notifications
	{
		void notify(std::string title, std::string body,
			std::function<void(bool yes)> resolver)
		{
			// Persistent drawer entry — kept until user clicks Accept or
			// Decline in the bell-button popup. Returns an id we use to
			// resolve the same entry from the toast.
			int32_t entry_id = NotificationCenter::get().push(
				title, body, resolver);

			// Transient toast — auto-dismisses after ~10s, or sooner if
			// the user clicks Accept/Decline. Either path routes through
			// NotificationCenter::resolve so the drawer entry is removed
			// in lockstep.
			auto toasts = UI::get().get_element<UIToastStack>();
			if (!toasts)
			{
				UI::get().emplace<UIToastStack>();
				toasts = UI::get().get_element<UIToastStack>();
			}
			if (toasts)
			{
				toasts->push(std::move(title), std::move(body),
					[entry_id](bool yes)
					{
						NotificationCenter::get().resolve(entry_id, yes);
					});
			}

			// Bell badge on the status bar.
			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();
		}

		void info(std::string title, std::string body)
		{
			notify(std::move(title), std::move(body), nullptr);
		}
	}
}
