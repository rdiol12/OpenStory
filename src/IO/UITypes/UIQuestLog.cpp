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
#include "../UI.h"
#include "UIMiniMap.h"
#include "UINotice.h"
#include "UINpcTalk.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/QuestPackets.h"

#include <algorithm>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIQuestLog::UIQuestLog(const QuestLog& ql) :
		UIDragElement<PosQUEST>(), questlog(ql), offset(0),
		selected_entry(-1), hover_entry(-1), show_detail(false),
		detail_progress(0.0f), show_icon_info(false), detail_npcid(0),
		detail_area(0), detail_order(0), detail_auto_start(false),
		detail_auto_complete(false), filter_my_level(true)
	{
		tab = Buttons::TAB0;

		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node quest = nl::nx::ui["UIWindow2.img"]["Quest"];
		nl::node list = quest["list"];
		nl::node quest_info_node = quest["quest_info"];

		// === List panel backgrounds ===
		nl::node backgrnd = list["backgrnd"];
		sprites.emplace_back(backgrnd);
		sprites.emplace_back(list["backgrnd2"]);

		// === Notice sprites (all 4) ===
		notice_sprites.emplace_back(list["notice0"]);
		notice_sprites.emplace_back(list["notice1"]);
		notice_sprites.emplace_back(list["notice2"]);
		notice_sprites.emplace_back(list["notice3"]);

		// === Tabs ===
		nl::node taben = list["Tab"]["enabled"];
		nl::node tabdis = list["Tab"]["disabled"];

		buttons[Buttons::TAB0] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
		buttons[Buttons::TAB1] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
		buttons[Buttons::TAB2] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
		buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(275, 6));

		// === List buttons ===
		buttons[Buttons::ALL_LEVEL] = std::make_unique<MapleButton>(list["BtAllLevel"]);
		buttons[Buttons::MY_LEVEL] = std::make_unique<MapleButton>(list["BtMyLevel"]);
		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(list["BtSearch"]);
		buttons[Buttons::BT_NEXT] = std::make_unique<MapleButton>(list["BtNext"]);
		buttons[Buttons::BT_PREV] = std::make_unique<MapleButton>(list["BtPrev"]);
		buttons[Buttons::BT_ALLLOCN] = std::make_unique<MapleButton>(list["BtAllLocation"]);
		buttons[Buttons::BT_MYLOCATION] = std::make_unique<MapleButton>(list["BtMyLocation"]);
		buttons[Buttons::BT_ICONINFO] = std::make_unique<MapleButton>(list["BtIconInfo"]);

		// === Detail panel buttons from quest_info ===
		buttons[Buttons::GIVEUP] = std::make_unique<MapleButton>(quest_info_node["BtGiveup"]);
		buttons[Buttons::DETAIL] = std::make_unique<MapleButton>(quest_info_node["BtNavi"]);
		buttons[Buttons::MARK_NPC] = std::make_unique<MapleButton>(quest_info_node["BtNPC"]);
		buttons[Buttons::BT_ACCEPT] = std::make_unique<MapleButton>(quest_info_node["BtAccept"]);
		buttons[Buttons::BT_FINISH] = std::make_unique<MapleButton>(quest_info_node["BtFinish"]);
		buttons[Buttons::BT_DETAIL_CLOSE] = std::make_unique<MapleButton>(quest_info_node["BtClose"]);
		buttons[Buttons::BT_ARLIM] = std::make_unique<MapleButton>(quest_info_node["BtArlim"]);
		buttons[Buttons::BT_DELIVERY_ACCEPT] = std::make_unique<MapleButton>(quest_info_node["BtQuestDeliveryAccept"]);
		buttons[Buttons::BT_DELIVERY_COMPLETE] = std::make_unique<MapleButton>(quest_info_node["BtQuestDeliveryComplete"]);

		// Hide detail buttons until a quest is selected
		buttons[Buttons::GIVEUP]->set_active(false);
		buttons[Buttons::DETAIL]->set_active(false);
		buttons[Buttons::MARK_NPC]->set_active(false);
		buttons[Buttons::BT_ACCEPT]->set_active(false);
		buttons[Buttons::BT_FINISH]->set_active(false);
		buttons[Buttons::BT_DETAIL_CLOSE]->set_active(false);
		buttons[Buttons::BT_ARLIM]->set_active(false);
		buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(false);
		buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(false);

		// === Quest state icons ===
		quest_state_started = Texture(list["questState"]["0"]);
		quest_state_completed = Texture(list["questState"]["1"]);

		// === Quest icon set (all icons from root) ===
		nl::node icon_node = quest["icon"];
		icon0 = Texture(icon_node["icon0"]);
		icon1 = Texture(icon_node["icon1"]);
		icon4 = Texture(icon_node["icon4"]);
		icon10 = Texture(icon_node["icon10"]);

		// Animated icons
		quest_icon_anim = Animation(icon_node["icon3"]);
		icon2_anim = Animation(icon_node["icon2"]);
		icon5_anim = Animation(icon_node["icon5"]);
		icon6_anim = Animation(icon_node["icon6"]);
		icon7_anim = Animation(icon_node["icon7"]);
		icon8_anim = Animation(icon_node["icon8"]);
		icon9_anim = Animation(icon_node["icon9"]);
		iconQM0_anim = Animation(icon_node["iconQM0"]);
		iconQM1_anim = Animation(icon_node["iconQM1"]);

		// === Gauge2 - quest progress bar (root level) ===
		nl::node gauge2_node = quest["Gauge2"];
		gauge2_frame = Texture(gauge2_node["frame"]);
		gauge2_bar = Texture(gauge2_node["gauge"]);

		// === TimeQuest (timed quest UI) ===
		nl::node time_quest = quest["TimeQuest"];
		time_alarm_clock = Animation(time_quest["AlarmClock"]);
		time_bar = Animation(time_quest["TimeBar"]);

		// === icon_info panel ===
		nl::node icon_info = quest["icon_info"];
		icon_info_backgrnd = Texture(icon_info["backgrnd"]);
		icon_info_backgrnd2 = Texture(icon_info["backgrnd2"]);

		// === Recommend textures ===
		nl::node recommend = list["recommend"];
		select_texture = Texture(recommend["select"]);
		drop_texture = Texture(recommend["drop"]);
		recommend_focus = Texture(recommend["focus"]);
		recommend_title = Texture(list["recommendTitle"]);

		// === Complete count display ===
		complete_count = Texture(list["completeCount"]);

		// === Search area + textfield ===
		search_area = list["searchArea"];
		auto search_area_dim = search_area.get_dimensions();
		auto search_area_origin = search_area.get_origin().abs();

		auto search_pos_adj = Point<int16_t>(29, 0);
		auto search_dim_adj = Point<int16_t>(-80, 0);

		auto search_pos = position + search_area_origin + search_pos_adj;
		auto search_dim = search_pos + search_area_dim + search_dim_adj;

		search = Textfield(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, Rectangle<int16_t>(search_pos, search_dim), 19);
		placeholder = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, "Enter the quest name.");

		// === Detail panel textures from quest_info ===
		if (quest_info_node)
		{
			detail_backgrnd = Texture(quest_info_node["backgrnd"]);
			detail_backgrnd2 = Texture(quest_info_node["backgrnd2"]);
			detail_backgrnd3 = Texture(quest_info_node["backgrnd3"]);
			detail_summary = Texture(quest_info_node["summary"]);
			detail_summary_pattern = Texture(quest_info_node["summary_pattern"]);
			detail_tip = Texture(quest_info_node["tip"]);

			// Detail gauge
			nl::node detail_gauge = quest_info_node["Gauge"];
			if (detail_gauge)
			{
				detail_gauge_frame = Texture(detail_gauge["frame"]);
				detail_gauge_bar = Texture(detail_gauge["gauge"]);
			}

			// Reward icon from summary_icon
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
		available_entries.clear();
		active_entries.clear();
		completed_entries.clear();

		nl::node quest_info = nl::nx::quest["QuestInfo.img"];
		nl::node quest_check = nl::nx::quest["Check.img"];

		const auto& started = questlog.get_started();
		const auto& completed_map = questlog.get_completed();

		// TAB1: In-Progress quests (from server)
		for (auto& iter : started)
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

		// TAB2: Completed quests (from server)
		for (auto& iter : completed_map)
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

		// TAB0: Available quests (from NX data, filtered by level/job)
		const auto& player = Stage::get().get_player();
		int16_t player_level = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::LEVEL));
		int16_t player_job = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::JOB));

		for (auto qnode : quest_info)
		{
			std::string qid_str = qnode.name();

			// Skip non-numeric entries
			if (qid_str.empty() || qid_str[0] < '0' || qid_str[0] > '9')
				continue;

			int16_t qid = static_cast<int16_t>(std::stoi(qid_str));

			// Skip already started or completed quests
			if (started.count(qid) || completed_map.count(qid))
				continue;

			// Check requirements from Check.img
			nl::node check = quest_check[qid_str];
			if (!check)
				continue;

			nl::node start_check = check["0"];
			if (!start_check)
				continue;

			// Level check
			int32_t lvmin = start_check["lvmin"].get_integer();
			int32_t lvmax = start_check["lvmax"].get_integer();

			if (lvmin > 0 && player_level < lvmin)
				continue;

			if (lvmax > 0 && player_level > lvmax)
				continue;

			// Filter by "my level" — only show quests within reasonable range
			if (filter_my_level && lvmin > 0 && (player_level - lvmin) > 30)
				continue;

			// Job check
			nl::node job_node = start_check["job"];
			if (job_node && job_node.size() > 0)
			{
				bool job_ok = false;
				for (auto j : job_node)
				{
					int32_t required_job = j.get_integer();

					if (required_job == 0 || required_job == player_job)
					{
						job_ok = true;
						break;
					}

					// Job group check (e.g., 100 matches 110, 111, 112)
					if (required_job > 0 && (player_job / 100) == (required_job / 100))
					{
						if (player_job >= required_job)
						{
							job_ok = true;
							break;
						}
					}
				}

				if (!job_ok)
					continue;
			}

			// Prerequisite quest check
			nl::node quest_prereq = start_check["quest"];
			if (quest_prereq)
			{
				bool prereqs_met = true;
				for (auto qp : quest_prereq)
				{
					int32_t prereq_id = qp["id"].get_integer();
					int32_t prereq_state = qp["state"].get_integer();

					if (prereq_id > 0)
					{
						// state 2 = must be completed
						if (prereq_state == 2 && !completed_map.count(static_cast<int16_t>(prereq_id)))
						{
							prereqs_met = false;
							break;
						}
						// state 1 = must be started
						if (prereq_state == 1 && !started.count(static_cast<int16_t>(prereq_id)))
						{
							prereqs_met = false;
							break;
						}
					}
				}

				if (!prereqs_met)
					continue;
			}

			// Quest is available — get name
			std::string name = qnode["name"].get_string();
			if (name.empty())
				name = "Quest " + std::to_string(qid);

			available_entries.push_back({ qid, Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, name, 220, false) });
		}

		// Sort available entries by level requirement
		std::sort(available_entries.begin(), available_entries.end(),
			[&](const QuestEntry& a, const QuestEntry& b)
			{
				nl::node ca = quest_check[std::to_string(a.id)]["0"];
				nl::node cb = quest_check[std::to_string(b.id)]["0"];
				int32_t la = ca ? ca["lvmin"].get_integer() : 0;
				int32_t lb = cb ? cb["lvmin"].get_integer() : 0;
				return la < lb;
			});

		uint16_t count;
		if (tab == Buttons::TAB0)
			count = static_cast<uint16_t>(available_entries.size());
		else if (tab == Buttons::TAB1)
			count = static_cast<uint16_t>(active_entries.size());
		else
			count = static_cast<uint16_t>(completed_entries.size());

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
		icon2_anim.update();
		icon5_anim.update();
		icon6_anim.update();
		icon7_anim.update();
		icon8_anim.update();
		icon9_anim.update();
		iconQM0_anim.update();
		iconQM1_anim.update();
		time_alarm_clock.update();
		time_bar.update();
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

	static std::string strip_quest_codes(const std::string& desc)
	{
		std::string clean;
		for (size_t i = 0; i < desc.size(); i++)
		{
			if (desc[i] == '#' && i + 1 < desc.size())
			{
				char next = desc[i + 1];
				if (next == 'b' || next == 'k' || next == 'e' || next == 'n'
					|| next == 'r' || next == 'd' || next == 'g' || next == 'c'
					|| next == 'B' || next == 'L' || next == 'l')
				{
					i++;
					continue;
				}
				else if (next == 'R' || next == 'f' || next == 'i' || next == 't'
					|| next == 'p' || next == 'h' || next == 'm' || next == 'o'
					|| next == 's' || next == 'v' || next == 'z')
				{
					size_t end = desc.find('#', i + 2);
					if (end != std::string::npos)
						i = end;
					continue;
				}
				clean += desc[i];
			}
			else if (desc[i] == '\r')
			{
				if (i + 1 < desc.size() && desc[i + 1] == '\n')
					i++;
				clean += '\\';
				clean += 'n';
			}
			else if (desc[i] == '\n')
			{
				clean += '\\';
				clean += 'n';
			}
			else
			{
				clean += desc[i];
			}
		}
		return clean;
	}

	void UIQuestLog::select_quest(int16_t index)
	{
		const auto& entries = (tab == Buttons::TAB0) ? available_entries :
			(tab == Buttons::TAB1) ? active_entries : completed_entries;

		if (index < 0 || (size_t)index >= entries.size())
		{
			show_detail = false;
			selected_entry = -1;
			buttons[Buttons::GIVEUP]->set_active(false);
			buttons[Buttons::DETAIL]->set_active(false);
			buttons[Buttons::MARK_NPC]->set_active(false);
			buttons[Buttons::BT_ACCEPT]->set_active(false);
			buttons[Buttons::BT_FINISH]->set_active(false);
			buttons[Buttons::BT_DETAIL_CLOSE]->set_active(false);
			buttons[Buttons::BT_ARLIM]->set_active(false);
			buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(false);
			buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(false);
			return;
		}

		selected_entry = index;
		show_detail = true;

		bool is_available = (tab == Buttons::TAB0);
		bool is_in_progress = (tab == Buttons::TAB1);

		// Position detail buttons relative to detail panel
		Point<int16_t> dp = Point<int16_t>(dimension.x() - 3, 0);

		buttons[Buttons::GIVEUP]->set_position(dp + Point<int16_t>(20, 370));
		buttons[Buttons::DETAIL]->set_position(dp + Point<int16_t>(100, 370));
		buttons[Buttons::MARK_NPC]->set_position(dp + Point<int16_t>(180, 370));
		buttons[Buttons::BT_ACCEPT]->set_position(dp + Point<int16_t>(60, 400));
		buttons[Buttons::BT_FINISH]->set_position(dp + Point<int16_t>(160, 400));
		buttons[Buttons::BT_DETAIL_CLOSE]->set_position(dp + Point<int16_t>(260, 6));
		buttons[Buttons::BT_ARLIM]->set_position(dp + Point<int16_t>(20, 400));
		buttons[Buttons::BT_DELIVERY_ACCEPT]->set_position(dp + Point<int16_t>(60, 430));
		buttons[Buttons::BT_DELIVERY_COMPLETE]->set_position(dp + Point<int16_t>(160, 430));

		// Show/hide detail buttons based on quest state
		buttons[Buttons::GIVEUP]->set_active(is_in_progress);
		buttons[Buttons::DETAIL]->set_active(true);
		buttons[Buttons::MARK_NPC]->set_active(true);
		buttons[Buttons::BT_ACCEPT]->set_active(is_available);
		buttons[Buttons::BT_FINISH]->set_active(is_in_progress);
		buttons[Buttons::BT_DETAIL_CLOSE]->set_active(true);
		buttons[Buttons::BT_ARLIM]->set_active(true);
		buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(false);
		buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(false);

		int16_t qid = entries[index].id;
		std::string name = entries[index].name.get_text();

		// === QuestInfo.img: name, descriptions, area, parent, order, autoStart, autoPreComplete ===
		nl::node quest_info = nl::nx::quest["QuestInfo.img"];
		nl::node qnode = quest_info[std::to_string(qid)];

		std::string desc;
		detail_area = 0;
		detail_parent = "";
		detail_order = 0;
		detail_auto_start = false;
		detail_auto_complete = false;

		if (qnode)
		{
			if (is_in_progress && qnode["1"])
				desc = qnode["1"].get_string();
			else if (qnode["0"])
				desc = qnode["0"].get_string();

			if (qnode["name"])
				name = qnode["name"].get_string();

			detail_area = qnode["area"].get_integer();
			detail_parent = qnode["parent"].get_string();
			detail_order = qnode["order"].get_integer();
			detail_auto_start = (qnode["autoStart"].get_integer() != 0);
			detail_auto_complete = (qnode["autoPreComplete"].get_integer() != 0);
		}

		if (desc.empty())
			desc = "No description available.";

		std::string clean_desc = strip_quest_codes(desc);

		detail_quest_name = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK, name, 180, false);
		detail_quest_desc = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, clean_desc, 260, true);

		// Build summary with parent/area info
		std::string summary_str;
		if (!detail_parent.empty())
			summary_str = detail_parent;
		if (detail_area > 0)
		{
			if (!summary_str.empty()) summary_str += " - ";
			summary_str += "Area " + std::to_string(detail_area);
		}
		if (!summary_str.empty())
			detail_quest_summary = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, summary_str, 200, false);
		else
			detail_quest_summary = Text();

		// Reset per-quest detail state
		detail_npc_sprite = Texture();
		detail_npc_name = Text();
		detail_level_req = Text();
		detail_job_req = Text();
		detail_rewards.clear();
		detail_requirements.clear();
		detail_prereq_quests.clear();
		detail_say_lines.clear();
		detail_pquest_name = "";
		detail_pquest_mark = "";
		detail_medal = "";
		detail_progress = 0.0f;
		detail_npcid = 0;

		// === Check.img: NPC, level, job, prereq quests, items, mobs ===
		nl::node quest_check = nl::nx::quest["Check.img"];
		nl::node check_node = quest_check[std::to_string(qid)];

		if (check_node)
		{
			nl::node start_check = check_node["0"];
			nl::node end_check = check_node["1"];

			// NPC
			if (is_in_progress && end_check["npc"])
				detail_npcid = end_check["npc"].get_integer();
			else if (start_check["npc"])
				detail_npcid = start_check["npc"].get_integer();

			// Level requirement
			int32_t lvmin = start_check["lvmin"].get_integer();
			if (lvmin > 0)
				detail_level_req = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Level " + std::to_string(lvmin) + "+", 120, false);

			// Job requirement
			nl::node job_node = start_check["job"];
			if (job_node)
			{
				std::string job_str = "Job: ";
				bool first = true;
				for (auto j : job_node)
				{
					int32_t jobid = j.get_integer();
					if (!first) job_str += ", ";
					job_str += std::to_string(jobid);
					first = false;
				}
				if (!first)
					detail_job_req = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, job_str, 200, false);
			}

			// Prerequisite quests
			nl::node quest_prereq = start_check["quest"];
			if (quest_prereq)
			{
				for (auto qp : quest_prereq)
				{
					int32_t prereq_id = qp["id"].get_integer();
					if (prereq_id <= 0) continue;

					std::string pname;
					nl::node pnode = quest_info[std::to_string(prereq_id)];
					if (pnode)
						pname = pnode["name"].get_string();
					if (pname.empty())
						pname = "Quest " + std::to_string(prereq_id);

					detail_prereq_quests.push_back(
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, pname, 220, false)
					);
				}
			}

			// Required items - check player inventory for progress
			int32_t total_required = 0;
			int32_t total_collected = 0;
			const Inventory& inventory = Stage::get().get_player().get_inventory();

			if (is_in_progress && end_check["item"])
			{
				for (auto item_node : end_check["item"])
				{
					int32_t itemid = item_node["id"].get_integer();
					int32_t count = item_node["count"].get_integer();

					if (itemid <= 0)
						continue;

					total_required += count;
					int16_t have = inventory.get_total_item_count(itemid);
					total_collected += std::min((int32_t)have, count);

					const ItemData& idata = ItemData::get(itemid);
					std::string item_name = idata.get_name();
					if (item_name.empty())
						item_name = "Item " + std::to_string(itemid);

					Texture icon = idata.get_icon(false);

					// Show "have/need" count with color based on completion
					std::string count_str = std::to_string(have) + "/" + std::to_string(count);
					Color::Name count_color = (have >= count) ? Color::Name::CHARTREUSE : Color::Name::DUSTYGRAY;

					detail_requirements.push_back({
						icon,
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, item_name, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, count_color, count_str, 60, false)
					});
				}
			}

			// Required mob kills
			if (is_in_progress && end_check["mob"])
			{
				for (auto mob_node : end_check["mob"])
				{
					int32_t mobid = mob_node["id"].get_integer();
					int32_t count = mob_node["count"].get_integer();

					if (mobid <= 0)
						continue;

					total_required += count;

					std::string mob_name = nl::nx::string["Mob.img"][std::to_string(mobid)]["name"].get_string();
					if (mob_name.empty())
						mob_name = "Monster " + std::to_string(mobid);

					detail_requirements.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, mob_name, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, std::to_string(count), 40, false)
					});
				}
			}

			if (total_required > 0)
				detail_progress = static_cast<float>(total_collected) / static_cast<float>(total_required);
		}

		// === Load NPC sprite ===
		if (detail_npcid > 0)
		{
			std::string strid = std::to_string(detail_npcid);
			strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");

			nl::node npc_src = nl::nx::npc[strid];

			std::string link = npc_src["info"]["link"].get_string();
			if (!link.empty())
			{
				link.append(".img");
				npc_src = nl::nx::npc[link];
			}

			if (npc_src["stand"] && npc_src["stand"]["0"])
				detail_npc_sprite = Texture(npc_src["stand"]["0"]);

			std::string npc_name = nl::nx::string["Npc.img"][std::to_string(detail_npcid)]["name"].get_string();
			if (!npc_name.empty())
				detail_npc_name = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, npc_name, 120, false);
		}

		// === Act.img: rewards (exp, money, fame, buff, items, skills, nextQuest) ===
		nl::node quest_act = nl::nx::quest["Act.img"];
		nl::node act_node = quest_act[std::to_string(qid)];
		if (act_node)
		{
			nl::node reward_node = act_node["1"];
			if (!reward_node)
				reward_node = act_node["0"];

			if (reward_node)
			{
				// EXP
				int32_t exp_reward = reward_node["exp"].get_integer();
				if (exp_reward > 0)
				{
					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "EXP", 180, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "+" + std::to_string(exp_reward), 80, false)
					});
				}

				// Mesos
				int32_t money = reward_node["money"].get_integer();
				if (money > 0)
				{
					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Mesos", 180, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "+" + std::to_string(money), 80, false)
					});
				}

				// Fame
				int32_t fame = reward_node["pop"].get_integer();
				if (fame > 0)
				{
					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Fame", 180, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "+" + std::to_string(fame), 80, false)
					});
				}

				// Buff item
				int32_t buffid = reward_node["buffItemID"].get_integer();
				if (buffid > 0)
				{
					const ItemData& bdata = ItemData::get(buffid);
					std::string bname = bdata.get_name();
					if (bname.empty())
						bname = "Buff Item " + std::to_string(buffid);

					detail_rewards.push_back({
						bdata.get_icon(false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, bname, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "x1", 80, false)
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

					detail_rewards.push_back({
						idata.get_icon(false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, item_name, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "x" + std::to_string(count > 0 ? count : 1), 80, false)
					});
				}

				// Skill rewards
				for (auto skill_node : reward_node["skill"])
				{
					int32_t skillid = skill_node["id"].get_integer();
					if (skillid <= 0) continue;

					std::string sname = "Skill " + std::to_string(skillid);
					int32_t mslevel = skill_node["masterLevel"].get_integer();
					int32_t slevel = skill_node["skillLevel"].get_integer();

					std::string slvl_str;
					if (mslevel > 0)
						slvl_str = "Lv." + std::to_string(mslevel);
					else if (slevel > 0)
						slvl_str = "Lv." + std::to_string(slevel);

					detail_rewards.push_back({
						Texture(),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, sname, 150, false),
						Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, slvl_str, 80, false)
					});
				}
			}
		}

		// === Say.img: NPC dialog lines ===
		nl::node say_node = nl::nx::quest["Say.img"][std::to_string(qid)];
		if (say_node)
		{
			// Load start (0) or completion (1) dialog
			nl::node say_phase = is_in_progress ? say_node["0"] : say_node["1"];
			if (!say_phase)
				say_phase = say_node["0"];

			if (say_phase)
			{
				bool has_yes = (say_phase["yes"].data_type() != nl::node::type::none);
				bool has_no = (say_phase["no"].data_type() != nl::node::type::none);

				for (auto line : say_phase)
				{
					std::string lname = line.name();
					// Only numbered entries are dialog lines
					if (!lname.empty() && lname[0] >= '0' && lname[0] <= '9')
					{
						std::string text = strip_quest_codes(line.get_string());
						detail_say_lines.push_back({ text, has_yes && has_no });
					}
				}
			}
		}

		// === PQuest.img ===
		nl::node pquest = nl::nx::quest["PQuest.img"];
		for (auto pq : pquest)
		{
			for (auto child : pq)
			{
				if (child.name() != "rank" && child.name() != "mark")
				{
					// The quest name key has the PQ name as key and 0 as value
					// Check if this PQ relates to our quest
				}
			}
		}

		// === Exclusive.img: medal ===
		nl::node exclusive = nl::nx::quest["Exclusive.img"];
		if (exclusive["medal"])
		{
			std::string medal_val = exclusive["medal"][std::to_string(qid)].get_string();
			if (!medal_val.empty())
				detail_medal = medal_val;
		}

		// Show delivery buttons if quest has delivery items
		bool has_delivery = false;
		if (is_in_progress && check_node)
		{
			nl::node end_check = check_node["1"];
			if (end_check && end_check["item"])
				has_delivery = true;
		}
		buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(has_delivery && is_available);
		buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(has_delivery && is_in_progress);
	}

	void UIQuestLog::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Draw tab notice
		if (tab < notice_sprites.size())
		{
			Point<int16_t> notice_position = Point<int16_t>(0, 26);

			if (tab == Buttons::TAB0)
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(9, 0), alpha);
			else if (tab == Buttons::TAB1)
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(0, 0), alpha);
			else if (tab == Buttons::TAB2)
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(-10, 0), alpha);
			else if (tab == 3 && notice_sprites.size() > 3)
				notice_sprites[3].draw(position + notice_position + Point<int16_t>(-20, 0), alpha);
		}

		if (tab != Buttons::TAB2)
		{
			search_area.draw(position);
			search.draw(Point<int16_t>(0, 0));

			if (search.get_state() == Textfield::State::NORMAL && search.empty())
				placeholder.draw(position + Point<int16_t>(39, 51));
		}

		// Draw completed count on completed tab
		if (tab == Buttons::TAB2 && complete_count.is_valid())
			complete_count.draw(DrawArgument(position + Point<int16_t>(10, 55)));

		// Draw recommend title on active tab
		if (tab == Buttons::TAB0 && recommend_title.is_valid())
			recommend_title.draw(DrawArgument(position + Point<int16_t>(10, 60)));

		// Draw slider and buttons
		slider.draw(position + Point<int16_t>(126, 75));
		UIElement::draw_buttons(alpha);

		const auto& entries = (tab == Buttons::TAB0) ? available_entries :
			(tab == Buttons::TAB1) ? active_entries : completed_entries;
		bool is_active_tab = (tab == Buttons::TAB0 || tab == Buttons::TAB1);

		// Draw quest entries
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
					if (recommend_focus.is_valid())
						recommend_focus.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
					else
					{
						ColorBox hover_bg(250, ROW_HEIGHT, Color::Name::WHITE, 0.1f);
						hover_bg.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
					}
				}

				// Draw selection highlight
				if (selected_entry >= 0 && (size_t)selected_entry == idx)
				{
					if (select_texture.is_valid())
						select_texture.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
					else
					{
						ColorBox sel_bg(250, ROW_HEIGHT, Color::Name::MEDIUMBLUE, 0.3f);
						sel_bg.draw(DrawArgument(position + Point<int16_t>(8, entry_y)));
					}
				}

				// Draw quest state icon
				if (is_active_tab)
					quest_icon_anim.draw(DrawArgument(position + Point<int16_t>(15, entry_y + 3)), alpha);
				else
				{
					if (icon1.is_valid())
						icon1.draw(DrawArgument(position + Point<int16_t>(15, entry_y + 5)));
					else
						quest_state_completed.draw(DrawArgument(position + Point<int16_t>(15, entry_y + 5)));
				}

				// Draw quest name text
				entries[idx].name.draw(position + Point<int16_t>(35, entry_y + 5));
			}
		}

		// Draw icon info panel if toggled (icon legend)
		if (show_icon_info)
		{
			Point<int16_t> info_pos = position + Point<int16_t>(dimension.x() + 5, 30);
			if (icon_info_backgrnd.is_valid())
				icon_info_backgrnd.draw(info_pos);
			if (icon_info_backgrnd2.is_valid())
				icon_info_backgrnd2.draw(info_pos);

			// Draw icon legend entries
			int16_t iy = 20;
			if (icon0.is_valid())
			{
				icon0.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)));
				iy += 25;
			}
			if (icon1.is_valid())
			{
				icon1.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)));
				iy += 25;
			}
			icon2_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			quest_icon_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			if (icon4.is_valid())
			{
				icon4.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)));
				iy += 25;
			}
			icon5_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			icon6_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			icon7_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			icon8_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			icon9_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			if (icon10.is_valid())
			{
				icon10.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)));
				iy += 25;
			}
			iconQM0_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
			iy += 25;
			iconQM1_anim.draw(DrawArgument(info_pos + Point<int16_t>(15, iy)), alpha);
		}

		// Draw TimeQuest elements (timed quest indicator)
		time_alarm_clock.draw(DrawArgument(position + Point<int16_t>(240, 30)), alpha);
		time_bar.draw(DrawArgument(position + Point<int16_t>(10, 395)), alpha);

		// === Detail panel (right of list) ===
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

			// Summary pattern and graphic
			if (detail_summary_pattern.is_valid())
				detail_summary_pattern.draw(detail_pos + Point<int16_t>(0, 10));

			if (detail_summary.is_valid())
				detail_summary.draw(detail_pos + Point<int16_t>(0, 5));

			// Quest name at top
			detail_quest_name.draw(detail_pos + Point<int16_t>(18, 15));

			// Quest chain/area info
			detail_quest_summary.draw(detail_pos + Point<int16_t>(18, 30));

			// Level requirement
			detail_level_req.draw(detail_pos + Point<int16_t>(18, 43));

			// Job requirement
			detail_job_req.draw(detail_pos + Point<int16_t>(18, 56));

			// Auto-start/complete indicators
			if (detail_auto_start)
			{
				static Text auto_label(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MALIBU, "[Auto-Start]", 120, false);
				auto_label.draw(detail_pos + Point<int16_t>(150, 43));
			}
			if (detail_auto_complete)
			{
				static Text auto_label(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MALIBU, "[Auto-Complete]", 120, false);
				auto_label.draw(detail_pos + Point<int16_t>(150, 56));
			}

			// NPC sprite at top-right
			if (detail_npc_sprite.is_valid())
				detail_npc_sprite.draw(detail_pos + Point<int16_t>(230, 90));

			// NPC name below sprite
			detail_npc_name.draw(detail_pos + Point<int16_t>(230, 130));

			// Tip graphic
			if (detail_tip.is_valid())
				detail_tip.draw(detail_pos + Point<int16_t>(10, 70));

			// Progress gauge (for active quests with requirements)
			if (detail_progress > 0.0f)
			{
				if (detail_gauge_frame.is_valid())
				{
					detail_gauge_frame.draw(detail_pos + Point<int16_t>(15, 85));

					if (detail_gauge_bar.is_valid())
						detail_gauge_bar.draw(DrawArgument(detail_pos + Point<int16_t>(15, 85)));
				}
				else if (gauge2_frame.is_valid())
				{
					gauge2_frame.draw(detail_pos + Point<int16_t>(15, 85));

					if (gauge2_bar.is_valid())
						gauge2_bar.draw(DrawArgument(detail_pos + Point<int16_t>(15, 85)));
				}
			}

			// Separator after header
			ColorBox header_sep(260, 1, Color::Name::DUSTYGRAY, 0.4f);
			header_sep.draw(DrawArgument(detail_pos + Point<int16_t>(15, 100)));

			int16_t y_offset = 110;

			// === NPC Dialog preview from Say.img ===
			if (!detail_say_lines.empty())
			{
				static Text say_header(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ENDEAVOUR, "NPC Dialog:", 240, false);
				say_header.draw(detail_pos + Point<int16_t>(15, y_offset));
				y_offset += 18;

				// Show first line as preview
				Text say_preview(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, detail_say_lines[0].text, 240, true);
				say_preview.draw(detail_pos + Point<int16_t>(18, y_offset));
				y_offset += std::max((int16_t)30, say_preview.height()) + 10;

				ColorBox say_sep(260, 1, Color::Name::DUSTYGRAY, 0.3f);
				say_sep.draw(DrawArgument(detail_pos + Point<int16_t>(15, y_offset)));
				y_offset += 10;
			}

			// === Prerequisite quests ===
			if (!detail_prereq_quests.empty())
			{
				static Text prereq_header(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ORANGE, "Prerequisites", 240, false);
				prereq_header.draw(detail_pos + Point<int16_t>(15, y_offset));
				y_offset += 20;

				for (auto& pq : detail_prereq_quests)
				{
					pq.draw(detail_pos + Point<int16_t>(25, y_offset));
					y_offset += 16;
				}
				y_offset += 10;
			}

			// === Rewards section ===
			if (!detail_rewards.empty())
			{
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

			// === Requirements section ===
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

			// === Medal info ===
			if (!detail_medal.empty())
			{
				static Text medal_header(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::ORANGE, "Medal", 240, false);
				medal_header.draw(detail_pos + Point<int16_t>(15, y_offset));
				y_offset += 20;

				Text medal_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, detail_medal, 240, false);
				medal_text.draw(detail_pos + Point<int16_t>(25, y_offset));
				y_offset += 20;
			}

			// === Quest description at bottom ===
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
				if (show_icon_info)
				{
					show_icon_info = false;
				}
				else if (show_detail)
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
		{
			const auto& entries = (tab == Buttons::TAB0) ? available_entries :
				(tab == Buttons::TAB1) ? active_entries : completed_entries;
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
		case Buttons::BT_DETAIL_CLOSE:
			show_detail = false;
			selected_entry = -1;
			buttons[Buttons::GIVEUP]->set_active(false);
			buttons[Buttons::DETAIL]->set_active(false);
			buttons[Buttons::MARK_NPC]->set_active(false);
			buttons[Buttons::BT_ACCEPT]->set_active(false);
			buttons[Buttons::BT_FINISH]->set_active(false);
			buttons[Buttons::BT_DETAIL_CLOSE]->set_active(false);
			buttons[Buttons::BT_ARLIM]->set_active(false);
			buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(false);
			buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(false);
			break;
		case Buttons::GIVEUP:
		{
			if (selected_entry >= 0 && tab == Buttons::TAB1
				&& (size_t)selected_entry < active_entries.size())
			{
				int16_t questid = active_entries[selected_entry].id;

				UI::get().emplace<UIYesNo>("Are you sure you want to forfeit this quest?",
					[questid](bool yes)
					{
						if (yes)
							ForfeitQuestPacket(questid).dispatch();
					});
			}
			break;
		}
		case Buttons::DETAIL:
			if (selected_entry >= 0)
				show_detail = !show_detail;
			break;
		case Buttons::MARK_NPC:
		{
			if (detail_npcid > 0)
			{
				auto minimap = UI::get().get_element<UIMiniMap>();

				if (minimap)
					minimap->set_quest_npc(detail_npcid);
			}
			break;
		}
		case Buttons::BT_ACCEPT:
		{
			if (selected_entry >= 0 && tab == Buttons::TAB0
				&& (size_t)selected_entry < available_entries.size())
			{
				int16_t questid = available_entries[selected_entry].id;
				StartQuestPacket(questid, 0).dispatch();
			}
			break;
		}
		case Buttons::BT_FINISH:
		{
			if (selected_entry >= 0 && tab == Buttons::TAB1
				&& (size_t)selected_entry < active_entries.size())
			{
				int16_t questid = active_entries[selected_entry].id;
				CompleteQuestPacket(questid, 0).dispatch();
			}
			break;
		}
		case Buttons::BT_ARLIM:
			// Arlim - open quest guide/helper
			break;
		case Buttons::BT_DELIVERY_ACCEPT:
		{
			if (selected_entry >= 0 && tab == Buttons::TAB0
				&& (size_t)selected_entry < available_entries.size())
			{
				int16_t questid = available_entries[selected_entry].id;
				StartQuestPacket(questid, 0).dispatch();
			}
			break;
		}
		case Buttons::BT_DELIVERY_COMPLETE:
		{
			if (selected_entry >= 0 && tab == Buttons::TAB1
				&& (size_t)selected_entry < active_entries.size())
			{
				int16_t questid = active_entries[selected_entry].id;
				CompleteQuestPacket(questid, 0).dispatch();
			}
			break;
		}
		case Buttons::BT_SEARCH:
			break;
		case Buttons::BT_NEXT:
		{
			const auto& entries = (tab == Buttons::TAB0) ? available_entries :
				(tab == Buttons::TAB1) ? active_entries : completed_entries;
			uint16_t count = entries.size();
			if (offset + ROWS < (int16_t)count)
				offset += ROWS;
			break;
		}
		case Buttons::BT_PREV:
			if (offset >= ROWS)
				offset -= ROWS;
			else
				offset = 0;
			break;
		case Buttons::BT_ICONINFO:
			show_icon_info = !show_icon_info;
			return Button::State::NORMAL;
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

			// Level/location filter buttons only on active tab
			buttons[Buttons::ALL_LEVEL]->set_active(tab == Buttons::TAB0);
			buttons[Buttons::MY_LEVEL]->set_active(tab == Buttons::TAB0);
			buttons[Buttons::BT_ALLLOCN]->set_active(tab == Buttons::TAB0);
			buttons[Buttons::BT_MYLOCATION]->set_active(tab == Buttons::TAB0);

			// Search on all except last tab
			bool search_active = (tab != Buttons::TAB2);
			buttons[Buttons::BT_SEARCH]->set_active(search_active);

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
		show_icon_info = false;

		// Hide all detail buttons
		buttons[Buttons::GIVEUP]->set_active(false);
		buttons[Buttons::DETAIL]->set_active(false);
		buttons[Buttons::MARK_NPC]->set_active(false);
		buttons[Buttons::BT_ACCEPT]->set_active(false);
		buttons[Buttons::BT_FINISH]->set_active(false);
		buttons[Buttons::BT_DETAIL_CLOSE]->set_active(false);
		buttons[Buttons::BT_ARLIM]->set_active(false);
		buttons[Buttons::BT_DELIVERY_ACCEPT]->set_active(false);
		buttons[Buttons::BT_DELIVERY_COMPLETE]->set_active(false);

		uint16_t count = 0;

		if (tab == Buttons::TAB0)
			count = static_cast<uint16_t>(available_entries.size());
		else if (tab == Buttons::TAB1)
			count = static_cast<uint16_t>(active_entries.size());
		else
			count = static_cast<uint16_t>(completed_entries.size());

		slider = Slider(
			Slider::Type::DEFAULT_SILVER, Range<int16_t>(LIST_Y, LIST_Y + ROWS * ROW_HEIGHT), 262, ROWS, count,
			[&](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				int16_t new_offset = offset + shift;
				uint16_t count = (tab == Buttons::TAB0) ? available_entries.size() :
					(tab == Buttons::TAB1) ? active_entries.size() : completed_entries.size();

				if (new_offset >= 0 && new_offset + ROWS <= (int16_t)count)
					offset = new_offset;
				else if (new_offset >= 0 && new_offset < (int16_t)count)
					offset = new_offset;
			}
		);
	}
}
