//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
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

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <cstdint>
#include <map>
#include <vector>

namespace ms
{
	class UIMonsterBattle : public UIDragElement<PosMONSTERBATTLE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MONSTERBATTLE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIMonsterBattle();

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
			BT_CLOSE,
			BT_PREV,
			BT_NEXT,
			BT_SETTEAM
		};

		static constexpr int16_t COLS = 6;
		static constexpr int16_t ROWS = 5;
		static constexpr int16_t PER_PAGE = COLS * ROWS;
		static constexpr int16_t SLOT_W = 29;
		static constexpr int16_t SLOT_H = 40;
		static constexpr int16_t STEP_X = 34;
		static constexpr int16_t STEP_Y = 46;
		static constexpr int16_t GRID_X = 360;
		static constexpr int16_t GRID_Y = 90;
		static constexpr size_t TEAM_MAX = 6;

		void refresh();
		void set_page(int16_t page);
		int16_t slot_at(Point<int16_t> cursorpos) const;
		Point<int16_t> slot_pos(int16_t index) const;
		Texture card_texture(int32_t cardid) const;
		bool in_team(int32_t cardid) const;

		Texture slot_empty;
		Texture slot_select;
		Texture slot_selected;
		Texture mark;
		Texture name_plate;
		Texture tier_plate;

		mutable std::map<int32_t, Texture> card_cache;

		Text page_label;
		Text detail_label;
		Text team_label;

		int16_t cur_page = 0;
		int16_t num_pages = 1;
		int16_t hovered = -1;
		int16_t selected = -1;
		size_t last_unit_count = 0;
		uint8_t last_error = 0;
	};
}
