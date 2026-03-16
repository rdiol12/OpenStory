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
#include "UIQuestLog.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../../Data/ItemData.h"


#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIQuestLog::UIQuestLog(const QuestLog& ql) : UIDragElement<PosQUEST>(), questlog(ql), offset(0), selected_entry(-1), hover_entry(-1), show_detail(false)
	{
		tab = Buttons::TAB0;

		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node quest = nl::nx::ui["UIWindow2.img"]["Quest"];
		nl::node list = quest["list"];

		nl::node backgrnd = list["backgrnd"];

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(list["backgrnd2"]);

		notice_sprites.emplace_back(list["notice0"]);
		notice_sprites.emplace_back(list["notice1"]);
		notice_sprites.emplace_back(list["notice2"]);

		nl::node taben = list["Tab"]["enabled"];
		nl::node tabdis = list["Tab"]["disabled"];

		buttons[Buttons::TAB0] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
		buttons[Buttons::TAB1] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
		buttons[Buttons::TAB2] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
		buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(275, 6));
		buttons[Buttons::SEARCH] = std::make_unique<MapleButton>(list["BtSearch"]);
		buttons[Buttons::ALL_LEVEL] = std::make_unique<MapleButton>(list["BtAllLevel"]);
		buttons[Buttons::MY_LOCATION] = std::make_unique<MapleButton>(list["BtMyLocation"]);

		quest_state_started = Texture(list["questState"]["0"]);
		quest_state_completed = Texture(list["questState"]["1"]);

		quest_icon_anim = Animation(quest["icon"]["icon3"]);

		select_texture = Texture(list["recommend"]["select"]);
		drop_texture = Texture(list["recommend"]["drop"]);

		search_area = list["searchArea"];
		auto search_area_dim = search_area.get_dimensions();
		auto search_area_origin = search_area.get_origin().abs();

		auto search_pos_adj = Point<int16_t>(29, 0);
		auto search_dim_adj = Point<int16_t>(-80, 0);

		auto search_pos = position + search_area_origin + search_pos_adj;
		auto search_dim = search_pos + search_area_dim + search_dim_adj;

		search = Textfield(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, Rectangle<int16_t>(search_pos, search_dim), 19);
		placeholder = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, "Enter the quest name.");

		// Load detail panel textures
		nl::node quest_info_node = quest["quest_info"];
		if (quest_info_node)
		{
			detail_backgrnd = Texture(quest_info_node["backgrnd"]);
			detail_backgrnd2 = Texture(quest_info_node["backgrnd2"]);
			detail_backgrnd3 = Texture(quest_info_node["backgrnd3"]);
			detail_summary = Texture(quest_info_node["summary"]);
			detail_tip = Texture(quest_info_node["tip"]);

			// Load reward icon from summary_icon
			nl::node summary_icon = quest_info_node["summary_icon"];
			if (summary_icon && summary_icon["reward"])
				detail_reward_icon = Texture(summary_icon["reward"]);
		}

		load_quests();

		change_tab(tab);

		dimension = Texture(backgrnd).get_dimensions();

		if (dimension.x() == 0 || dimension.y() == 0)
			dimension = Point<int16_t>(280, 400);

		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIQuestLog::load_quests()
	{
		active_entries.clear();
		completed_entries.clear();


		nl::node quest_info = nl::nx::quest["QuestInfo.img"];

		for (auto& iter : questlog.get_started())
		{
			int16_t qid = iter.first;
			std::string name;

			nl::node qnode = quest_info[std::to_string(qid)];

			if (qnode)
				name = qnode["name"].get_string();

			if (name.empty())
				name = "Quest " + std::to_string(qid);

			active_entries.push_back({ qid, Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, name, 220, false) });
		}

		for (auto& iter : questlog.get_completed())
		{
			int16_t qid = iter.first;
			std::string name;

			nl::node qnode = quest_info[std::to_string(qid)];

			if (qnode)
				name = qnode["name"].get_string();

			if (name.empty())
				name = "Quest " + std::to_string(qid);

			completed_entries.push_back({ qid, Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, name, 220, false) });
		}

		uint16_t count = (tab == Buttons::TAB0) ? active_entries.size() : completed_entries.size();
		slider.setrows(ROWS, count);
		offset = 0;
		selected_entry = -1;
		hover_entry = -1;
		show_detail = false;
	}

	void UIQuestLog::update()
	{
		UIElement::update();
		quest_icon_anim.update();
	}

	void UIQuestLog::toggle_active()
	{
		UIElement::toggle_active();

		if (active)
		{
			load_quests();
			change_tab(tab);
		}
	}

	void UIQuestLog::select_quest(int16_t index)
	{
		const auto& entries = (tab == Buttons::TAB0) ? active_entries : completed_entries;

		if (index < 0 || (size_t)index >= entries.size())
		{
			show_detail = false;
			selected_entry = -1;
			return;
		}

		selected_entry = index;
		show_detail = true;

		int16_t qid = entries[index].id;
		std::string name = entries[index].name.get_text();

		// Load quest details from NX
		nl::node quest_info = nl::nx::quest["QuestInfo.img"];
		nl::node qnode = quest_info[std::to_string(qid)];

		// Get quest description: "1" = in-progress, "0" = start
		std::string desc;
		bool is_active = (tab == Buttons::TAB0);

		if (qnode)
		{
			if (is_active && qnode["1"])
				desc = qnode["1"].get_string();
			else if (qnode["0"])
				desc = qnode["0"].get_string();

			if (qnode["name"])
				name = qnode["name"].get_string();
		}

		if (desc.empty())
			desc = "No description available.";

		// Strip color/formatting codes (#b, #k, #e, #n, #r, #R..#) but keep \n for line breaks
		// The formatted text parser handles \n as linebreaks, so we keep those
		std::string clean_desc;
		for (size_t i = 0; i < desc.size(); i++)
		{
			if (desc[i] == '#' && i + 1 < desc.size())
			{
				char next = desc[i + 1];
				if (next == 'b' || next == 'k' || next == 'e' || next == 'n'
					|| next == 'r' || next == 'd' || next == 'g' || next == 'c'
					|| next == 'B' || next == 'L')
				{
					i++; // skip the code
					continue;
				}
				else if (next == 'R')
				{
					// #R...# reference - skip to closing #
					size_t end = desc.find('#', i + 2);
					if (end != std::string::npos)
						i = end;
					continue;
				}
				// Skip any other # codes we don't recognize
				clean_desc += desc[i];
			}
			else if (desc[i] == '\r')
			{
				// Convert \r\n to \n for the formatted text parser
				if (i + 1 < desc.size() && desc[i + 1] == '\n')
					i++; // skip \r, next iteration picks up \n
				clean_desc += '\\';
				clean_desc += 'n';
			}
			else if (desc[i] == '\n')
			{
				clean_desc += '\\';
				clean_desc += 'n';
			}
			else
			{
				clean_desc += desc[i];
			}
		}

		detail_quest_name = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK, name, 180, false);
		// Use formatted=true so \n line breaks are processed by the text layout engine
		detail_quest_desc = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, clean_desc, 260, true);

		// Load NPC sprite from Check.img
		detail_npc_sprite = Texture();
		detail_npc_name = Text();
		detail_level_req = Text();
		detail_rewards.clear();
		detail_requirements.clear();

		nl::node quest_check = nl::nx::quest["Check.img"];
		nl::node check_node = quest_check[std::to_string(qid)];

		int32_t npcid = 0;
		if (check_node)
		{
			// Check start (0) and completion (1) for NPC
			nl::node start_check = check_node["0"];
			nl::node end_check = check_node["1"];

			if (is_active && end_check["npc"])
				npcid = end_check["npc"].get_integer();
			else if (start_check["npc"])
				npcid = start_check["npc"].get_integer();

			// Level requirement
			int32_t lvmin = start_check["lvmin"].get_integer();
			if (lvmin > 0)
				detail_level_req = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Level " + std::to_string(lvmin) + "+", 120, false);

			// Load required items from end check
			if (is_active && end_check["item"])
			{
				for (auto item_node : end_check["item"])
				{
					int32_t itemid = item_node["id"].get_integer();
					int32_t count = item_node["count"].get_integer();

					if (itemid <= 0)
						continue;

					const ItemData& idata = ItemData::get(itemid);
					std::string item_name = idata.get_name();
					if (item_name.empty())
						item_name = "Item " + std::to_string(itemid);

					Texture icon = idata.get_icon(false);
					std::string count_str = std::to_string(count);

					detail_requirements.push_back({
						icon,
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, item_name, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, count_str, 40, false)
					});
				}
			}
		}

		// Load NPC sprite
		if (npcid > 0)
		{
			std::string strid = std::to_string(npcid);
			strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");

			nl::node npc_src = nl::nx::npc[strid];

			// Check for link
			std::string link = npc_src["info"]["link"].get_string();
			if (!link.empty())
			{
				link.append(".img");
				npc_src = nl::nx::npc[link];
			}

			if (npc_src["stand"] && npc_src["stand"]["0"])
				detail_npc_sprite = Texture(npc_src["stand"]["0"]);

			// Get NPC name
			std::string npc_name = nl::nx::string["Npc.img"][std::to_string(npcid)]["name"].get_string();
			if (!npc_name.empty())
				detail_npc_name = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, npc_name, 120, false);
		}

		// Load rewards from Act.img
		nl::node quest_act = nl::nx::quest["Act.img"];
		nl::node act_node = quest_act[std::to_string(qid)];
		if (act_node)
		{
			// Check completion rewards (node "1")
			nl::node reward_node = act_node["1"];
			if (!reward_node)
				reward_node = act_node["0"];

			if (reward_node)
			{
				// EXP reward
				int32_t exp_reward = reward_node["exp"].get_integer();
				if (exp_reward > 0)
				{
					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "EXP", 180, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "+" + std::to_string(exp_reward), 80, false)
					});
				}

				// Meso reward
				int32_t money = reward_node["money"].get_integer();
				if (money > 0)
				{
					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Mesos", 180, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "+" + std::to_string(money), 80, false)
					});
				}

				// Item rewards
				for (auto item_node : reward_node["item"])
				{
					int32_t itemid = item_node["id"].get_integer();
					int32_t count = item_node["count"].get_integer();

					if (itemid <= 0)
						continue;

					const ItemData& idata = ItemData::get(itemid);
					std::string item_name = idata.get_name();
					if (item_name.empty())
						item_name = "Item " + std::to_string(itemid);

					Texture icon = idata.get_icon(false);
					std::string count_str = "x" + std::to_string(count > 0 ? count : 1);

					detail_rewards.push_back({
						icon,
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, item_name, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, count_str, 80, false)
					});
				}
			}
		}
	}

	void UIQuestLog::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Draw tab notice
		Point<int16_t> notice_position = Point<int16_t>(0, 26);

		if (tab == Buttons::TAB0)
			notice_sprites[tab].draw(position + notice_position + Point<int16_t>(9, 0), alpha);
		else if (tab == Buttons::TAB1)
			notice_sprites[tab].draw(position + notice_position + Point<int16_t>(0, 0), alpha);
		else
			notice_sprites[tab].draw(position + notice_position + Point<int16_t>(-10, 0), alpha);

		if (tab != Buttons::TAB2)
		{
			search_area.draw(position);
			search.draw(Point<int16_t>(0, 0));

			if (search.get_state() == Textfield::State::NORMAL && search.empty())
				placeholder.draw(position + Point<int16_t>(39, 51));
		}

		// Draw slider and buttons
		slider.draw(position + Point<int16_t>(126, 75));
		UIElement::draw_buttons(alpha);

		const auto& entries = (tab == Buttons::TAB0) ? active_entries : completed_entries;
		bool is_active_tab = (tab == Buttons::TAB0);

		// Draw entries LAST so nothing covers them
		if (tab != Buttons::TAB2)
		{
			for (int16_t i = 0; i < ROWS; i++)
			{
				size_t idx = offset + i;

				if (idx >= entries.size())
					break;

				int16_t entry_y = LIST_Y + i * ROW_HEIGHT;

				// Draw hover highlight
				if (hover_entry >= 0 && (size_t)hover_entry == idx)
				{
					ColorBox hover_bg(250, ROW_HEIGHT, Color::Name::WHITE, 0.1f);
					hover_bg.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
				}

				// Draw selection highlight
				if (selected_entry >= 0 && (size_t)selected_entry == idx)
				{
					ColorBox sel_bg(250, ROW_HEIGHT, Color::Name::MEDIUMBLUE, 0.3f);
					sel_bg.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
				}

				// Draw quest state icon
				if (is_active_tab)
					quest_icon_anim.draw(DrawArgument(position + Point<int16_t>(15, entry_y + 3)), alpha);
				else
					quest_state_completed.draw(DrawArgument(position + Point<int16_t>(15, entry_y + 5)));

				// Draw quest name text
				entries[idx].name.draw(position + Point<int16_t>(35, entry_y + 5));
			}

		}

		// Draw detail panel to the right of the list (matching original MS layout)
		if (show_detail && selected_entry >= 0)
		{
			Point<int16_t> detail_pos = position + Point<int16_t>(dimension.x() - 3, 0);

			// Draw detail backgrounds
			if (detail_backgrnd.is_valid())
				detail_backgrnd.draw(detail_pos);

			if (detail_backgrnd2.is_valid())
				detail_backgrnd2.draw(detail_pos);

			if (detail_backgrnd3.is_valid())
				detail_backgrnd3.draw(detail_pos);

			// === TOP HEADER AREA ===
			// Quest name at top
			detail_quest_name.draw(detail_pos + Point<int16_t>(18, 15));

			// Level requirement below quest name
			detail_level_req.draw(detail_pos + Point<int16_t>(18, 35));

			// NPC sprite at top-right
			if (detail_npc_sprite.is_valid())
				detail_npc_sprite.draw(detail_pos + Point<int16_t>(230, 90));

			// Separator after header
			ColorBox header_sep(260, 1, Color::Name::DUSTYGRAY, 0.4f);
			header_sep.draw(DrawArgument(detail_pos + Point<int16_t>(15, 100)));

			int16_t y_offset = 110;

			// === REWARDS SECTION ===
			if (!detail_rewards.empty())
			{
				// "Reward!!" header with icon
				if (detail_reward_icon.is_valid())
				{
					detail_reward_icon.draw(detail_pos + Point<int16_t>(15, y_offset + 2));
					static Text reward_label(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ORANGE, "Reward!!", 200, false);
					reward_label.draw(detail_pos + Point<int16_t>(35, y_offset));
				}
				else
				{
					static Text reward_label(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ORANGE, "Reward!!", 200, false);
					reward_label.draw(detail_pos + Point<int16_t>(15, y_offset));
				}
				y_offset += 25;

				// "YOU WILL RECEIVE" sub-header
				static Text receive_label(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::ENDEAVOUR, "YOU WILL RECEIVE", 200, false);
				receive_label.draw(detail_pos + Point<int16_t>(18, y_offset));
				y_offset += 22;

				for (auto& rew : detail_rewards)
				{
					if (rew.icon.is_valid())
					{
						rew.icon.draw(detail_pos + Point<int16_t>(20, y_offset));
						rew.name.draw(detail_pos + Point<int16_t>(55, y_offset + 8));
						rew.count.draw(detail_pos + Point<int16_t>(180, y_offset + 8));
					}
					else
					{
						rew.name.draw(detail_pos + Point<int16_t>(25, y_offset + 2));
						rew.count.draw(detail_pos + Point<int16_t>(100, y_offset + 2));
					}
					y_offset += 35;
				}
				y_offset += 10;
			}

			// === REQUIREMENTS SECTION ===
			if (!detail_requirements.empty())
			{
				ColorBox sep_line(260, 1, Color::Name::DUSTYGRAY, 0.4f);
				sep_line.draw(DrawArgument(detail_pos + Point<int16_t>(15, y_offset)));
				y_offset += 12;

				static Text req_header(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ORANGE, "Requirements", 240, false);
				req_header.draw(detail_pos + Point<int16_t>(15, y_offset));
				y_offset += 25;

				for (auto& req : detail_requirements)
				{
					if (req.icon.is_valid())
					{
						req.icon.draw(detail_pos + Point<int16_t>(20, y_offset));
						req.name.draw(detail_pos + Point<int16_t>(55, y_offset + 8));
						req.count.draw(detail_pos + Point<int16_t>(220, y_offset + 8));
					}
					else
					{
						req.name.draw(detail_pos + Point<int16_t>(25, y_offset + 2));
						req.count.draw(detail_pos + Point<int16_t>(220, y_offset + 2));
					}
					y_offset += 35;
				}
				y_offset += 10;
			}

			// === QUEST DESCRIPTION at bottom ===
			ColorBox sep2(260, 1, Color::Name::DUSTYGRAY, 0.4f);
			sep2.draw(DrawArgument(detail_pos + Point<int16_t>(15, y_offset)));
			y_offset += 12;

			detail_quest_desc.draw(detail_pos + Point<int16_t>(15, y_offset));
		}
	}

	void UIQuestLog::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				if (show_detail)
				{
					show_detail = false;
					selected_entry = -1;
				}
				else
				{
					deactivate();
				}
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				uint16_t new_tab = tab;

				if (new_tab < Buttons::TAB2)
					new_tab++;
				else
					new_tab = Buttons::TAB0;

				change_tab(new_tab);
			}
		}
	}

	Cursor::State UIQuestLog::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		if (Cursor::State new_state = search.send_cursor(cursorpos, clicking))
			return new_state;

		// Track hover and click on quest entries
		if (tab != Buttons::TAB2)
		{
			const auto& entries = (tab == Buttons::TAB0) ? active_entries : completed_entries;
			int16_t new_hover = -1;

			for (int16_t i = 0; i < ROWS; i++)
			{
				size_t idx = offset + i;
				if (idx >= entries.size())
					break;

				int16_t entry_y = LIST_Y + i * ROW_HEIGHT;
				Rectangle<int16_t> entry_bounds(
					position + Point<int16_t>(10, entry_y),
					position + Point<int16_t>(260, entry_y + ROW_HEIGHT)
				);

				if (entry_bounds.contains(cursorpos))
				{
					new_hover = static_cast<int16_t>(idx);

					if (clicking)
					{
						select_quest(static_cast<int16_t>(idx));
						return Cursor::State::CLICKING;
					}

					break;
				}
			}

			hover_entry = new_hover;
		}

		return UIDragElement::send_cursor(clicking, cursorpos);
	}

	UIElement::Type UIQuestLog::get_type() const
	{
		return TYPE;
	}

	Button::State UIQuestLog::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::TAB0:
		case Buttons::TAB1:
		case Buttons::TAB2:
			change_tab(buttonid);

			return Button::State::IDENTITY;
		case Buttons::CLOSE:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIQuestLog::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			buttons[Buttons::TAB0 + oldtab]->set_state(Button::State::NORMAL);
			buttons[Buttons::MY_LOCATION]->set_active(tab == Buttons::TAB0);
			buttons[Buttons::ALL_LEVEL]->set_active(tab == Buttons::TAB0);
			buttons[Buttons::SEARCH]->set_active(tab != Buttons::TAB2);

			if (tab == Buttons::TAB2)
				search.set_state(Textfield::State::DISABLED);
			else
				search.set_state(Textfield::State::NORMAL);
		}

		buttons[Buttons::TAB0 + tab]->set_state(Button::State::PRESSED);

		offset = 0;
		selected_entry = -1;
		hover_entry = -1;
		show_detail = false;
		uint16_t count = 0;

		if (tab == Buttons::TAB0)
			count = active_entries.size();
		else if (tab == Buttons::TAB1)
			count = completed_entries.size();

		slider = Slider(
			Slider::Type::DEFAULT_SILVER, Range<int16_t>(LIST_Y, LIST_Y + ROWS * ROW_HEIGHT), 262, ROWS, count,
			[&](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				int16_t new_offset = offset + shift;
				uint16_t count = (tab == Buttons::TAB0) ? active_entries.size() : completed_entries.size();

				if (new_offset >= 0 && new_offset + ROWS <= (int16_t)count)
					offset = new_offset;
				else if (new_offset >= 0 && new_offset < (int16_t)count)
					offset = new_offset;
			}
		);
	}
}
