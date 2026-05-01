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

#include "../UIElement.h"
#include "../Components/SideMenuBackdrop.h"
#include "../../Graphics/Texture.h"

#include <cstdint>
#include <string>

namespace ms
{
	// Right-click popup over a buddy row in UIUserList. Backdrop is the
	// 3-piece SideMenu strip from UIWindow2.img/UserList/Main/Friend/SideMenu;
	// each button uses its baked normal/mouseOver/pressed states.
	class UIBuddyContextMenu : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::BUDDYCONTEXTMENU;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		// `online_only_actions` = true when the menu is opened from the
		// Online sub-tab (or any context where the buddy is online and
		// chat/party/block actions make sense). When false, the menu
		// trims to just Memo + Delete (the only two operations that
		// make sense on an offline buddy).
		UIBuddyContextMenu(int32_t cid, const std::string& name,
			Point<int16_t> spawn, bool online_only_actions);

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_MEMO,
			BT_CONVERT,
			BT_DELETE,
			BT_WHISPER,
			BT_CHAT,
			BT_MESSAGE,
			BT_PARTY,
			BT_BLOCK,
			BT_UNBLOCK,
			NUM_BUTTONS
		};

		int32_t target_cid;
		std::string target_name;

		// 3-piece chrome shared with UIPartyMemberMenu — see
		// Components/SideMenuBackdrop for layout constants.
		SideMenuBackdrop chrome;

		// Computed from the actually-placed button count rather than
		// fixed at all 9 — the menu is shorter on the All sub-tab.
		int16_t height       = 0;
		size_t  visible_count = 0;
	};
}
