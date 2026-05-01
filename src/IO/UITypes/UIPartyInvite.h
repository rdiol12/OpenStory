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

#include "../UIDragElement.h"
#include "../Components/Textfield.h"

#include "../../Graphics/Text.h"
#include "../../Configuration.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{
	// Party invite popup — UIWindow2.img/UserList/PopupInvite
	// (264x382). NX-styled chrome (title, backgrnd2/3, column-header
	// buttons, BtInviteName, INVITE-column label, BtInvite per row).
	// The "Recommended" table is populated with online buddies as a
	// best-fit substitute since Cosmic v83 has no server-side roster
	// recommendation.
	class UIPartyInvite : public UIDragElement<PosPARTYINVITE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYINVITE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIPartyInvite();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,          // X in the title bar
			BT_INVITE_NAME,    // top-right invite button next to name field
			BT_COL_NAME,       // sortable column header
			BT_COL_JOB,
			BT_COL_LEVEL
		};

		void dispatch_invite(const std::string& name);

		struct InviteRow
		{
			std::string name;
			std::string job;            // empty when not known (buddies)
			int16_t level = 0;          // 0 when not known
			Rectangle<int16_t> hit;     // per-row INVITE hit-rect
		};

		void rebuild_rows() const;

		Textfield name_field;

		Texture title;
		Texture backgrnd2;
		Texture backgrnd3;
		Texture table_bg;
		Texture invite_label;          // INVITE-column header sprite
		Texture row_invite_tex;        // per-row INVITE button (normal)
		Texture row_invite_hover_tex;  // per-row INVITE button (mouseOver)
		Texture row_invite_pressed_tex;// per-row INVITE button (pressed)

		mutable std::vector<InviteRow> rows;
		mutable int16_t list_scroll = 0;
		mutable int16_t hovered_row  = -1;
	};
}
