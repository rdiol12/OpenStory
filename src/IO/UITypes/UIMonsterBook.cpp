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

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "UINotice.h"

#include "../../Configuration.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/PlayerPackets.h"
#include "../../Util/Misc.h"

#include <algorithm>
#include <unordered_map>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMonsterBook::UIMonsterBook() : UIDragElement<PosMONSTERBOOK>(Point<int16_t>(300, 20)),
		cur_page(0), num_pages(1), cur_tab(-1), hovered_card(-1)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["MonsterBook"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// Main sprites
		cover = src["cover"];
		card_slot = src["cardSlot"];
		select_tex = src["select"];
		full_mark = src["fullMark"];
		info_page = src["infoPage"];

		// Page info sprites
		nl::node page_info = src["pageInfo"];
		book_info0 = page_info["bookInfo0"];
		book_info1 = page_info["bookInfo1"];
		monster_info = page_info["monsterInfo"];

		for (int i = 0; i < 10; i++)
			card_category[i] = page_info["cardCategory"][std::to_string(i)];

		all_card = page_info["allCard"];
		set_icon_back = page_info["setIconBack"];
		set_info0 = page_info["setInfo0"];
		set_info1 = page_info["setInfo1"];

		// Region icons for set effect page (8 regions)
		for (int i = 0; i < 8; i++)
			set_region_icon[i] = src["icon"][std::to_string(i)];


		// Level marks (0-4 = 1-5 cards collected)
		for (int i = 0; i < 5; i++)
			marks[i] = src["mark"][std::to_string(i)];

		// Number sprites for page display
		for (int i = 0; i < 10; i++)
			num_large[i] = src["numberLarge"][std::to_string(i)];

		for (int i = 0; i < 11; i++)
			num_small[i] = src["numberSmall"][std::to_string(i)];

		// Buttons
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"], Point<int16_t>(bg_dimensions.x() - 14 - 45, 6));
		buttons[Buttons::BT_ARROW_LEFT] = std::make_unique<MapleButton>(src["arrowLeft"], Point<int16_t>(280, 290));
		buttons[Buttons::BT_ARROW_RIGHT] = std::make_unique<MapleButton>(src["arrowRight"], Point<int16_t>(400, 290));
		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(src["BtSearch"]);
		buttons[Buttons::BT_SETEFFECT] = std::make_unique<MapleButton>(src["BtSetEffect"]);
		buttons[Buttons::BT_BACK] = std::make_unique<MapleButton>(src["UpTab"]);
		buttons[Buttons::BT_BACK]->set_active(false);

		// Left tabs
		nl::node left_tab = src["LeftTab"];

		for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB8; i++)
		{
			uint16_t tabid = i - Buttons::BT_TAB0;
			nl::node tab_node = left_tab[std::to_string(tabid)];

			if (tab_node.size() > 0)
			{
				nl::node normal_node = tab_node["normal"]["0"];
				nl::node selected_node = tab_node["selected"]["0"];

				if (normal_node.size() > 0 && selected_node.size() > 0)
					buttons[i] = std::make_unique<TwoSpriteButton>(normal_node, selected_node);
				else
					buttons[i] = std::make_unique<MapleButton>(tab_node);
			}
		}

		// Right tabs (9 category tabs on the right side)
		nl::node right_tab = src["RightTab"];

		for (uint16_t i = Buttons::BT_RTAB0; i <= Buttons::BT_RTAB8; i++)
		{
			uint16_t tabid = i - Buttons::BT_RTAB0;
			nl::node tab_node = right_tab[std::to_string(tabid)];

			if (tab_node.size() > 0)
				buttons[i] = std::make_unique<MapleButton>(tab_node);
		}

		// Text elements
		page_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "1 / 1");
		card_count_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "0");
		book_level_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Lv. 0");
		normal_count_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Normal: 0");
		special_count_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Special: 0");
		card_name_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, "");
		mob_name_text = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::WHITE, "");
		mob_stat_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "");
		hp_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::RED, "");
		mp_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLUE, "");

		// Load HP/MP gauge sprites from status bar
		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];
		nl::node gauge_src = mainbar["gauge"];

		detail_hp_gauge = Gauge(Gauge::Type::GAME, gauge_src["hp"]["0"], gauge_src["hp"]["1"], gauge_src["hp"]["2"], 120, 1.0f);
		detail_mp_gauge = Gauge(Gauge::Type::GAME, gauge_src["mp"]["0"], gauge_src["mp"]["1"], gauge_src["mp"]["2"], 120, 1.0f);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);



		load_cards();
		load_sets();
		set_tab(-1);
		update_buttons();
	}

	int16_t UIMonsterBook::get_card_category(int32_t cardid) const
	{
		bool is_special = (cardid / 1000 >= 2388);

		if (is_special)
			return 8;

		int32_t offset = cardid - 2380000;
		int32_t group = offset / 1000;

		if (group > 7) group = 7;
		if (group < 0) group = 0;

		return static_cast<int16_t>(group);
	}

	void UIMonsterBook::load_cards()
	{
		sorted_cards.clear();

		auto& cards = Stage::get().get_player().get_monsterbook().get_cards();

		for (auto& [cardid, level] : cards)
		{
			if (cardid <= 0)
				continue;

			CardEntry entry;
			entry.cardid = cardid;
			entry.level = level;

			// Look up mob ID from the card item's NX data
			std::string strprefix = "0" + std::to_string(cardid / 10000);
			std::string strid = "0" + std::to_string(cardid);
			nl::node card_info = nl::nx::item["Consume"][strprefix + ".img"][strid]["info"];

			if (card_info)
				entry.mobid = card_info["mob"];

			// Load card icon (iconRaw)
			if (card_info)
				entry.card_icon = Texture(card_info["iconRaw"]);

			// Load mob sprite, name, and stats
			if (entry.mobid > 0)
			{
				std::string mob_strid = string_format::extend_id(entry.mobid, 7);
				nl::node mob_src = nl::nx::mob[mob_strid + ".img"];

				nl::node stand = mob_src["stand"];
				if (stand && stand["0"])
					entry.mob_icon = Texture(stand["0"]);
				else if (mob_src["fly"] && mob_src["fly"]["0"])
					entry.mob_icon = Texture(mob_src["fly"]["0"]);

				nl::node name_node = nl::nx::string["Mob.img"][std::to_string(entry.mobid)];
				if (name_node["name"])
					entry.mob_name = (std::string)name_node["name"];

				nl::node mob_info = mob_src["info"];
				if (mob_info)
				{
					entry.mob_level = mob_info["level"];
					entry.mob_hp = mob_info["maxHP"];
					entry.mob_mp = mob_info["maxMP"];
					entry.mob_watk = mob_info["PADamage"];
					entry.mob_wdef = mob_info["PDDamage"];
					entry.mob_matk = mob_info["MADamage"];
					entry.mob_mdef = mob_info["MDDamage"];
					entry.mob_exp = mob_info["exp"];
				}
			}

			sorted_cards.push_back(entry);
		}

		std::sort(sorted_cards.begin(), sorted_cards.end(),
			[](const CardEntry& a, const CardEntry& b) { return a.cardid < b.cardid; });

		// Compute per-category counts
		for (int i = 0; i < 9; i++)
		{
			cat_collected[i] = 0;
			cat_total[i] = 0;
		}

		total_collected = static_cast<int32_t>(sorted_cards.size());

		for (auto& entry : sorted_cards)
		{
			int16_t cat = get_card_category(entry.cardid);
			if (cat >= 0 && cat < 9)
				cat_collected[cat]++;
		}

		// Count total possible cards per category from NX data
		nl::node consume_root = nl::nx::item["Consume"];
		for (auto img : consume_root)
		{
			for (auto item : img)
			{
				std::string name = item.name();
				if (name.size() < 7)
					continue;

				int32_t itemid = std::atoi(name.c_str());
				if (itemid >= 2380000 && itemid < 2390000)
				{
					int16_t cat = get_card_category(itemid);
					if (cat >= 0 && cat < 9)
						cat_total[cat]++;
				}
			}
		}

		// Update stats text
		auto& mb = Stage::get().get_player().get_monsterbook();
		card_count_text.change_text(std::to_string(mb.get_total_cards()));
		book_level_text.change_text("Lv. " + std::to_string(mb.get_book_level()));
		normal_count_text.change_text("Normal: " + std::to_string(mb.get_normal_cards()));
		special_count_text.change_text("Special: " + std::to_string(mb.get_special_cards()));
	}

	void UIMonsterBook::set_tab(int16_t tab)
	{
		if (cur_tab >= 0 && cur_tab <= 8)
		{
			buttons[Buttons::BT_TAB0 + cur_tab]->set_state(Button::State::NORMAL);
			buttons[Buttons::BT_RTAB0 + cur_tab]->set_state(Button::State::NORMAL);
		}

		cur_tab = tab;

		if (cur_tab >= 0 && cur_tab <= 8)
		{
			buttons[Buttons::BT_TAB0 + cur_tab]->set_state(Button::State::PRESSED);
			buttons[Buttons::BT_RTAB0 + cur_tab]->set_state(Button::State::PRESSED);
		}

		filtered_cards.clear();

		if (cur_tab < 0)
		{
			filtered_cards = sorted_cards;
		}
		else
		{
			for (auto& entry : sorted_cards)
			{
				int32_t full_id = entry.cardid;
				bool is_special = (full_id / 1000 >= 2388);
				bool matches = false;

				if (cur_tab == 8)
				{
					matches = is_special;
				}
				else if (!is_special)
				{
					int32_t offset = entry.cardid - 2380000;
					int32_t group = offset / 1000;

					if (group > 7) group = 7;
					if (group < 0) group = 0;

					matches = (group == cur_tab);
				}

				if (matches)
					filtered_cards.push_back(entry);
			}
		}

		int16_t total = static_cast<int16_t>(filtered_cards.size());
		// Page 0 right side shows CARDS_PER_SIDE, then each page after shows CARDS_PER_PAGE
		if (total <= CARDS_PER_SIDE)
			num_pages = 1;
		else
			num_pages = 1 + ((total - CARDS_PER_SIDE - 1) / CARDS_PER_PAGE + 1);

		cur_page = 0;
		page_text.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
		update_buttons();
	}

	void UIMonsterBook::draw(float inter) const
	{
		// Detail page for selected card
		if (selected_card >= 0 && selected_card < static_cast<int16_t>(filtered_cards.size()))
		{
			draw_detail(inter);
			return;
		}

		// Set effect page
		if (show_set_page)
		{
			draw_set_effect(inter);
			return;
		}

		UIElement::draw_sprites(inter);

		Point<int16_t> left_page = position + Point<int16_t>(50, 27);
		Point<int16_t> right_page = position + Point<int16_t>(247, 27);

		Point<int16_t> card_origin = cover.get_origin();

		int16_t bg_center_x = (dimension.x() - 14) / 2;

		// Card drawing lambda
		int16_t card_w = 35;
		int16_t card_h = 43;
		Point<int16_t> card_grid_offset = Point<int16_t>(0, 50); // below search button

		auto draw_card = [&](Point<int16_t> page_origin, int16_t card_index)
		{
			if (card_index < 0 || card_index >= static_cast<int16_t>(filtered_cards.size()))
				return;

			int16_t local = card_index % CARDS_PER_SIDE;
			int16_t col = local % COLS_PER_SIDE;
			int16_t row = local / COLS_PER_SIDE;

			Point<int16_t> card_topleft = page_origin + card_grid_offset + Point<int16_t>(col * card_w, row * card_h);
			Point<int16_t> draw_pos = card_topleft + card_origin;

			auto& entry = filtered_cards[card_index];

			// Draw card icon (iconRaw shows mob on card)
			Point<int16_t> icon_pos = card_topleft + Point<int16_t>(15, 21);

			if (entry.card_icon.is_valid())
			{
				if (hovered_card == card_index)
					entry.card_icon.draw(DrawArgument(icon_pos, Color(1.3f, 1.3f, 1.3f, 1.0f)));
				else
					entry.card_icon.draw(DrawArgument(icon_pos));
			}
			else
			{
				cover.draw(draw_pos);
			}
		};

		if (cur_page == 0)
		{
			// Cover page: left = category info, right = first batch of cards

			// "Crusader Codex" + "Number of Cards" header
			book_info0.draw(position);

			// Total cards: large number "collected/total"
			int32_t total_possible = 0;
			for (int i = 0; i < 9; i++)
				total_possible += cat_total[i];

			draw_number(left_page + Point<int16_t>(50, 88), total_collected, true);
			draw_number(left_page + Point<int16_t>(110, 88), total_possible, true);

			card_name_text.change_text("Cards");
			card_name_text.draw(left_page + Point<int16_t>(145, 100));

			// 9 category rows (2 columns, 5 rows) with star + collected/total
			Point<int16_t> cat_start = left_page + Point<int16_t>(0, 132);
			int16_t row_h = 27;
			int16_t col_w = 87;

			static const int cat_order[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };

			for (int idx = 0; idx < 9; idx++)
			{
				int tab = cat_order[idx];
				int16_t col = idx % 2;
				int16_t row = idx / 2;
				Point<int16_t> cell_pos = cat_start + Point<int16_t>(col * col_w, row * row_h);

				int sprite_idx = (tab == 0) ? 9 : tab;
				card_category[sprite_idx].draw(cell_pos);

				draw_number(cell_pos + Point<int16_t>(48, 8), cat_collected[tab], false);
				draw_number(cell_pos + Point<int16_t>(68, 8), cat_total[tab], false);
			}

			// Right page: first 25 cards below search
			for (int16_t i = 0; i < CARDS_PER_SIDE && i < static_cast<int16_t>(filtered_cards.size()); i++)
				draw_card(right_page, i);
		}
		else
		{
			// Card pages: both panels show cards
			int16_t card_start;

			if (cur_page == 1)
				card_start = CARDS_PER_SIDE; // page 0 right showed first 25
			else
				card_start = CARDS_PER_SIDE + (cur_page - 1) * CARDS_PER_PAGE;

			// Left panel cards
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t idx = card_start + i;
				if (idx < static_cast<int16_t>(filtered_cards.size()))
					draw_card(left_page, idx);
			}

			// Right panel cards
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t idx = card_start + CARDS_PER_SIDE + i;
				if (idx < static_cast<int16_t>(filtered_cards.size()))
					draw_card(right_page, idx);
			}
		}

		// Card name on hover
		if (hovered_card >= 0 && hovered_card < static_cast<int16_t>(filtered_cards.size()))
		{
			auto& entry = filtered_cards[hovered_card];
			if (!entry.mob_name.empty())
				card_name_text.change_text(entry.mob_name);
			card_name_text.draw(right_page + Point<int16_t>(87, 270));
		}

		// Page text centered on right panel
		page_text.draw(right_page + Point<int16_t>(87, 290));

		UIElement::draw_buttons(inter);
	}

	void UIMonsterBook::draw_detail(float inter) const
	{
		UIElement::draw_sprites(inter);

		auto& entry = filtered_cards[selected_card];

		monster_info.draw(position);

		Point<int16_t> left_page = position + Point<int16_t>(50, 35);
		Point<int16_t> right_page = position + Point<int16_t>(250, 35);
		int16_t left_center_x = 87;

		// Left panel: monster sprite centered
		Point<int16_t> mob_draw = left_page + Point<int16_t>(left_center_x, 120);
		switch (detail_anim_state)
		{
		case 0: detail_stand.draw(DrawArgument(mob_draw), inter); break;
		case 1: detail_move.draw(DrawArgument(mob_draw), inter); break;
		case 2: detail_die.draw(DrawArgument(mob_draw), inter); break;
		}

		// Monster name in black below sprite, above HP/MP
		card_name_text.change_text(entry.mob_name);
		card_name_text.draw(left_page + Point<int16_t>(left_center_x, 175));

		// HP number in red
		hp_text.change_text(std::to_string(entry.mob_hp));
		hp_text.draw(left_page + Point<int16_t>(left_center_x, 198));

		// MP number in blue
		mp_text.change_text(std::to_string(entry.mob_mp));
		mp_text.draw(left_page + Point<int16_t>(left_center_x, 216));

		// Right panel drops - cubes are baked into monster_info (52x52 cells)
		// monster_info screen topleft = position + (45, 32)
		// Cube grid on the right side of the sprite
		Point<int16_t> sprite_topleft = position + Point<int16_t>(45, 32);
		Point<int16_t> drop_area = sprite_topleft + Point<int16_t>(210, 133);
		int16_t drop_cell = 40;
		int16_t drop_cols = 4;
		int16_t visible_rows = 3;

		if (!entry.drops.empty())
		{
			int16_t total_rows = static_cast<int16_t>((entry.drops.size() + drop_cols - 1) / drop_cols);
			int16_t max_offset = std::max(static_cast<int16_t>(0), static_cast<int16_t>(total_rows - visible_rows));
			int16_t scroll = std::min(detail_drop_offset, max_offset);

			for (int16_t r = scroll; r < total_rows && r < scroll + visible_rows; r++)
			{
				for (int16_t c = 0; c < drop_cols; c++)
				{
					size_t idx = r * drop_cols + c;
					if (idx >= entry.drops.size())
						break;

					// Center item in cube cell (icon origin is typically 0,32)
					int16_t dx = c * drop_cell + 22;
					int16_t dy = (r - scroll) * drop_cell + 22;
					Point<int16_t> item_pos = drop_area + Point<int16_t>(dx, dy);

					if (hovered_drop == static_cast<int16_t>(idx))
						entry.drops[idx].second.draw(DrawArgument(item_pos, Color(1.3f, 1.3f, 1.3f, 1.0f)));
					else
						entry.drops[idx].second.draw(DrawArgument(item_pos, 0.8f, 0.8f));
				}
			}
		}

		UIElement::draw_buttons(inter);
	}

	void UIMonsterBook::load_sets()
	{
		// v83-compatible card sets (map area based)
		// region: 0=Victoria, 1=Orbis, 2=ElNath, 3=Ludibrium, 4=MuLung, 5=Leafre, 6=Nihal, 7=Japan
		struct SetDef { const char* name; const char* bonus; int8_t region; std::vector<int32_t> mobs; };
		static const SetDef defs[] = {
			{"Victoria Island", "+50 HP", 0, {100100, 100101, 120100, 130100}},
			{"Kerning City", "+20 Avoid", 0, {2230100, 2230101, 2230102, 2230103, 2230104, 2230105, 2230106, 2230107, 2230108, 2230109, 2230110}},
			{"Ellinia", "+50 MP", 0, {210100, 100130, 100131, 100132, 100133, 100134, 1210100, 1210101, 1210102, 1210103}},
			{"Sleepywood", "+5 ATK, +50 DEF", 0, {4130100, 4130101, 4130102, 4130103, 4130104, 4130105, 6130100, 6130101, 6130102}},
			{"Perion", "+50 HP", 0, {130101, 1110100, 1110101, 1110130, 1120100, 1130100, 1140100, 1140130, 2100100, 2100101, 2100102, 2100103, 2100104, 2100105, 2100106, 2100107, 2100108}},
			{"Henesys", "+20 ACC", 0, {1210100, 2110200, 2110300, 2110301, 2130100, 2130103, 3000000, 3000001, 3000005, 3000006, 3100101, 3100102}},
			{"Orbis", "+3 Speed, +10 Jump", 1, {3210100, 3210200, 3210201, 3210202, 3210203, 3210204, 3210205, 3210206, 3210207, 3210208}},
			{"El Nath", "+5 ATK, +10 DEX", 2, {5100100, 5100101, 5100102, 5100103, 5100104, 5105100, 5105101, 5105102, 5120100, 5120101}},
			{"Dead Mine", "+6 ATK%, +10 MATK", 2, {5130100, 5130101, 5130102, 5130103, 5130104, 5130105, 5130106, 5130107, 5130108}},
			{"Ludibrium", "+100 HP, +100 DEF", 3, {3210450, 3210800, 3220000, 3230100, 3230101, 3230102, 3230200, 3230300, 3230301, 3230302}},
			{"Aqua Road", "+6 Magic%, +100 MDEF", 3, {2230131, 2230200, 2300100, 3110100, 3110101, 3110102, 3110300, 3110301, 3110302, 3110303}},
			{"Mu Lung", "+15% Ignore DEF", 4, {5090100, 6090000, 6090001, 6090002, 6090003, 6090004, 6130200, 6230100, 6230200, 6300100}},
			{"Leafre", "+1 All Skills", 5, {8140100, 8140101, 8140102, 8140103, 8140104, 8140105, 8140110, 8140111, 8140112, 8143000}},
			{"Temple of Time", "+47 HP/MP Recovery", 5, {8200000, 8200001, 8200002, 8200003, 8200004, 8200005, 8200006, 8200007, 8200008, 8200009}},
			{"Nihal Desert", "+50 DEF, +50 MDEF", 6, {7130000, 7130001, 7130002, 7130003, 7130004, 7130005, 7130100, 7130101, 7130200, 7130300}},
			{"Magatia", "+10 INT, +5 MATK", 6, {7130400, 7130401, 7130402, 7130500, 7130501, 7130600, 7130601, 7140000, 7140100}},
			{"Mushroom Shrine", "+85 MP Recovery", 7, {9400000, 9400001, 9400002, 9400003, 9400004, 9400005, 9400006, 9400007, 9400008}},
			{"Ninja Castle", "+6 Speed, +6 Jump", 7, {9400100, 9400101, 9400102, 9400103, 9400104, 9400105, 9400106}},
			{"Showa Village", "+5 All Stats", 7, {9400110, 9400111, 9400112, 9400113, 9400114, 9400115, 9400120, 9400121}},
		};

		card_sets.clear();

		// Build a lookup: mobid -> whether player has collected it
		std::unordered_map<int32_t, bool> collected_mobs;
		for (auto& entry : sorted_cards)
		{
			if (entry.mobid > 0)
				collected_mobs[entry.mobid] = true;
		}

		for (auto& def : defs)
		{
			CardSet cs;
			cs.name = def.name;
			cs.bonus = def.bonus;
			cs.mob_ids = def.mobs;
			cs.total = static_cast<int32_t>(def.mobs.size());
			cs.collected = 0;
			cs.region = def.region;

			for (int32_t mobid : def.mobs)
			{
				if (collected_mobs.count(mobid))
					cs.collected++;
			}

			// Load icon: try collected card_icon first
			for (int32_t mobid : def.mobs)
			{
				for (auto& entry : sorted_cards)
				{
					if (entry.mobid == mobid && entry.card_icon.is_valid())
					{
						cs.icon = entry.card_icon;
						break;
					}
				}
				if (cs.icon.is_valid()) break;
			}

			// Fallback: search ALL consume .img files for card iconRaw
			if (!cs.icon.is_valid())
			{
				nl::node consume_root = nl::nx::item["Consume"];
				for (int32_t mobid : def.mobs)
				{
					for (auto img : consume_root)
					{
						for (auto item : img)
						{
							nl::node info = item["info"];
							if (!info) continue;
							nl::node mob_node = info["mob"];
							if (!mob_node) continue;
							if (static_cast<int32_t>(mob_node) == mobid)
							{
								nl::node icon_node = info["iconRaw"];
								if (icon_node)
									cs.icon = Texture(icon_node);
								break;
							}
						}
						if (cs.icon.is_valid()) break;
					}
					if (cs.icon.is_valid()) break;
				}
			}

			// Final fallback: mob stand sprite
			if (!cs.icon.is_valid() && !def.mobs.empty())
			{
				std::string mob_strid = string_format::extend_id(def.mobs[0], 7);
				nl::node mob_src = nl::nx::mob[mob_strid + ".img"];
				nl::node stand = mob_src["stand"];
				if (stand && stand["0"])
					cs.icon = Texture(stand["0"]);
				else if (mob_src["fly"] && mob_src["fly"]["0"])
					cs.icon = Texture(mob_src["fly"]["0"]);
			}

			card_sets.push_back(cs);
		}
	}

	void UIMonsterBook::draw_set_effect(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Left page background with baked-in labels: "Set List", "Set score", "Points", "Selected set"
		set_info0.draw(position);

		// Right page header with "Complete Set" label
		set_info1.draw(position);

		Point<int16_t> left_page = position + Point<int16_t>(50, 35);
		Point<int16_t> right_page = position + Point<int16_t>(250, 35);
		int16_t left_center_x = 87;

		// Calculate totals
		int32_t score = 0;
		int32_t complete_count = 0;
		for (auto& cs : card_sets)
		{
			score += cs.collected * 10;
			if (cs.total > 0 && cs.collected >= cs.total)
				complete_count++;
		}

		// Score number below "Set score" label, centered on left page
		int16_t score_w = number_width(score, true);
		draw_number(left_page + Point<int16_t>(left_center_x - score_w / 2, 100), score, true);

		// "Points" is baked into setInfo0 — no text needed

		// Selected set display below "Selected set" label
		if (active_set >= 0 && active_set < static_cast<int16_t>(card_sets.size()))
		{
			auto& active = card_sets[active_set];

			// setIconBack cell centered on left page
			Point<int16_t> icon_cell = left_page + Point<int16_t>(left_center_x - 26, 160);
			set_icon_back.draw(icon_cell);

			// Card icon centered in cell
			if (active.icon.is_valid())
				active.icon.draw(DrawArgument(icon_cell + Point<int16_t>(26, 26)));

			// Set name below cell
			card_name_text.change_text(active.name);
			card_name_text.draw(left_page + Point<int16_t>(left_center_x, 220));
		}

		// Right page: "Complete Set X / Y" count using small number sprites
		// setInfo1 draws the "Complete Set" label; numbers go to its right
		draw_count(right_page + Point<int16_t>(108, 3), complete_count, static_cast<int32_t>(card_sets.size()), false);

		// Right page grid: 3 columns of setIconBack cells
		int16_t grid_cols = 3;
		int16_t cell_size = 52;   // setIconBack is 52x52
		int16_t cell_gap_x = 7;
		int16_t cell_gap_y = 16;  // space for count text below cell
		int16_t cell_stride_x = cell_size + cell_gap_x;
		int16_t cell_stride_y = cell_size + cell_gap_y;
		int16_t grid_width = grid_cols * cell_size + (grid_cols - 1) * cell_gap_x;
		int16_t grid_x_offset = (175 - grid_width) / 2;
		int16_t grid_start_y = 22;
		int16_t max_visible_rows = 3;

		int16_t total_sets = static_cast<int16_t>(card_sets.size());
		int16_t total_rows = (total_sets + grid_cols - 1) / grid_cols;

		for (int16_t r = set_scroll; r < total_rows && r < set_scroll + max_visible_rows; r++)
		{
			for (int16_t c = 0; c < grid_cols; c++)
			{
				int16_t idx = r * grid_cols + c;
				if (idx >= total_sets)
					break;

				auto& cs = card_sets[idx];
				Point<int16_t> cell_pos = right_page + Point<int16_t>(
					grid_x_offset + c * cell_stride_x,
					grid_start_y + (r - set_scroll) * cell_stride_y
				);

				// Gold cell background
				set_icon_back.draw(cell_pos);

				// Card icon centered in 52x52 cell
				if (cs.icon.is_valid())
				{
					Point<int16_t> icon_center = cell_pos + Point<int16_t>(26, 26);
					if (hovered_set == idx)
						cs.icon.draw(DrawArgument(icon_center, Color(1.3f, 1.3f, 1.3f, 1.0f)));
					else
						cs.icon.draw(DrawArgument(icon_center));
				}

				// Collected/total count below cell using small number sprites, centered
				int16_t count_w = number_width(cs.collected, false) + 2 + num_small[10].get_dimensions().x() + 2 + number_width(cs.total, false);
				Point<int16_t> count_pos = cell_pos + Point<int16_t>((cell_size - count_w) / 2, cell_size + 2);
				draw_count(count_pos, cs.collected, cs.total, false);
			}
		}

		UIElement::draw_buttons(inter);
	}

	void UIMonsterBook::select_card(int16_t idx)
	{
		if (idx < 0 || idx >= static_cast<int16_t>(filtered_cards.size()))
		{
			selected_card = -1;
			return;
		}

		selected_card = idx;
		detail_anim_state = 0;
		detail_drop_offset = 0;

		auto& entry = filtered_cards[idx];

		if (entry.mobid > 0)
		{
			std::string mob_strid = string_format::extend_id(entry.mobid, 7);
			nl::node mob_src = nl::nx::mob[mob_strid + ".img"];

			nl::node stand_node = mob_src["stand"];
			nl::node move_node = mob_src["move"];
			nl::node die_node = mob_src["die1"];
			nl::node fly_node = mob_src["fly"];

			if (stand_node)
				detail_stand = Animation(stand_node);
			else if (fly_node)
				detail_stand = Animation(fly_node);
			else
				detail_stand = Animation();

			detail_move = move_node ? Animation(move_node) : detail_stand;
			detail_die = die_node ? Animation(die_node) : Animation();

			// Load drops if not already loaded
			if (entry.drops.empty())
			{
				nl::node reward = nl::nx::string["MonsterBook.img"][std::to_string(entry.mobid)]["reward"];
				if (reward)
				{
					for (auto drop : reward)
					{
						int32_t itemid = drop;
						if (itemid > 0)
						{
							const ItemData& idata = ItemData::get(itemid);
							if (idata.is_valid())
								entry.drops.push_back({ itemid, idata.get_icon(false) });
						}
					}
				}
			}
		}
	}

	void UIMonsterBook::update()
	{
		UIElement::update();

		if (selected_card >= 0)
		{
			switch (detail_anim_state)
			{
			case 0: detail_stand.update(); break;
			case 1: detail_move.update(); break;
			case 2:
				if (detail_die.update())
				{
					detail_anim_state = 0;
					detail_die.reset();
				}
				break;
			}
		}
	}

	int16_t UIMonsterBook::card_at_cursor(Point<int16_t> cursorpos) const
	{
		int16_t card_w = 35;
		int16_t card_h = 43;
		Point<int16_t> card_grid_offset = Point<int16_t>(0, 50);

		Point<int16_t> left_page = position + Point<int16_t>(50, 27);
		Point<int16_t> right_page = position + Point<int16_t>(247, 27);

		auto check_side = [&](Point<int16_t> page_origin, int16_t base_idx) -> int16_t
		{
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t col = i % COLS_PER_SIDE;
				int16_t row = i / COLS_PER_SIDE;
				Point<int16_t> card_topleft = page_origin + card_grid_offset + Point<int16_t>(col * card_w, row * card_h);

				Rectangle<int16_t> bounds(card_topleft.x(), card_topleft.x() + 31, card_topleft.y(), card_topleft.y() + 42);

				if (bounds.contains(cursorpos))
				{
					int16_t idx = base_idx + i;
					if (idx < static_cast<int16_t>(filtered_cards.size()))
						return idx;
				}
			}
			return -1;
		};

		if (cur_page == 0)
		{
			// Only right page has cards on cover page
			return check_side(right_page, 0);
		}
		else
		{
			int16_t card_start;
			if (cur_page == 1)
				card_start = CARDS_PER_SIDE;
			else
				card_start = CARDS_PER_SIDE + (cur_page - 1) * CARDS_PER_PAGE;

			int16_t result = check_side(left_page, card_start);
			if (result >= 0)
				return result;

			return check_side(right_page, card_start + CARDS_PER_SIDE);
		}
	}

	Cursor::State UIMonsterBook::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		// Handle tab clicks manually
		if (clicked)
		{
			auto drawpos = get_draw_position();

			// Big tabs (0, 1, 2) - view switching
			for (uint16_t t = 0; t <= 2; t++)
			{
				auto& btn = buttons[Buttons::BT_TAB0 + t];
				if (btn && btn->is_active() && btn->bounds(drawpos).contains(cursorpos))
				{
					if (t == 1)
					{
						show_set_page = !show_set_page;
						selected_card = -1;
					}
					else
					{
						show_set_page = false;
						selected_card = -1;
					}
					buttons[Buttons::BT_BACK]->set_active(show_set_page);
					return Cursor::State::CLICKING;
				}
			}

			// Small left tabs (3-8) - category filtering
			for (uint16_t t = 3; t <= 8; t++)
			{
				auto& btn = buttons[Buttons::BT_TAB0 + t];
				if (btn && btn->is_active() && btn->bounds(drawpos).contains(cursorpos))
				{
					show_set_page = false;
					selected_card = -1;
					buttons[Buttons::BT_BACK]->set_active(false);
					int16_t new_tab = static_cast<int16_t>(t);
					if (new_tab == cur_tab)
						set_tab(-1);
					else
						set_tab(new_tab);
					return Cursor::State::CLICKING;
				}
			}

			// Right tabs (0-8) - category filtering
			for (uint16_t t = 0; t <= 8; t++)
			{
				auto& btn = buttons[Buttons::BT_RTAB0 + t];
				if (btn && btn->is_active() && btn->bounds(drawpos).contains(cursorpos))
				{
					show_set_page = false;
					selected_card = -1;
					buttons[Buttons::BT_BACK]->set_active(false);
					int16_t new_tab = static_cast<int16_t>(t);
					if (new_tab == cur_tab)
						set_tab(-1);
					else
						set_tab(new_tab);
					return Cursor::State::CLICKING;
				}
			}
		}

		// Set effect page hover/click
		if (show_set_page)
		{
			hovered_set = -1;

			// Match grid layout from draw_set_effect
			Point<int16_t> right_page = position + Point<int16_t>(250, 35);
			int16_t grid_cols = 3;
			int16_t cell_size = 52;
			int16_t cell_gap_x = 7;
			int16_t cell_gap_y = 16;
			int16_t cell_stride_x = cell_size + cell_gap_x;
			int16_t cell_stride_y = cell_size + cell_gap_y;
			int16_t grid_width = grid_cols * cell_size + (grid_cols - 1) * cell_gap_x;
			int16_t grid_x_offset = (175 - grid_width) / 2;
			int16_t grid_start_y = 22;
			int16_t max_visible_rows = 3;
			int16_t total_sets = static_cast<int16_t>(card_sets.size());
			int16_t total_rows = (total_sets + grid_cols - 1) / grid_cols;

			for (int16_t r = set_scroll; r < total_rows && r < set_scroll + max_visible_rows; r++)
			{
				for (int16_t c = 0; c < grid_cols; c++)
				{
					int16_t idx = r * grid_cols + c;
					if (idx >= total_sets)
						break;

					Point<int16_t> cell_pos = right_page + Point<int16_t>(
						grid_x_offset + c * cell_stride_x,
						grid_start_y + (r - set_scroll) * cell_stride_y
					);
					Rectangle<int16_t> cell_rect(cell_pos.x(), cell_pos.x() + cell_size, cell_pos.y(), cell_pos.y() + cell_size);

					if (cell_rect.contains(cursorpos))
						hovered_set = idx;
				}
			}

			Cursor::State btn_state = UIElement::send_cursor(clicked, cursorpos);

			if (clicked && hovered_set >= 0 && hovered_set < total_sets)
			{
				if (active_set == hovered_set)
					active_set = -1;
				else
					active_set = hovered_set;
				return Cursor::State::CLICKING;
			}

			return btn_state;
		}

		int16_t card_idx = card_at_cursor(cursorpos);
		hovered_card = card_idx;

		if (selected_card >= 0)
		{
			// In detail view: detect hovered drop item
			int16_t prev_hovered = hovered_drop;
			hovered_drop = -1;
			auto& entry = filtered_cards[selected_card];
			if (!entry.drops.empty())
			{
				Point<int16_t> sprite_topleft = position + Point<int16_t>(45, 32);
				Point<int16_t> drop_area = sprite_topleft + Point<int16_t>(210, 133);
				int16_t drop_cell = 40;
				int16_t drop_cols = 4;
				int16_t visible_rows = 3;
				int16_t total_rows = static_cast<int16_t>((entry.drops.size() + drop_cols - 1) / drop_cols);
				int16_t max_offset = std::max(static_cast<int16_t>(0), static_cast<int16_t>(total_rows - visible_rows));
				int16_t scroll = std::min(detail_drop_offset, max_offset);

				for (int16_t r = scroll; r < total_rows && r < scroll + visible_rows; r++)
				{
					for (int16_t c = 0; c < drop_cols; c++)
					{
						size_t idx = r * drop_cols + c;
						if (idx >= entry.drops.size())
							break;

						int16_t cx = drop_area.x() + c * drop_cell;
						int16_t cy = drop_area.y() + (r - scroll) * drop_cell;
						Rectangle<int16_t> cell_rect(cx, cx + drop_cell, cy, cy + drop_cell);

						if (cell_rect.contains(cursorpos))
							hovered_drop = static_cast<int16_t>(idx);
					}
				}
			}

			// Show/hide item tooltip
			if (hovered_drop >= 0 && hovered_drop < static_cast<int16_t>(entry.drops.size()))
				UI::get().show_item(Tooltip::Parent::ITEMINVENTORY, entry.drops[hovered_drop].first);
			else if (prev_hovered >= 0)
				UI::get().clear_tooltip(Tooltip::Parent::ITEMINVENTORY);

			// Let buttons handle first, then handle clicks
			Cursor::State btn_state = UIElement::send_cursor(clicked, cursorpos);

			if (clicked)
			{
				Point<int16_t> left_page = position + Point<int16_t>(50, 27);
				Rectangle<int16_t> mob_area(left_page.x() + 30, left_page.x() + 144, left_page.y() + 40, left_page.y() + 170);

				if (mob_area.contains(cursorpos))
				{
					detail_anim_state = (detail_anim_state + 1) % 3;
					if (detail_anim_state == 0) detail_stand.reset();
					else if (detail_anim_state == 1) detail_move.reset();
					else detail_die.reset();
					return Cursor::State::CLICKING;
				}
				else
				{
					select_card(-1);
					return Cursor::State::CLICKING;
				}
			}

			return btn_state;
		}

		if (clicked && card_idx >= 0)
		{
			select_card(card_idx);
			return Cursor::State::CLICKING;
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIMonsterBook::send_scroll(double yoffset)
	{
		if (show_set_page)
		{
			int16_t grid_cols = 3;
			int16_t visible_rows = 3;
			int16_t total_sets = static_cast<int16_t>(card_sets.size());
			int16_t total_rows = (total_sets + grid_cols - 1) / grid_cols;
			int16_t max_offset = std::max(0, total_rows - visible_rows);

			if (yoffset < 0)
				set_scroll = std::min(static_cast<int16_t>(set_scroll + 1), max_offset);
			else if (yoffset > 0)
				set_scroll = std::max(static_cast<int16_t>(set_scroll - 1), static_cast<int16_t>(0));
		}
		else if (selected_card >= 0 && selected_card < static_cast<int16_t>(filtered_cards.size()))
		{
			auto& entry = filtered_cards[selected_card];
			int16_t drop_cols = 4;
			int16_t visible_rows = 3;
			int16_t total_rows = static_cast<int16_t>((entry.drops.size() + drop_cols - 1) / drop_cols);
			int16_t max_offset = std::max(0, (total_rows - visible_rows));

			if (yoffset < 0)
				detail_drop_offset = std::min(static_cast<int16_t>(detail_drop_offset + 1), max_offset);
			else if (yoffset > 0)
				detail_drop_offset = std::max(static_cast<int16_t>(detail_drop_offset - 1), static_cast<int16_t>(0));
		}
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

	void UIMonsterBook::update_card(int32_t cardid, int8_t level)
	{
		load_cards();
		set_tab(cur_tab);
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
			UI::get().emplace<UIEnterText>(
				"Enter monster card name:",
				[this](const std::string& query)
				{
					if (query.empty())
						return;

					std::string lower_query = query;
					std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

					for (int16_t i = 0; i < static_cast<int16_t>(filtered_cards.size()); i++)
					{
						std::string name = filtered_cards[i].mob_name;
						if (name.empty())
						{
							const ItemData& idata = ItemData::get(filtered_cards[i].cardid);
							if (idata.is_valid())
								name = idata.get_name();
						}

						std::transform(name.begin(), name.end(), name.begin(), ::tolower);

						if (name.find(lower_query) != std::string::npos)
						{
							int16_t page = (i / CARDS_PER_PAGE) + 1;
							set_page(page);
							return;
						}
					}

					UI::get().emplace<UIOk>("No matching card found.", [](bool) {});
				},
				20
			);
			break;
		case Buttons::BT_SETEFFECT:
			show_set_page = !show_set_page;
			buttons[Buttons::BT_BACK]->set_active(show_set_page);
			return Button::State::NORMAL;
		case Buttons::BT_BACK:
			show_set_page = false;
			buttons[Buttons::BT_BACK]->set_active(false);
			return Button::State::NORMAL;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
		case Buttons::BT_TAB5:
		case Buttons::BT_TAB6:
		case Buttons::BT_TAB7:
		case Buttons::BT_TAB8:
		{
			int16_t new_tab = buttonid - Buttons::BT_TAB0;

			if (new_tab == cur_tab)
				set_tab(-1);
			else
				set_tab(new_tab);

			return Button::State::NORMAL;
		}
		case Buttons::BT_RTAB0:
		case Buttons::BT_RTAB1:
		case Buttons::BT_RTAB2:
		case Buttons::BT_RTAB3:
		case Buttons::BT_RTAB4:
		case Buttons::BT_RTAB5:
		case Buttons::BT_RTAB6:
		case Buttons::BT_RTAB7:
		case Buttons::BT_RTAB8:
		{
			int16_t new_tab = buttonid - Buttons::BT_RTAB0;

			if (new_tab == cur_tab)
				set_tab(-1);
			else
				set_tab(new_tab);

			return Button::State::NORMAL;
		}
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIMonsterBook::set_page(int16_t page)
	{
		cur_page = page;
		hovered_card = -1;
		selected_card = -1;
		page_text.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
		update_buttons();
	}

	void UIMonsterBook::update_buttons()
	{
		// Hide left arrow on first page
		buttons[Buttons::BT_ARROW_LEFT]->set_active(cur_page > 0);
		if (cur_page > 0)
			buttons[Buttons::BT_ARROW_LEFT]->set_state(Button::State::NORMAL);

		if (cur_page >= num_pages - 1)
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::NORMAL);

		buttons[Buttons::BT_SETEFFECT]->set_active(true);
	}

	void UIMonsterBook::draw_number(Point<int16_t> pos, int32_t number, bool large) const
	{
		std::string numstr = std::to_string(number);

		for (char c : numstr)
		{
			int digit = c - '0';

			if (large)
			{
				num_large[digit].draw(pos);
				pos.shift_x(num_large[digit].get_dimensions().x());
			}
			else
			{
				num_small[digit].draw(pos);
				pos.shift_x(num_small[digit].get_dimensions().x());
			}
		}
	}

	int16_t UIMonsterBook::number_width(int32_t number, bool large) const
	{
		std::string numstr = std::to_string(number);
		int16_t w = 0;

		for (char c : numstr)
		{
			int digit = c - '0';
			if (large)
				w += num_large[digit].get_dimensions().x();
			else
				w += num_small[digit].get_dimensions().x();
		}

		return w;
	}

	void UIMonsterBook::draw_count(Point<int16_t> pos, int32_t num, int32_t den, bool large) const
	{
		// Draw numerator
		draw_number(pos, num, large);

		std::string numstr = std::to_string(num);
		for (char c : numstr)
		{
			int digit = c - '0';
			if (large)
				pos.shift_x(num_large[digit].get_dimensions().x());
			else
				pos.shift_x(num_small[digit].get_dimensions().x());
		}

		// "/" separator (num_small[10])
		pos.shift_x(2);
		num_small[10].draw(pos);
		pos.shift_x(num_small[10].get_dimensions().x() + 2);

		// Draw denominator
		draw_number(pos, den, large);
	}
}
