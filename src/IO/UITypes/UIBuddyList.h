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

#include "../../Character/BuddyList.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIBuddyList : public UIDragElement<PosBUDDYLIST>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::BUDDYLIST;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIBuddyList();

		void draw(float inter) const override;
		void update() override;

		Button::State button_pressed(uint16_t buttonid) override;

		Cursor::State send_cursor(bool pressed, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	private:
		// Refresh cached buddy data from the player's BuddyList
		void refresh_buddies();

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_ADD,
			BT_DELETE,
			BT_WHISPER
		};

		// Row backgrounds (friend0 through friend6)
		Texture row_textures[7];

		// Status icons (icon0 through icon5)
		Texture status_icons[6];

		// Separator line
		Texture line;

		// Button area background
		Texture button_bg;

		// Cached buddy entries for rendering
		struct BuddyDisplay
		{
			int32_t cid;
			std::string name;
			std::string group;
			int32_t channel;
			bool is_online;
		};

		std::vector<BuddyDisplay> cached_buddies;

		// Text objects for drawing buddy rows (mutable so draw() can update text)
		mutable Text name_label;
		mutable Text status_label;
		mutable Text group_label;
		mutable Text title_text;

		// Max visible rows
		static constexpr int16_t MAX_VISIBLE = 7;

		// Row height in pixels
		static constexpr int16_t ROW_HEIGHT = 24;

		// Selected buddy index (-1 = none)
		int16_t selected_buddy;

		// Scroll offset
		int16_t scroll_offset;
	};
}
