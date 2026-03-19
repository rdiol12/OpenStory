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
#include "../Components/TwoSpriteButton.h"

#include "../../Graphics/Text.h"

namespace ms
{
	class UIGuild : public UIDragElement<PosGUILD>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::GUILD;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIGuild();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		// Called from guild packet handler
		void set_guild_info(const std::string& name, const std::string& notice, int16_t level, int16_t capacity);
		void add_member(const std::string& name, const std::string& rank, int16_t level, int16_t job, bool online);
		void clear_members();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_tab(uint16_t tabid);

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_TAB0,
			BT_TAB1,
			BT_TAB2,
			BT_TAB3
		};

		uint16_t tab;

		mutable Text guild_name;
		mutable Text guild_notice;
		mutable Text guild_level;
		mutable Text guild_capacity;
		mutable Text member_count_text;

		struct MemberEntry
		{
			std::string name;
			std::string rank;
			int16_t level;
			int16_t job;
			bool online;
		};

		std::vector<MemberEntry> members;
		mutable Text member_name_label;
		mutable Text member_info_label;
		int16_t scroll_offset;

		static constexpr int16_t MAX_VISIBLE_MEMBERS = 8;
		static constexpr int16_t MEMBER_ROW_HEIGHT = 24;
		static constexpr int16_t MEMBER_LIST_Y = 115;
	};
}
