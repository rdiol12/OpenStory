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
#include "UIQuestHelper.h"
#include "UIQuestLog.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIQuestHelper::UIQuestHelper(const QuestLog& ql) :
		UIDragElement<PosQUESTHELPER>(), questlog(ql),
		tracked_questid(-1), minimized(false), show_bulb(false)
	{
		// === QuestAlarm (UIWindow.img — v83 primary) ===
		nl::node alarm1 = nl::nx::ui["UIWindow.img"]["QuestAlarm"];
		if (alarm1)
		{
			bg_min = Texture(alarm1["backgrndmin"]);
			bg_center = Texture(alarm1["backgrndcenter"]);
			bg_bottom = Texture(alarm1["backgrndbottom"]);
			bg_max = Texture(alarm1["backgrndmax"]);

			nl::node btq = alarm1["BtQ"];
			if (btq.size() > 0)
				buttons[Buttons::BT_Q] = std::make_unique<MapleButton>(btq);

			nl::node btauto = alarm1["BtAuto"];
			if (btauto.size() > 0)
				buttons[Buttons::BT_AUTO] = std::make_unique<MapleButton>(btauto);
		}

		// === QuestAlarm (UIWindow2.img — extra buttons) ===
		nl::node alarm2 = nl::nx::ui["UIWindow2.img"]["QuestAlarm"];
		if (alarm2)
		{
			// Load extra buttons only if v83 ones didn't load
			if (!bg_min.is_valid())
			{
				bg_min = Texture(alarm2["backgrndmin"]);
				bg_center = Texture(alarm2["backgrndcenter"]);
				bg_bottom = Texture(alarm2["backgrndbottom"]);
				bg_max = Texture(alarm2["backgrndmax"]);
			}

			nl::node btdel = alarm2["BtDelete"];
			if (btdel.size() > 0)
				buttons[Buttons::BT_DELETE] = std::make_unique<MapleButton>(btdel);

			nl::node btmax = alarm2["BtMax"];
			if (btmax.size() > 0)
				buttons[Buttons::BT_MAX] = std::make_unique<MapleButton>(btmax);

			nl::node btmin = alarm2["BtMin"];
			if (btmin.size() > 0)
				buttons[Buttons::BT_MIN] = std::make_unique<MapleButton>(btmin);

			if (!buttons.count(Buttons::BT_Q))
			{
				nl::node btq2 = alarm2["BtQ"];
				if (btq2.size() > 0)
					buttons[Buttons::BT_Q] = std::make_unique<MapleButton>(btq2);
			}

			if (!buttons.count(Buttons::BT_AUTO))
			{
				nl::node btauto2 = alarm2["BtAuto"];
				if (btauto2.size() > 0)
					buttons[Buttons::BT_AUTO] = std::make_unique<MapleButton>(btauto2);
			}
		}

		// === QuestBulb (UIWindowBT.img) ===
		nl::node bulb = nl::nx::ui["UIWindowBT.img"]["QuestBulb"];
		if (bulb)
		{
			nl::node open_node = bulb["BtBulbOpen"];
			if (open_node.size() > 0)
			{
				buttons[Buttons::BT_BULB_OPEN] = std::make_unique<MapleButton>(open_node);
				bulb_open_anim = Animation(open_node["ani"]);
			}

			nl::node close_node = bulb["BtBulbClose"];
			if (close_node.size() > 0)
			{
				buttons[Buttons::BT_BULB_CLOSE] = std::make_unique<MapleButton>(close_node);
				bulb_close_anim = Animation(close_node["ani"]);
			}
		}

		// === QuestMessenger (UIWindow2.img) ===
		nl::node alice = nl::nx::ui["UIWindow2.img"]["QuestMessengerAlice"];
		if (alice)
		{
			messenger_alice_bg = Texture(alice["backgrnd"]);
			messenger_alice_notice = Texture(alice["notice"]);
		}

		nl::node thomas = nl::nx::ui["UIWindow2.img"]["QuestMessengerThomas"];
		if (thomas)
		{
			messenger_thomas_bg = Texture(thomas["backgrnd"]);
			messenger_thomas_notice = Texture(thomas["notice"]);
		}

		// === QuestGuide marks (UIWindow2.img) ===
		nl::node guide = nl::nx::ui["UIWindow2.img"]["QuestGuide"];
		if (guide)
		{
			quest_mark = Animation(guide["QuestMark"]);

			nl::node repeat = guide["RepeatQuestMark"];
			if (repeat)
				repeat_quest_mark = Animation(repeat["forMiniMap"]);

			nl::node high = guide["HighLVQuestMark"];
			if (high)
				high_lv_mark = Animation(high["forMiniMap"]);

			nl::node low = guide["LowLVQuestMark"];
			if (low)
				low_lv_mark = Animation(low["forMiniMap"]);

			nl::node low_repeat = guide["LowLVRepeatQuestMark"];
			if (low_repeat)
				low_lv_repeat_mark = Animation(low_repeat["forMiniMap"]);
		}

		// === StatusBar3/Quest ===
		nl::node sb_quest = nl::nx::ui["StatusBar3.img"]["Quest"];
		if (sb_quest)
		{
			nl::node check = sb_quest["checkArlim"];
			if (check)
			{
				sb_checkarlim_box = Texture(check["box"]);
				sb_checkarlim_check = Texture(check["check"]);
			}

			nl::node qi = sb_quest["quest_info"];
			if (qi)
			{
				sb_quest_info_bg = Texture(qi["backgrnd"]);
				sb_quest_info_bg2 = Texture(qi["backgrnd2"]);
				sb_quest_info_bg3 = Texture(qi["backgrnd3"]);
				sb_quest_info_tip = Texture(qi["tip"]);
			}
		}

		// === Extended QuestIcon (UIWindow2.img/QuestIcon 12-29) ===
		nl::node qicon2 = nl::nx::ui["UIWindow2.img"]["QuestIcon"];
		if (qicon2)
		{
			for (int i = 12; i <= 29; i++)
			{
				nl::node icon_n = qicon2[std::to_string(i)];
				if (icon_n)
					extended_quest_icons.push_back(Animation(icon_n));
				else
					extended_quest_icons.push_back(Animation());
			}
		}

		// Set dimension based on background
		if (bg_max.is_valid())
			dimension = bg_max.get_dimensions();
		else if (bg_min.is_valid())
			dimension = bg_min.get_dimensions();
		else
			dimension = Point<int16_t>(200, 60);

		dragarea = Point<int16_t>(dimension.x(), 15);

		// Auto-track first available quest
		auto_track();
	}

	void UIQuestHelper::draw(float alpha) const
	{
		if (minimized)
		{
			// Draw minimized — just the bulb or min background
			if (bg_min.is_valid())
				bg_min.draw(DrawArgument(position));

			// Draw quest mark indicator
			quest_mark.draw(DrawArgument(position + Point<int16_t>(5, 5)), alpha);

			UIElement::draw_buttons(alpha);
			return;
		}

		// Draw expanded background
		if (bg_max.is_valid())
		{
			bg_max.draw(DrawArgument(position));
		}
		else
		{
			// Fallback: draw scalable pieces
			if (bg_min.is_valid())
				bg_min.draw(DrawArgument(position));
			if (bg_center.is_valid())
				bg_center.draw(DrawArgument(position));
			if (bg_bottom.is_valid())
				bg_bottom.draw(DrawArgument(position));
		}

		// Draw quest info
		if (tracked_questid > 0)
		{
			quest_name.draw(position + Point<int16_t>(25, 5));

			int16_t y = 22;
			for (auto& line : requirement_lines)
			{
				line.draw(position + Point<int16_t>(30, y));
				y += 15;
			}

			if (y <= 22)
				quest_progress.draw(position + Point<int16_t>(30, y));
		}
		else
		{
			static Text no_quest(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "No quest tracked", 170, false);
			no_quest.draw(position + Point<int16_t>(25, 15));
		}

		UIElement::draw_buttons(alpha);

		// Draw bulb notification if active
		if (show_bulb)
			bulb_open_anim.draw(DrawArgument(position + Point<int16_t>(-15, -10)), alpha);
	}

	void UIQuestHelper::toggle_active()
	{
		UIElement::toggle_active();

		if (active)
			auto_track();
	}

	void UIQuestHelper::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	void UIQuestHelper::update()
	{
		UIElement::update();
		quest_mark.update();
		repeat_quest_mark.update();
		high_lv_mark.update();
		low_lv_mark.update();
		low_lv_repeat_mark.update();
		bulb_open_anim.update();
		bulb_close_anim.update();

		for (auto& icon : extended_quest_icons)
			icon.update();
	}

	UIElement::Type UIQuestHelper::get_type() const
	{
		return TYPE;
	}

	void UIQuestHelper::track_quest(int16_t questid)
	{
		tracked_questid = questid;
		refresh_quest_info();
	}

	void UIQuestHelper::auto_track()
	{
		const auto& started = questlog.get_started();

		if (started.empty())
		{
			tracked_questid = -1;
			quest_name = Text();
			quest_progress = Text();
			requirement_lines.clear();
			return;
		}

		// Track the most recently started quest
		tracked_questid = started.rbegin()->first;
		refresh_quest_info();
	}

	void UIQuestHelper::refresh_quest_info()
	{
		requirement_lines.clear();

		if (tracked_questid <= 0)
			return;

		nl::node quest_info = nl::nx::quest["QuestInfo.img"];
		nl::node quest_check = nl::nx::quest["Check.img"];

		std::string qid_str = std::to_string(tracked_questid);

		// Get quest name
		nl::node qnode = quest_info[qid_str];
		std::string name;
		if (qnode)
			name = qnode["name"].get_string();
		if (name.empty())
			name = "Quest " + qid_str;

		quest_name = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::WHITE, name, 170, false);

		// Get progress requirements
		nl::node check = quest_check[qid_str];
		if (!check) return;

		nl::node end_check = check["1"];
		if (!end_check) return;

		const Inventory& inventory = Stage::get().get_player().get_inventory();

		// Item requirements
		for (auto item_node : end_check["item"])
		{
			int32_t itemid = item_node["id"].get_integer();
			int32_t count = item_node["count"].get_integer();
			if (itemid <= 0) continue;

			int16_t have = inventory.get_total_item_count(itemid);
			const ItemData& idata = ItemData::get(itemid);
			std::string item_name = idata.get_name();
			if (item_name.empty())
				item_name = "Item " + std::to_string(itemid);

			std::string line = item_name + " " + std::to_string(have) + "/" + std::to_string(count);
			Color::Name color = (have >= count) ? Color::Name::CHARTREUSE : Color::Name::WHITE;

			requirement_lines.push_back(
				Text(Text::Font::A11M, Text::Alignment::LEFT, color, line, 160, false)
			);
		}

		// Mob requirements
		for (auto mob_node : end_check["mob"])
		{
			int32_t mobid = mob_node["id"].get_integer();
			int32_t count = mob_node["count"].get_integer();
			if (mobid <= 0) continue;

			std::string mob_name = nl::nx::string["Mob.img"][std::to_string(mobid)]["name"].get_string();
			if (mob_name.empty())
				mob_name = "Monster " + std::to_string(mobid);

			// We don't have kill count client-side, just show requirement
			std::string line = mob_name + " 0/" + std::to_string(count);
			requirement_lines.push_back(
				Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, line, 160, false)
			);
		}

		// NPC requirement
		int32_t end_npc = end_check["npc"].get_integer();
		if (end_npc > 0)
		{
			std::string npc_name = nl::nx::string["Npc.img"][std::to_string(end_npc)]["name"].get_string();
			if (!npc_name.empty())
			{
				requirement_lines.push_back(
					Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MALIBU, "Talk to " + npc_name, 160, false)
				);
			}
		}

		// Summary progress text
		if (requirement_lines.empty())
			quest_progress = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "In progress...", 160, false);
		else
			quest_progress = Text();
	}

	Button::State UIQuestHelper::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_Q:
		{
			auto questlog = UI::get().get_element<UIQuestLog>();
			if (questlog)
				questlog->toggle_active();
			break;
		}
		case Buttons::BT_AUTO:
			auto_track();
			break;
		case Buttons::BT_DELETE:
			tracked_questid = -1;
			quest_name = Text();
			quest_progress = Text();
			requirement_lines.clear();
			break;
		case Buttons::BT_MAX:
			minimized = false;
			if (bg_max.is_valid())
				dimension = bg_max.get_dimensions();
			break;
		case Buttons::BT_MIN:
			minimized = true;
			if (bg_min.is_valid())
				dimension = bg_min.get_dimensions();
			break;
		case Buttons::BT_BULB_OPEN:
			show_bulb = false;
			break;
		case Buttons::BT_BULB_CLOSE:
			show_bulb = false;
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
