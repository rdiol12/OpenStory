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
#include "../../Graphics/Geometry.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace ms
{
	// Manage existing buddy groups. Backdrop UIWindow2.img/UserList/Group
	// (264x382). Server has no "create empty group" or "rename group"
	// primitive — both ops are emulated by re-dispatching mode-1
	// AddBuddyPacket(name, new_group) for every buddy currently in the
	// group; the server in-memory updates the entry and pushes a fresh
	// 0x07 full refresh, which redraws the Friend tab automatically.
	class UIBuddyGroup : public UIDragElement<PosBUDDYGROUP>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::BUDDYGROUP;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIBuddyGroup();
		UIBuddyGroup(Point<int16_t> spawn);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void doubleclick(Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_OK,        // Apply rename if a group is selected and the
			              // textfield has text different from the row.
			BT_CANCEL
		};

		void rebuild_layout();
		void apply_rename(const std::string& old_name, const std::string& new_name);
		void apply_delete(const std::string& old_name);
		void apply_move_checked(const std::string& target_group);

		struct GroupRow
		{
			std::string name;
			int member_count;
			Rectangle<int16_t> hit;
		};

		struct BuddyRow
		{
			int32_t cid;
			std::string name;
			std::string current_group;
			Rectangle<int16_t> hit;        // whole row (left of group column)
			Rectangle<int16_t> checkbox;   // checkbox area
			Rectangle<int16_t> group_cell; // GROUP column cell — hover/dbl-click target
		};

		// Overlay states sourced from UIWindow2.img/UserList/Group/Popup.
		// Each one is a baked sub-bitmap that floats over the dialog
		// for a specific operation (delete confirm, name-required
		// nag, etc.).
		enum class Popup : uint8_t
		{
			NONE,
			NEED_MESSAGE,    // user hit OK without typing a new name
			GROUP_DEL,       // confirm group deletion
			GROUP_DEL_DENY,  // can't delete (e.g. Default Group)
			ADD_FRIEND,      // success notice
			GROUP_WHISPER,   // group whisper confirm (cosmetic — Cosmic has no op)
		};
		Popup popup_state = Popup::NONE;
		std::string popup_target;      // group name affected by the popup

		Texture popup_addfriend;
		Texture popup_base;
		Texture popup_groupdel;
		Texture popup_groupdeldeny;
		Texture popup_groupwhisper;
		Texture popup_needmessage;

		Text caption;                   // "Selected buddies will be moved to the group."
		Text col_header_name;
		Text col_header_group;
		Text hint;
		mutable Text row_label;
		mutable Text row_group_label;

		Textfield new_group_field;     // top textfield: target group name

		Texture checkbox_off;          // 11×11 unchecked
		Texture checkbox_on;            // 6×6 check overlay
		Texture buddy_dot;             // small blue indicator next to each buddy
		Texture row_bg_even;           // Sheet1[1]
		Texture row_bg_odd;            // Sheet1[2]

		// Refilled every draw from BuddyList::get_entries() so the
		// dialog reflects whatever the server most recently pushed.
		mutable std::vector<GroupRow> group_rows;
		mutable std::vector<BuddyRow> buddy_rows;
		std::string selected_group;
		std::set<int32_t> checked_buddies;

		// Vertical scroll for the buddy list — pulled by mouse wheel
		// when the cursor is over the list region.
		mutable int16_t buddy_scroll = 0;

		// Group name currently hovered in the GROUP column. Highlight
		// drawn behind every row whose current group matches.
		mutable std::string hovered_group;
	};
}
