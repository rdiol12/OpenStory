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
#include "../../Graphics/Geometry.h"

#include <algorithm>
#include <iostream>

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

		std::cout << "[MonsterBook] bg_dimensions: " << bg_dimensions.x() << "x" << bg_dimensions.y() << std::endl;

		// Main sprites
		cover = src["cover"];
		card_slot = src["cardSlot"];
		select_tex = src["select"];
		full_mark = src["fullMark"];
		info_page = src["infoPage"];

		std::cout << "[MonsterBook] cover: dim=" << cover.get_dimensions().x() << "x" << cover.get_dimensions().y()
			<< " origin=" << cover.get_origin().x() << "," << cover.get_origin().y() << std::endl;
		std::cout << "[MonsterBook] card_slot: dim=" << card_slot.get_dimensions().x() << "x" << card_slot.get_dimensions().y()
			<< " origin=" << card_slot.get_origin().x() << "," << card_slot.get_origin().y() << std::endl;
		std::cout << "[MonsterBook] info_page: dim=" << info_page.get_dimensions().x() << "x" << info_page.get_dimensions().y()
			<< " origin=" << info_page.get_origin().x() << "," << info_page.get_origin().y() << std::endl;

		// Page info sprites
		nl::node page_info = src["pageInfo"];
		book_info0 = page_info["bookInfo0"];
		book_info1 = page_info["bookInfo1"];
		monster_info = page_info["monsterInfo"];

		std::cout << "[MonsterBook] monster_info: dim=" << monster_info.get_dimensions().x() << "x" << monster_info.get_dimensions().y()
			<< " origin=" << monster_info.get_origin().x() << "," << monster_info.get_origin().y() << std::endl;
		std::cout << "[MonsterBook] book_info0: dim=" << book_info0.get_dimensions().x() << "x" << book_info0.get_dimensions().y()
			<< " origin=" << book_info0.get_origin().x() << "," << book_info0.get_origin().y() << std::endl;

		for (int i = 0; i < 10; i++)
			card_category[i] = page_info["cardCategory"][std::to_string(i)];

		// Level marks (0-4 = 1-5 cards collected)
		for (int i = 0; i < 5; i++)
			marks[i] = src["mark"][std::to_string(i)];

		// Number sprites for page display
		for (int i = 0; i < 10; i++)
			num_large[i] = src["numberLarge"][std::to_string(i)];

		for (int i = 0; i < 11; i++)
			num_small[i] = src["numberSmall"][std::to_string(i)];

		// Buttons
		// Background has origin (14,0), so bg left edge is at position.x - 14
		// BtClose: top-right corner of the background
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"], Point<int16_t>(bg_dimensions.x() - 14 - 19, 6));
		// Arrow buttons have origin (0,0) so need explicit positioning at bottom center
		buttons[Buttons::BT_ARROW_LEFT] = std::make_unique<MapleButton>(src["arrowLeft"], Point<int16_t>(185, 338));
		buttons[Buttons::BT_ARROW_RIGHT] = std::make_unique<MapleButton>(src["arrowRight"], Point<int16_t>(260, 338));
		// BtSearch and BtSetEffect use their NX negative origins for positioning
		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(src["BtSearch"]);
		buttons[Buttons::BT_SETEFFECT] = std::make_unique<MapleButton>(src["BtSetEffect"]);

		// Left tabs (9 category tabs by monster level range)
		nl::node left_tab = src["LeftTab"];

		for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB8; i++)
		{
			uint16_t tabid = i - Buttons::BT_TAB0;
			nl::node tab_node = left_tab[std::to_string(tabid)];

			if (tab_node.size() > 0)
			{
				// Use normal/selected states for TwoSpriteButton
				nl::node normal_node = tab_node["normal"]["0"];
				nl::node selected_node = tab_node["selected"]["0"];

				if (normal_node.size() > 0 && selected_node.size() > 0)
					buttons[i] = std::make_unique<TwoSpriteButton>(normal_node, selected_node);
				else
					buttons[i] = std::make_unique<MapleButton>(tab_node);
			}
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

		// Load HP/MP gauge sprites from status bar
		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];
		nl::node gauge_src = mainbar["gauge"];

		std::cout << "[MonsterBook] gauge/hp node size: " << gauge_src["hp"].size() << std::endl;
		std::cout << "[MonsterBook] gauge/mp node size: " << gauge_src["mp"].size() << std::endl;

		Texture hp0(gauge_src["hp"]["0"]), hp1(gauge_src["hp"]["1"]), hp2(gauge_src["hp"]["2"]);
		Texture mp0(gauge_src["mp"]["0"]), mp1(gauge_src["mp"]["1"]), mp2(gauge_src["mp"]["2"]);

		std::cout << "[MonsterBook] hp gauge sprites: "
			<< "0=" << hp0.get_dimensions().x() << "x" << hp0.get_dimensions().y()
			<< " 1=" << hp1.get_dimensions().x() << "x" << hp1.get_dimensions().y()
			<< " 2=" << hp2.get_dimensions().x() << "x" << hp2.get_dimensions().y() << std::endl;
		std::cout << "[MonsterBook] mp gauge sprites: "
			<< "0=" << mp0.get_dimensions().x() << "x" << mp0.get_dimensions().y()
			<< " 1=" << mp1.get_dimensions().x() << "x" << mp1.get_dimensions().y()
			<< " 2=" << mp2.get_dimensions().x() << "x" << mp2.get_dimensions().y() << std::endl;

		detail_hp_gauge = Gauge(Gauge::Type::GAME, gauge_src["hp"]["0"], gauge_src["hp"]["1"], gauge_src["hp"]["2"], 120, 1.0f);
		detail_mp_gauge = Gauge(Gauge::Type::GAME, gauge_src["mp"]["0"], gauge_src["mp"]["1"], gauge_src["mp"]["2"], 120, 1.0f);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		load_cards();
		set_tab(-1); // Show all cards initially
		update_buttons();
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
			// Cards (238xxxx) are prefix 2 = "Consume" category
			std::string strprefix = "0" + std::to_string(cardid / 10000);
			std::string strid = "0" + std::to_string(cardid);
			nl::node card_info = nl::nx::item["Consume"][strprefix + ".img"][strid]["info"];

			if (card_info)
				entry.mobid = card_info["mob"];

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

				std::cout << "[MonsterBook] card=" << entry.cardid << " mob=" << entry.mobid
					<< " icon_valid=" << entry.mob_icon.is_valid()
					<< " icon_dim=" << entry.mob_icon.get_dimensions().x() << "x" << entry.mob_icon.get_dimensions().y()
					<< " icon_origin=" << entry.mob_icon.get_origin().x() << "," << entry.mob_icon.get_origin().y()
					<< std::endl;

				nl::node name_node = nl::nx::string["Mob.img"][std::to_string(entry.mobid)];
				if (name_node["name"])
					entry.mob_name = (std::string)name_node["name"];

				// Load mob stats from NX info
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

		// Update stats text
		auto& mb = Stage::get().get_player().get_monsterbook();
		card_count_text.change_text(std::to_string(mb.get_total_cards()));
		book_level_text.change_text("Lv. " + std::to_string(mb.get_book_level()));
		normal_count_text.change_text("Normal: " + std::to_string(mb.get_normal_cards()));
		special_count_text.change_text("Special: " + std::to_string(mb.get_special_cards()));
	}

	void UIMonsterBook::set_tab(int16_t tab)
	{
		// Deselect previous tab
		if (cur_tab >= 0 && cur_tab <= 8)
			buttons[Buttons::BT_TAB0 + cur_tab]->set_state(Button::State::NORMAL);

		cur_tab = tab;

		// Select new tab
		if (cur_tab >= 0 && cur_tab <= 8)
			buttons[Buttons::BT_TAB0 + cur_tab]->set_state(Button::State::PRESSED);

		// Filter cards by tab category
		filtered_cards.clear();

		if (cur_tab < 0)
		{
			// Show all cards
			filtered_cards = sorted_cards;
		}
		else
		{
			// Filter by card ID range (rough level categories)
			// Tab 8 = special (cardid / 1000 >= 2388 when full_id = 2380000 + cardid)
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
					// Split normal cards into 8 roughly equal groups by card ID offset
					// Card IDs are 238xxxx, so offset = cardid - 2380000
					int32_t offset = entry.cardid - 2380000;
					int32_t group = offset / 1000; // rough grouping

					if (group > 7) group = 7;
					if (group < 0) group = 0;

					matches = (group == cur_tab);
				}

				if (matches)
					filtered_cards.push_back(entry);
			}
		}

		int16_t total = static_cast<int16_t>(filtered_cards.size());
		num_pages = (total > 0) ? ((total - 1) / CARDS_PER_PAGE + 1) + 1 : 1; // +1 for cover page

		cur_page = 0;
		page_text.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
		update_buttons();
	}

	void UIMonsterBook::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		static bool draw_logged = false;
		if (!draw_logged)
		{
			std::cout << "[MonsterBook] draw() position=" << position.x() << "," << position.y()
				<< " dimension=" << dimension.x() << "x" << dimension.y() << std::endl;
			draw_logged = true;
		}

		// Background origin is (14, 0) — bg left edge is 14px left of position
		// Positions derived from NX button origins:
		//   BtSetEffect at bg(108,283) → left page center ≈ bg x=151
		//   BtSearch at bg(408,32) → right page area
		//   book_info0 origin(-42,-26) → draw at position places at bg(56,26)
		// Left cardSlot:  bg x=64, y=27 → position + (50, 27)
		// Right cardSlot: bg x=261, y=27 → position + (247, 27)
		Point<int16_t> left_page = position + Point<int16_t>(50, 27);
		Point<int16_t> right_page = position + Point<int16_t>(247, 27);

		// Cover origin offset — cover/select have origin (2, 40)
		Point<int16_t> card_origin = cover.get_origin();

		// bg visual center relative to position
		int16_t bg_center_x = (dimension.x() - 14) / 2;

		if (cur_page == 0)
		{
			// Cover page - draw cardSlot backgrounds
			card_slot.draw(left_page);
			card_slot.draw(right_page);

			// book_info0 has negative origin (-42,-26) — drawing at position
			// auto-places it at the correct left page position
			book_info0.draw(position);

			// Draw stats on right page
			Point<int16_t> rp_center = right_page + Point<int16_t>(87, 0);
			card_count_text.draw(rp_center + Point<int16_t>(0, 80));
			book_level_text.draw(rp_center + Point<int16_t>(0, 100));
			normal_count_text.draw(right_page + Point<int16_t>(20, 130));
			special_count_text.draw(right_page + Point<int16_t>(20, 150));
		}
		else
		{
			// Card pages - draw cardSlot backgrounds
			card_slot.draw(left_page);
			card_slot.draw(right_page);

			int16_t card_start = (cur_page - 1) * CARDS_PER_PAGE;
			int16_t card_w = 35;
			int16_t card_h = 43;

			auto draw_card = [&](Point<int16_t> page_origin, int16_t card_index)
			{
				if (card_index >= static_cast<int16_t>(filtered_cards.size()))
					return;

				int16_t local = card_index % CARDS_PER_SIDE;
				int16_t col = local % COLS_PER_SIDE;
				int16_t row = local / COLS_PER_SIDE;

				// Card top-left position within the page
				Point<int16_t> card_topleft = page_origin + Point<int16_t>(col * card_w, row * card_h);

				// Draw position adds origin so sprite renders with top-left at card_topleft
				Point<int16_t> draw_pos = card_topleft + card_origin;

				auto& entry = filtered_cards[card_index];

				// Draw card background frame
				cover.draw(draw_pos);

				// Draw mob sprite in card (card is 31x42)
				if (entry.mob_icon.is_valid())
				{
					// Scale mob to fit within card
					Point<int16_t> mob_dim = entry.mob_icon.get_dimensions();
					Point<int16_t> mob_origin = entry.mob_icon.get_origin();
					float sx = (mob_dim.x() > 0) ? std::min(1.0f, 27.0f / mob_dim.x()) : 0.35f;
					float sy = (mob_dim.y() > 0) ? std::min(1.0f, 35.0f / mob_dim.y()) : 0.35f;
					float scale = std::min(sx, sy);

					// Center the scaled sprite within the card
					int16_t scaled_w = static_cast<int16_t>(mob_dim.x() * scale);
					int16_t scaled_h = static_cast<int16_t>(mob_dim.y() * scale);
					int16_t ox = (31 - scaled_w) / 2;
					int16_t oy = (42 - scaled_h) / 2;

					// Draw position = where we want the top-left + origin offset for the sprite
					Point<int16_t> draw_pos_mob = card_topleft + Point<int16_t>(
						ox + static_cast<int16_t>(mob_origin.x() * scale),
						oy + static_cast<int16_t>(mob_origin.y() * scale)
					);

					entry.mob_icon.draw(DrawArgument(draw_pos_mob, draw_pos_mob, scale, scale, 1.0f));
				}
				else
				{
					Point<int16_t> card_center = card_topleft + Point<int16_t>(15, 21);
					const ItemData& idata = ItemData::get(entry.cardid);
					if (idata.is_valid())
						idata.get_icon(false).draw(DrawArgument(card_center));
				}

				// Draw level marks at bottom of card
				Point<int16_t> mark_pos = card_topleft + Point<int16_t>(12, 35);
				if (entry.level >= 5)
					full_mark.draw(mark_pos);
				else if (entry.level > 0)
					marks[entry.level - 1].draw(mark_pos);

				// Draw selection highlight on hover
				if (hovered_card == card_index)
					select_tex.draw(draw_pos);
			};

			// Draw left page cards
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t idx = card_start + i;
				if (idx < static_cast<int16_t>(filtered_cards.size()))
					draw_card(left_page, idx);
			}

			// Draw right page cards
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t idx = card_start + CARDS_PER_SIDE + i;
				if (idx < static_cast<int16_t>(filtered_cards.size()))
					draw_card(right_page, idx);
			}

			// Draw hovered card name at bottom
			if (hovered_card >= 0 && hovered_card < static_cast<int16_t>(filtered_cards.size()))
			{
				auto& entry = filtered_cards[hovered_card];
				if (!entry.mob_name.empty())
					card_name_text.change_text(entry.mob_name);
				card_name_text.draw(position + Point<int16_t>(bg_center_x, dimension.y() - 35));
			}
		}

		// Draw selected card info overlay
		if (selected_card >= 0 && selected_card < static_cast<int16_t>(filtered_cards.size()))
		{
			auto& entry = filtered_cards[selected_card];

			// monster_info has origin (-45,-32), drawing at position auto-places it
			monster_info.draw(position);

			Point<int16_t> info_pos = position + Point<int16_t>(45, 32);
			int16_t right_x = 198; // center of right panel

			// Monster sprite at top of right panel
			Point<int16_t> mob_draw = info_pos + Point<int16_t>(right_x, 80);
			switch (detail_anim_state)
			{
			case 0: detail_stand.draw(DrawArgument(mob_draw), inter); break;
			case 1: detail_move.draw(DrawArgument(mob_draw), inter); break;
			case 2: detail_die.draw(DrawArgument(mob_draw), inter); break;
			}

			// "Monster Drops" label - use the NX card_category sprite as decorative header
			mob_name_text.change_text("Monster Drops");
			mob_name_text.draw(info_pos + Point<int16_t>(right_x, 100));

			// HP bar with label and number
			mob_stat_text.change_text("HP");
			mob_stat_text.draw(info_pos + Point<int16_t>(160, 120));
			detail_hp_gauge.draw(DrawArgument(info_pos + Point<int16_t>(178, 118)));
			mob_name_text.change_text(std::to_string(entry.mob_hp));
			mob_name_text.draw(info_pos + Point<int16_t>(240, 120));

			// MP bar with label and number
			mob_stat_text.change_text("MP");
			mob_stat_text.draw(info_pos + Point<int16_t>(160, 138));
			detail_mp_gauge.draw(DrawArgument(info_pos + Point<int16_t>(178, 136)));
			mob_name_text.change_text(std::to_string(entry.mob_mp));
			mob_name_text.draw(info_pos + Point<int16_t>(240, 138));

			// Drop items in a 4-column grid below the bars
			int16_t drop_x0 = 160;
			int16_t drop_y0 = 158;
			int16_t drop_cell = 34;
			int16_t drop_cols = 4;
			for (size_t i = 0; i < entry.drops.size() && i < 16; i++)
			{
				int16_t dx = drop_x0 + static_cast<int16_t>(i % drop_cols) * drop_cell;
				int16_t dy = drop_y0 + static_cast<int16_t>(i / drop_cols) * drop_cell;
				entry.drops[i].second.draw(DrawArgument(info_pos + Point<int16_t>(dx, dy)));
			}

			// Stats on left side of right panel
			int16_t stat_x = 160;
			int16_t stat_y = 158 + (std::min(static_cast<int16_t>(entry.drops.size()), static_cast<int16_t>(16)) / drop_cols + 1) * drop_cell;
			int16_t line_h = 14;

			auto draw_stat = [&](const std::string& text) {
				mob_stat_text.change_text(text);
				mob_stat_text.draw(info_pos + Point<int16_t>(stat_x, stat_y));
				stat_y += line_h;
			};

			draw_stat("Level: " + std::to_string(entry.mob_level));
			draw_stat("W.Atk: " + std::to_string(entry.mob_watk));
			draw_stat("W.Def: " + std::to_string(entry.mob_wdef));
			draw_stat("M.Atk: " + std::to_string(entry.mob_matk));
			draw_stat("M.Def: " + std::to_string(entry.mob_mdef));
			draw_stat("EXP: " + std::to_string(entry.mob_exp));
		}

		// Page indicator at bottom center of book
		page_text.draw(position + Point<int16_t>(bg_center_x, dimension.y() - 18));

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

		auto& entry = filtered_cards[idx];
		std::cout << "[MonsterBook] select_card idx=" << idx << " cardid=" << entry.cardid
			<< " mobid=" << entry.mobid << " name=" << entry.mob_name
			<< " drops=" << entry.drops.size() << std::endl;

		if (entry.mobid > 0)
		{
			std::string mob_strid = string_format::extend_id(entry.mobid, 7);
			nl::node mob_src = nl::nx::mob[mob_strid + ".img"];

			// Load animations
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

			std::cout << "[MonsterBook] loaded animations: stand=" << (stand_node ? "yes" : "no")
				<< " move=" << (move_node ? "yes" : "no")
				<< " die=" << (die_node ? "yes" : "no")
				<< " fly=" << (fly_node ? "yes" : "no") << std::endl;

			// Load drops if not already loaded
			if (entry.drops.empty())
			{
				nl::node reward = nl::nx::string["MonsterBook.img"][std::to_string(entry.mobid)]["reward"];
				std::cout << "[MonsterBook] reward node exists=" << (reward ? "yes" : "no") << std::endl;
				if (reward)
				{
					for (auto drop : reward)
					{
						int32_t itemid = drop;
						if (itemid > 0)
						{
							const ItemData& idata = ItemData::get(itemid);
							if (idata.is_valid())
							{
								entry.drops.push_back({ itemid, idata.get_icon(false) });
								std::cout << "[MonsterBook] drop item: " << itemid << std::endl;
							}
						}
					}
					std::cout << "[MonsterBook] total drops loaded: " << entry.drops.size() << std::endl;
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
		if (cur_page == 0)
			return -1;

		int16_t card_start = (cur_page - 1) * CARDS_PER_PAGE;
		int16_t card_w = 35;
		int16_t card_h = 43;

		Point<int16_t> pages[2] = {
			position + Point<int16_t>(50, 27),   // left page
			position + Point<int16_t>(247, 27)   // right page
		};

		for (int side = 0; side < 2; side++)
		{
			for (int16_t i = 0; i < CARDS_PER_SIDE; i++)
			{
				int16_t col = i % COLS_PER_SIDE;
				int16_t row = i / COLS_PER_SIDE;
				Point<int16_t> card_topleft = pages[side] + Point<int16_t>(col * card_w, row * card_h);

				Rectangle<int16_t> bounds(card_topleft.x(), card_topleft.x() + 31, card_topleft.y(), card_topleft.y() + 42);

				if (bounds.contains(cursorpos))
				{
					int16_t idx = card_start + side * CARDS_PER_SIDE + i;
					if (idx < static_cast<int16_t>(filtered_cards.size()))
						return idx;
				}
			}
		}

		return -1;
	}

	Cursor::State UIMonsterBook::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		int16_t card_idx = card_at_cursor(cursorpos);
		hovered_card = card_idx;

		if (clicked && card_idx >= 0)
		{
			if (selected_card == card_idx)
				select_card(-1);
			else
				select_card(card_idx);

			return Cursor::State::CLICKING;
		}
		else if (clicked && selected_card >= 0)
		{
			// Check if clicking in the sprite/animation area to cycle animation
			Point<int16_t> info_pos = position + Point<int16_t>(45, 32);
			Rectangle<int16_t> anim_area(info_pos.x() + 220, info_pos.x() + 397, info_pos.y() + 25, info_pos.y() + 125);

			if (anim_area.contains(cursorpos))
			{
				detail_anim_state = (detail_anim_state + 1) % 3;
				if (detail_anim_state == 0) detail_stand.reset();
				else if (detail_anim_state == 1) detail_move.reset();
				else detail_die.reset();
				return Cursor::State::CLICKING;
			}

			// Check if clicking outside the monster_info overlay to close it
			Rectangle<int16_t> info_bounds(info_pos.x(), info_pos.x() + 397, info_pos.y(), info_pos.y() + 239);
			if (!info_bounds.contains(cursorpos))
				select_card(-1);
		}

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
		{
			int16_t new_tab = buttonid - Buttons::BT_TAB0;

			if (new_tab == cur_tab)
				set_tab(-1); // Toggle off = show all
			else
				set_tab(new_tab);

			return Button::State::PRESSED;
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
		if (cur_page <= 0)
			buttons[Buttons::BT_ARROW_LEFT]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::BT_ARROW_LEFT]->set_state(Button::State::NORMAL);

		if (cur_page >= num_pages - 1)
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::BT_ARROW_RIGHT]->set_state(Button::State::NORMAL);
	}

	void UIMonsterBook::draw_number(Point<int16_t> pos, int32_t number, bool large) const
	{
		std::string numstr = std::to_string(number);

		for (char c : numstr)
		{
			int digit = c - '0';

			if (large)
				num_large[digit].draw(pos);
			else
				num_small[digit].draw(pos);

			pos.shift_x(large ? 12 : 8);
		}
	}
}
