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

#include "../../Configuration.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"

#include <algorithm>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMonsterBook::UIMonsterBook() : UIDragElement<PosMONSTERBOOK>(), cur_page(0), num_pages(1)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["MonsterBook"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		cover = src["cover"];
		card_slot = src["cardSlot"];

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_ARROW_LEFT] = std::make_unique<MapleButton>(src["arrowLeft"]);
		buttons[Buttons::BT_ARROW_RIGHT] = std::make_unique<MapleButton>(src["arrowRight"]);
		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(src["BtSearch"]);

		nl::node left_tab = src["LeftTab"];

		for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB8; i++)
		{
			uint16_t tabid = i - Buttons::BT_TAB0;
			nl::node tab_node = left_tab[std::to_string(tabid)];

			if (tab_node.size() > 0)
				buttons[i] = std::make_unique<MapleButton>(tab_node);
		}

		page_text = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::WHITE, "1 / 1");
		card_count_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Cards: 0");

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		load_cards();
		update_buttons();
	}

	void UIMonsterBook::load_cards()
	{
		sorted_cards.clear();

		auto& cards = Stage::get().get_player().get_monsterbook().get_cards();

		for (auto& [cardid, level] : cards)
		{
			CardEntry entry;
			entry.cardid = cardid;
			entry.level = level;
			entry.full_itemid = 2380000 + cardid;
			sorted_cards.push_back(entry);
		}

		std::sort(sorted_cards.begin(), sorted_cards.end(),
			[](const CardEntry& a, const CardEntry& b) { return a.cardid < b.cardid; });

		int16_t total = static_cast<int16_t>(sorted_cards.size());
		num_pages = (total > 0) ? ((total - 1) / CARDS_PER_PAGE + 1) + 1 : 1; // +1 for cover page

		if (cur_page >= num_pages)
			cur_page = num_pages - 1;

		card_count_text.change_text("Cards: " + std::to_string(total));
		page_text.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
	}

	void UIMonsterBook::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		if (cur_page == 0)
		{
			cover.draw(position);

			// Draw card count on cover page
			card_count_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2 + 20));
		}
		else
		{
			// Card pages - show 8 cards per page (4 left, 4 right)
			int16_t card_start = (cur_page - 1) * CARDS_PER_PAGE;

			// Left page - 4 slots (2x2)
			Point<int16_t> left_page_origin = position + Point<int16_t>(40, 65);

			for (int16_t slot = 0; slot < 4; slot++)
			{
				int16_t row = slot / 2;
				int16_t col = slot % 2;
				Point<int16_t> slot_pos = left_page_origin + Point<int16_t>(col * 80, row * 100);

				card_slot.draw(slot_pos);

				int16_t card_index = card_start + slot;

				if (card_index < static_cast<int16_t>(sorted_cards.size()))
				{
					auto& entry = sorted_cards[card_index];
					const ItemData& idata = ItemData::get(entry.full_itemid);

					if (idata.is_valid())
					{
						// Draw card icon centered in slot
						idata.get_icon(false).draw(DrawArgument(slot_pos + Point<int16_t>(16, 16)));

						// Draw card level stars below icon
						std::string level_str = "Lv." + std::to_string(entry.level);
						Text level_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, level_str);
						level_text.draw(slot_pos + Point<int16_t>(35, 70));
					}

					// Draw card name
					const std::string& name = idata.is_valid() ? idata.get_name() : "???";
					Text name_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, name);
					name_text.draw(slot_pos + Point<int16_t>(35, 82));
				}
			}

			// Right page - 4 slots (2x2)
			Point<int16_t> right_page_origin = position + Point<int16_t>(248, 65);

			for (int16_t slot = 0; slot < 4; slot++)
			{
				int16_t row = slot / 2;
				int16_t col = slot % 2;
				Point<int16_t> slot_pos = right_page_origin + Point<int16_t>(col * 80, row * 100);

				card_slot.draw(slot_pos);

				int16_t card_index = card_start + 4 + slot;

				if (card_index < static_cast<int16_t>(sorted_cards.size()))
				{
					auto& entry = sorted_cards[card_index];
					const ItemData& idata = ItemData::get(entry.full_itemid);

					if (idata.is_valid())
					{
						idata.get_icon(false).draw(DrawArgument(slot_pos + Point<int16_t>(16, 16)));

						std::string level_str = "Lv." + std::to_string(entry.level);
						Text level_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, level_str);
						level_text.draw(slot_pos + Point<int16_t>(35, 70));
					}

					const std::string& name = idata.is_valid() ? idata.get_name() : "???";
					Text name_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, name);
					name_text.draw(slot_pos + Point<int16_t>(35, 82));
				}
			}
		}

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

	void UIMonsterBook::update_card(int16_t cardid, int8_t level)
	{
		load_cards();
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
