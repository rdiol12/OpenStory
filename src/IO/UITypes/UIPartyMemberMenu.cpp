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
#include "UIPartyMemberMenu.h"

#include "UIWhisper.h"
#include "UINotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Net/Packets/BuddyPackets.h"
#include "../../Net/Packets/GameplayPackets.h"

#include <vector>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartyMemberMenu::UIPartyMemberMenu(int32_t cid, const std::string& name,
		Point<int16_t> spawn, bool is_leader, bool target_is_self)
		: UIElement(spawn, Point<int16_t>(SideMenuBackdrop::WIDTH, 0), true),
		  target_cid(cid),
		  target_name(name)
	{
		nl::node menu = nl::nx::ui["UIWindow2.img"]["UserList"]["Main"]["Party"]["SideMenu"];
		chrome.load(menu);

		const char* button_names[NUM_BUTTONS] = {
			"BtWhisper", "BtFriend", "BtChat", "BtMessage",
			"BtFollow", "BtMaster", "BtKickOut"
		};

		// Compose the visible button list. Master / KickOut only when
		// local player is leader and target isn't ourselves. Whisper /
		// Friend hide on self.
		std::vector<uint16_t> visible;
		if (!target_is_self)
		{
			visible.push_back(BT_WHISPER);
			visible.push_back(BT_FRIEND);
		}
		visible.push_back(BT_CHAT);
		visible.push_back(BT_MESSAGE);
		if (!target_is_self)
			visible.push_back(BT_FOLLOW);
		if (is_leader && !target_is_self)
		{
			visible.push_back(BT_MASTER);
			visible.push_back(BT_KICKOUT);
		}

		int16_t y = SideMenuBackdrop::TOP_H;
		for (uint16_t id : visible)
		{
			buttons[id] = std::make_unique<MapleButton>(
				menu[button_names[id]],
				Point<int16_t>(SideMenuBackdrop::BUTTON_X, y));
			y += SideMenuBackdrop::BUTTON_H;
		}

		visible_count = visible.size();
		height = SideMenuBackdrop::height_for(visible_count);
		dimension = Point<int16_t>(SideMenuBackdrop::WIDTH, height);

		position = spawn;
	}

	void UIPartyMemberMenu::draw(float inter) const
	{
		chrome.draw(position, visible_count);
		UIElement::draw_buttons(inter);
	}

	Cursor::State UIPartyMemberMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked)
		{
			Rectangle<int16_t> area(position,
				position + Point<int16_t>(SideMenuBackdrop::WIDTH, height));
			if (!area.contains(cursorpos))
			{
				deactivate();
				return Cursor::State::IDLE;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIPartyMemberMenu::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed) return;
		if (escape)
		{
			deactivate();
			return;
		}
	}

	Button::State UIPartyMemberMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_WHISPER:
			UI::get().emplace<UIWhisper>(target_name);
			break;
		case BT_FRIEND:
			AddBuddyPacket(target_name).dispatch();
			break;
		case BT_MASTER:
		{
			int32_t cid = target_cid;
			std::string nm = target_name;
			UI::get().emplace<UIYesNo>(
				"Pass party leader to " + nm + "?",
				[cid](bool yes)
				{
					if (yes) ChangePartyLeaderPacket(cid).dispatch();
				});
			break;
		}
		case BT_KICKOUT:
		{
			int32_t cid = target_cid;
			std::string nm = target_name;
			UI::get().emplace<UIYesNo>(
				"Kick " + nm + " from the party?",
				[cid](bool yes)
				{
					if (yes) ExpelFromPartyPacket(cid).dispatch();
				});
			break;
		}
		case BT_CHAT:
		case BT_MESSAGE:
		case BT_FOLLOW:
			UI::get().emplace<UIOk>(
				"Not implemented on this server.", [](bool) {});
			break;
		}

		deactivate();
		return Button::State::NORMAL;
	}

	UIElement::Type UIPartyMemberMenu::get_type() const
	{
		return TYPE;
	}
}
