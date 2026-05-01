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
#include "UIBuddyContextMenu.h"

#include "UIAddBuddy.h"
#include "UIBuddyNickname.h"
#include "UINotice.h"
#include "UIWhisper.h"

#include "../../Gameplay/Stage.h"
#include "../../Character/BuddyList.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Net/Packets/BuddyPackets.h"
#include "../../Net/Packets/GameplayPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIBuddyContextMenu::UIBuddyContextMenu(int32_t cid, const std::string& name,
		Point<int16_t> spawn, bool online_only_actions)
		: UIElement(spawn, Point<int16_t>(SideMenuBackdrop::WIDTH, 0), true),
		  target_cid(cid),
		  target_name(name)
	{
		nl::node menu = nl::nx::ui["UIWindow2.img"]["UserList"]["Main"]["Friend"]["SideMenu"];
		chrome.load(menu);

		const char* button_names[NUM_BUTTONS] = {
			"BtMemo", "BtConvert", "BtDelete", "BtWhisper",
			"BtChat", "BtMessage", "BtParty", "BtBlock", "BtUnBlock"
		};

		// Pick which actions to include.
		// Offline buddy / All sub-tab → only Memo + Delete.
		// Online buddy / Online sub-tab → all 9 actions.
		std::vector<uint16_t> visible;
		if (online_only_actions)
			visible = { BT_MEMO, BT_CONVERT, BT_DELETE, BT_WHISPER,
				BT_CHAT, BT_MESSAGE, BT_PARTY, BT_BLOCK, BT_UNBLOCK };
		else
			visible = { BT_MEMO, BT_DELETE };

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

		// Caller is expected to pass a spawn that already accounts for
		// where the menu should appear (e.g., after the buddy's name
		// on the row). Use it verbatim.
		position = spawn;
	}

	void UIBuddyContextMenu::draw(float inter) const
	{
		chrome.draw(position, visible_count);
		UIElement::draw_buttons(inter);
	}

	Cursor::State UIBuddyContextMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Click outside the menu dismisses it (matches UISystemMenu).
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

	Button::State UIBuddyContextMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_MEMO:
			UI::get().emplace<UIBuddyNickname>(target_cid, target_name);
			break;
		case BT_CONVERT:
		{
			// "Convert to account friend" doesn't exist in v83. Repurpose
			// the button as "Move to group..." — open UIAddBuddy prefilled
			// with this buddy's name + current group; user edits the
			// group label and OK dispatches AddBuddyPacket which the
			// server treats as a group reassignment.
			const auto& entries = Stage::get().get_player().get_buddylist().get_entries();
			auto it = entries.find(target_cid);
			std::string cur = (it != entries.end() && !it->second.group.empty())
				? it->second.group : std::string("Default Group");
			UI::get().emplace<UIAddBuddy>(target_name, cur);
			break;
		}
		case BT_DELETE:
		{
			int32_t cid = target_cid;
			std::string nm = target_name;
			auto onok = [cid, nm](bool yes)
			{
				if (yes)
					DeleteBuddyPacket(cid).dispatch();
			};
			UI::get().emplace<UIYesNo>(
				"Delete " + nm + " from your buddy list?", onok);
			break;
		}
		case BT_WHISPER:
			UI::get().emplace<UIWhisper>(target_name);
			break;
		case BT_CHAT:
		case BT_MESSAGE:
			UI::get().emplace<UIOk>(
				"Group chat is not yet implemented.", [](bool) {});
			break;
		case BT_PARTY:
			InviteToPartyPacket(target_name).dispatch();
			break;
		case BT_BLOCK:
		case BT_UNBLOCK:
			UI::get().emplace<UIOk>(
				"Block list is not yet implemented.", [](bool) {});
			break;
		}

		deactivate();
		return Button::State::NORMAL;
	}

	UIElement::Type UIBuddyContextMenu::get_type() const
	{
		return TYPE;
	}
}
