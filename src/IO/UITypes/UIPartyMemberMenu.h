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
	// Right-click popup for a party member row. Backdrop is the
	// 3-piece SideMenu strip from
	// UIWindow2.img/UserList/Main/Party/SideMenu, with the seven
	// buttons (Whisper, Friend, Chat, Message, Follow, Master,
	// KickOut). Master + KickOut only show when the local player
	// holds party leader.
	class UIPartyMemberMenu : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYMEMBERMENU;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIPartyMemberMenu(int32_t cid, const std::string& name,
			Point<int16_t> spawn, bool is_leader, bool target_is_self);

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_WHISPER,
			BT_FRIEND,
			BT_CHAT,
			BT_MESSAGE,
			BT_FOLLOW,
			BT_MASTER,
			BT_KICKOUT,
			NUM_BUTTONS
		};

		int32_t target_cid;
		std::string target_name;

		// 3-piece chrome shared with UIBuddyContextMenu — see
		// Components/SideMenuBackdrop for layout constants.
		SideMenuBackdrop chrome;

		size_t visible_count = 0;
		int16_t height = 0;
	};
}
