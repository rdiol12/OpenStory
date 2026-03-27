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
#include "../Components/Gauge.h"

#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <vector>

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
		void send_scroll(double yoffset) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void update_card(int32_t cardid, int8_t level);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void set_page(int16_t page);
		void update_buttons();
		void load_cards();
		void set_tab(int16_t tab);
		void select_card(int16_t idx);
		int16_t card_at_cursor(Point<int16_t> cursorpos) const;
		void draw_number(Point<int16_t> pos, int32_t number, bool large) const;
		void draw_detail(float inter) const;
		void draw_set_effect(float inter) const;

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_ARROW_LEFT,
			BT_ARROW_RIGHT,
			BT_SEARCH,
			BT_SETEFFECT,
			BT_TAB0,
			BT_TAB1,
			BT_TAB2,
			BT_TAB3,
			BT_TAB4,
			BT_TAB5,
			BT_TAB6,
			BT_TAB7,
			BT_TAB8,
			BT_RTAB0,
			BT_RTAB1,
			BT_RTAB2,
			BT_RTAB3,
			BT_RTAB4,
			BT_RTAB5,
			BT_RTAB6,
			BT_RTAB7,
			BT_RTAB8,
			NUM_BUTTONS
		};

		// Card grid: 5 columns x 5 rows per page side
	static constexpr int16_t COLS_PER_SIDE = 5;
	static constexpr int16_t ROWS_PER_SIDE = 5;
	static constexpr int16_t CARDS_PER_SIDE = COLS_PER_SIDE * ROWS_PER_SIDE; // 25
	static constexpr int16_t CARDS_PER_PAGE = CARDS_PER_SIDE * 2; // 50 per spread

		struct CardEntry
		{
			int32_t cardid;
			int8_t level;
			int32_t mobid = 0;
			Texture mob_icon;
			Texture card_icon;
			std::string mob_name;
			int32_t mob_level = 0;
			int32_t mob_hp = 0;
			int32_t mob_mp = 0;
			int32_t mob_watk = 0;
			int32_t mob_wdef = 0;
			int32_t mob_matk = 0;
			int32_t mob_mdef = 0;
			int32_t mob_exp = 0;
			std::vector<std::pair<int32_t, Texture>> drops; // itemid, icon
		};

		// NX sprites
		Texture cover;
		Texture card_slot;
		Texture select_tex;
		Texture full_mark;
		Texture info_page;
		Texture book_info0;
		Texture book_info1;
		Texture monster_info;
		Texture marks[5];
		Texture num_large[10];
		Texture num_small[11];
		Texture card_category[10];
		Texture all_card;
		Texture set_icon_back;
		Texture set_info0;
		Texture set_info1;

		Text page_text;
		Text card_count_text;
		Text book_level_text;
		Text normal_count_text;
		Text special_count_text;

		int16_t get_card_category(int32_t cardid) const;

		std::vector<CardEntry> sorted_cards;
		std::vector<CardEntry> filtered_cards;
		int16_t cur_page;
		int16_t num_pages;
		int16_t cur_tab;
		int16_t hovered_card;
		int16_t selected_card = -1;

		// Per-category card counts for cover page
		int32_t cat_collected[9] = {};
		int32_t cat_total[9] = {};
		int32_t total_collected = 0;

		// Detail page state
		mutable Animation detail_stand;
		mutable Animation detail_move;
		mutable Animation detail_die;
		int8_t detail_anim_state = 0; // 0=stand, 1=move, 2=die

		Gauge detail_hp_gauge;
		Gauge detail_mp_gauge;

		int16_t detail_drop_offset = 0; // scroll offset for drops
		int16_t hovered_drop = -1; // hovered drop item index

		// Set effect state
		bool show_set_page = false;
		int16_t active_set = -1; // currently active set (-1 = none)
		int16_t hovered_set = -1; // hovered set row
		int16_t set_scroll = 0; // scroll offset for set grid

		struct CardSet
		{
			std::string name;
			std::string bonus;
			std::vector<int32_t> mob_ids; // mobs in this set
			int32_t collected = 0;
			int32_t total = 0;
			Texture icon; // representative icon (first mob's card)
		};

		std::vector<CardSet> card_sets;
		void load_sets();

		// Pre-allocated draw objects
		mutable Text card_name_text;
		mutable Text mob_name_text;
		mutable Text mob_stat_text;
		mutable Text hp_text;
		mutable Text mp_text;
	};
}
