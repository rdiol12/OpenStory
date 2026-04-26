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
#include <string>
#include <vector>

namespace ms
{
	// Add-buddy popup. Backdrop is UIWindow2.img/UserList/AddFriend
	// (260x247). Sections, top to bottom:
	//   1. "Please enter a character name." + name field
	//   2. "Please enter a nickname."        + nickname field
	//   3. "Memo (optional)"                 + memo textarea
	//   4. Group radio toggle — click to expand a dropdown listing all
	//      groups currently available; click a row to pick.
	// On OK: AddBuddyPacket(name, group) is dispatched and any
	// nickname / memo are stashed in BuddyMemoStore.
	class UIAddBuddy : public UIDragElement<PosADDBUDDY>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::ADDBUDDY;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIAddBuddy();
		UIAddBuddy(Point<int16_t> spawn);
		UIAddBuddy(const std::string& prefill_name, const std::string& prefill_group);
		UIAddBuddy(const std::string& prefill_name, const std::string& prefill_group, Point<int16_t> spawn);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_OK,
			BT_CANCEL
		};

		void rebuild_groups();
		void commit();

		Texture radio_box;     // 11×11 toggle indicator
		Texture radio_check;   // 6×6 fill overlay (drawn when dropdown open OR group picked)

		Text prompt_name;
		Text prompt_nick;
		Text label_memo;
		Text typing_indicator;       // shown in place of a prompt label
		                              // when its field is focused
		mutable Text radio_label;
		mutable Text dropdown_label;

		Textfield name_field;
		Textfield nick_field;
		Textfield memo_field;

		// Group dropdown state.
		std::string selected_group;
		mutable bool dropdown_open = false;
		mutable Rectangle<int16_t> radio_hit;
		struct DropdownRow
		{
			std::string name;
			Rectangle<int16_t> hit;
		};
		mutable std::vector<DropdownRow> dropdown_rows;
	};
}
