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
#include "UIMonsterBook.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMonsterBook::UIMonsterBook() : UIDragElement<PosMONSTERBOOK>(), cur_page(0), num_pages(MAX_PAGES)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["MonsterBook"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// Load additional textures for drawing on the book pages
		cover = src["cover"];
		card_slot = src["cardSlot"];
		info_page = src["infoPage"];

		// Close button in the top-right corner
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));

		// Page navigation arrows
		buttons[Buttons::BT_ARROW_LEFT] = std::make_unique<MapleButton>(src["arrowLeft"]);
		buttons[Buttons::BT_ARROW_RIGHT] = std::make_unique<MapleButton>(src["arrowRight"]);

		// Search button
		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(src["BtSearch"]);

		// Left-side category tabs (0-8)
		nl::node left_tab = src["LeftTab"];

		for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB8; i++)
		{
			uint16_t tabid = i - Buttons::BT_TAB0;
			nl::node tab_node = left_tab[std::to_string(tabid)];

			if (tab_node.size() > 0)
				buttons[i] = std::make_unique<MapleButton>(tab_node);
		}

		// Page number text at the bottom center of the book
		page_text = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::WHITE, "1 / " + std::to_string(num_pages));

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		// Initialize button states
		update_buttons();
	}

	void UIMonsterBook::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw cover on the first page
		if (cur_page == 0)
		{
			cover.draw(position);
		}
		else
		{
			// Draw empty card slots on the left page (4 slots in a 2x2 grid)
			Point<int16_t> left_page_origin = position + Point<int16_t>(40, 65);

			for (int16_t row = 0; row < 2; row++)
			{
				for (int16_t col = 0; col < 2; col++)
				{
					Point<int16_t> slot_pos = left_page_origin + Point<int16_t>(col * 80, row * 100);
					card_slot.draw(slot_pos);
				}
			}

			// Draw empty card slots on the right page (4 slots in a 2x2 grid)
			Point<int16_t> right_page_origin = position + Point<int16_t>(248, 65);

			for (int16_t row = 0; row < 2; row++)
			{
				for (int16_t col = 0; col < 2; col++)
				{
					Point<int16_t> slot_pos = right_page_origin + Point<int16_t>(col * 80, row * 100);
					card_slot.draw(slot_pos);
				}
			}
		}

		// Draw page number text at the bottom of the book
		page_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 18));

		UIElement::draw_buttons(inter);
	}

	void UIMonsterBook::update()
	{
		UIElement::update();
	}

	Cursor::State UIMonsterBook::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIMonsterBook::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIMonsterBook::get_type() const
	{
		return TYPE;
	}

	Button::State UIMonsterBook::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_ARROW_LEFT:
			if (cur_page > 0)
				set_page(cur_page - 1);

			return Button::State::NORMAL;
		case Buttons::BT_ARROW_RIGHT:
			if (cur_page < num_pages - 1)
				set_page(cur_page + 1);

			return Button::State::NORMAL;
		case Buttons::BT_SEARCH:
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
		case Buttons::BT_TAB5:
		case Buttons::BT_TAB6:
		case Buttons::BT_TAB7:
		case Buttons::BT_TAB8:
			// Category tab pressed - reset to first page for now
			set_page(0);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIMonsterBook::set_page(int16_t page)
	{
		cur_page = page;
		page_text.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
		update_buttons();
	}

	void UIMonsterBook::update_buttons()
	{
		// Disable left arrow on first page, right arrow on last page
		if (cur_page <= 0)
			buttons[Buttons::BT_ARROW_LEFT]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::BT_ARROW_LEFT]->set_state(Button::State::NORMAL);

		if (cur_page >= num_pages - 1)
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::NORMAL);
	}
}
