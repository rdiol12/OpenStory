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

#include "../../Graphics/Text.h"

namespace ms
{
	class UIMonsterBook : public UIDragElement<PosMONSTERBOOK>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MONSTERBOOK;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIMonsterBook();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void set_page(int16_t page);
		void update_buttons();

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_ARROW_LEFT,
			BT_ARROW_RIGHT,
			BT_SEARCH,
			BT_TAB0,
			BT_TAB1,
			BT_TAB2,
			BT_TAB3,
			BT_TAB4,
			BT_TAB5,
			BT_TAB6,
			BT_TAB7,
			BT_TAB8,
			NUM_BUTTONS
		};

		// Number of card slots per page (4 columns x 2 rows on each side = 8 per spread)
		static constexpr int16_t CARDS_PER_PAGE = 8;
		static constexpr int16_t MAX_PAGES = 10;

		Texture cover;
		Texture card_slot;
		Texture info_page;

		Text page_text;

		int16_t cur_page;
		int16_t num_pages;
	};
}
