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
#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIQuestHelper::UIQuestHelper(const QuestLog& ql) :
		UIDragElement<PosQUESTHELPER>(Point<int16_t>(200, 20)), questlog(ql),
		tracked_questid(-1), minimized(false), show_messenger(false)
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

		// QuestBulb removed — not used

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

		// Set dimension — stack all three pieces vertically
		if (bg_min.is_valid())
		{
			int16_t w = bg_min.get_dimensions().x();
			int16_t min_h = bg_min.get_dimensions().y() - bg_min.get_origin().y();
			int16_t center_h = bg_center.is_valid() ? bg_center.get_dimensions().y() : 0;
			int16_t bottom_h = bg_bottom.is_valid() ? bg_bottom.get_dimensions().y() : 0;
			dimension = Point<int16_t>(w, min_h + center_h + bottom_h);
		}
		else
			dimension = Point<int16_t>(200, 60);

		expanded_dimension = dimension;

		dragarea = Point<int16_t>(dimension.x(), 15);

		// Button positions: draw_buttons uses `position` as parentpos, background
		// draws at position - origin. Compensate with -origin + tex_origin.
		Point<int16_t> bg_origin = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
		Point<int16_t> bo = bg_origin * -1;

		auto set_helper_btn = [&](uint16_t id, Point<int16_t> offset) {
			auto it = buttons.find(id);
			if (it != buttons.end() && it->second)
				it->second->set_position(bo + offset + it->second->origin());
		};

		set_helper_btn(Buttons::BT_MIN, Point<int16_t>(dimension.x() - 18, 2));
		set_helper_btn(Buttons::BT_MAX, Point<int16_t>(dimension.x() - 18, 2));
		set_helper_btn(Buttons::BT_AUTO, Point<int16_t>(dimension.x() - 40, 2));
		set_helper_btn(Buttons::BT_Q, Point<int16_t>(dimension.x() - 62, 2));
		// BT_DELETE is drawn manually per-quest, not through draw_buttons
		if (buttons.count(Buttons::BT_DELETE))
		{
			buttons[Buttons::BT_DELETE]->set_position(Point<int16_t>());
			buttons[Buttons::BT_DELETE]->set_active(false);
		}
		// Start expanded: show MIN, hide MAX
		if (buttons.count(Buttons::BT_MAX))
			buttons[Buttons::BT_MAX]->set_active(false);

		// Auto-track first available quest
		auto_track();
	}

	void UIQuestHelper::draw(float alpha) const
	{
		Point<int16_t> content_pos = position;

		// Draw background — bg_min is the title bar, center+bottom stack below it
		if (bg_min.is_valid())
		{
			bg_min.draw(DrawArgument(position));
			content_pos = position - bg_min.get_origin();
		}
		if (!minimized)
		{
			int16_t min_h = bg_min.is_valid() ? (bg_min.get_dimensions().y() - bg_min.get_origin().y()) : 20;
			Point<int16_t> below = position + Point<int16_t>(0, min_h);

			// Calculate content height based on tracked quest info
			int16_t content_h = 20; // minimum content area
			if (tracked_questid > 0)
			{
				content_h = 22 + 16; // quest name line + gap
				if (!requirement_lines.empty())
					content_h += static_cast<int16_t>(requirement_lines.size()) * 16;
				else
					content_h += 16; // progress text
				content_h += 8; // bottom padding
			}

			// Tile bg_center to fill the content area
			if (bg_center.is_valid())
			{
				int16_t tile_h = bg_center.get_dimensions().y();
				if (tile_h > 0)
				{
					int16_t drawn = 0;
					while (drawn < content_h)
					{
						bg_center.draw(DrawArgument(below + Point<int16_t>(0, drawn) + bg_center.get_origin()));
						drawn += tile_h;
					}
					// Draw bg_bottom at end of tiled area
					if (bg_bottom.is_valid())
						bg_bottom.draw(DrawArgument(below + Point<int16_t>(0, drawn) + bg_bottom.get_origin()));
				}
			}
		}

		// Draw title: "Quest Helper (tracked/5)"
		{
			int16_t tracked_count = (tracked_questid > 0) ? 1 : 0;
			std::string title = "Quest Helper (" + std::to_string(tracked_count) + "/5)";
			Text title_text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK, title, 170, false);
			title_text.draw(content_pos + Point<int16_t>(8, 1));
		}

		// Always draw buttons (title bar buttons stay visible even when minimized)
		UIElement::draw_buttons(alpha);

		if (minimized)
			return;

		// Draw tracked quest info below title
		if (tracked_questid > 0)
		{
			int16_t y = 20;
			quest_name.draw(content_pos + Point<int16_t>(10, y));

			// Draw BtDelete button to untrack this quest (right side of quest entry)
			quest_untrack_pos = content_pos + Point<int16_t>(dimension.x() - 18, y - 2);
			{
				auto del_it = buttons.find(Buttons::BT_DELETE);
				if (del_it != buttons.end() && del_it->second)
				{
					del_it->second->set_active(true);
					del_it->second->draw(quest_untrack_pos);
					del_it->second->set_active(false);
				}
			}

			y += 18;

			for (auto& line : requirement_lines)
			{
				line.draw(content_pos + Point<int16_t>(15, y));
				y += 15;
			}

			if (requirement_lines.empty())
				quest_progress.draw(content_pos + Point<int16_t>(15, y));
		}
		else
		{
			static Text no_quest(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "No quest tracked", 170, false);
			no_quest.draw(content_pos + Point<int16_t>(15, 22));
		}

		// Draw quest messenger notification below the helper
		if (show_messenger && tracked_questid > 0)
		{
			Point<int16_t> msg_pos = position + Point<int16_t>(0, dimension.y() + 5);
			if (messenger_alice_bg.is_valid())
			{
				messenger_alice_bg.draw(DrawArgument(msg_pos));
				if (messenger_alice_notice.is_valid())
					messenger_alice_notice.draw(DrawArgument(msg_pos));
			}
			else if (messenger_thomas_bg.is_valid())
			{
				messenger_thomas_bg.draw(DrawArgument(msg_pos));
				if (messenger_thomas_notice.is_valid())
					messenger_thomas_notice.draw(DrawArgument(msg_pos));
			}
		}
	}

	void UIQuestHelper::toggle_active()
	{
		UIElement::toggle_active();

		if (active)
		{
			// Always open expanded
			minimized = false;
			dimension = expanded_dimension;
			if (buttons.count(Buttons::BT_MIN))
				buttons[Buttons::BT_MIN]->set_active(true);
			if (buttons.count(Buttons::BT_MAX))
				buttons[Buttons::BT_MAX]->set_active(false);
			if (buttons.count(Buttons::BT_AUTO))
				buttons[Buttons::BT_AUTO]->set_active(true);
			auto_track();
		}
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

		for (auto& icon : extended_quest_icons)
			icon.update();
	}

	Cursor::State UIQuestHelper::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		// Check per-quest BtDelete untrack button
		if (tracked_questid > 0 && !minimized)
		{
			auto del_it = buttons.find(Buttons::BT_DELETE);
			if (del_it != buttons.end() && del_it->second)
			{
				// BtDelete is drawn at quest_untrack_pos as parentpos
				Rectangle<int16_t> del_bounds = del_it->second->bounds(quest_untrack_pos);
				if (del_bounds.contains(cursorpos))
				{
					if (clicking)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						tracked_questid = -1;
						quest_name = Text();
						quest_progress = Text();
						requirement_lines.clear();
						recalc_dimension();
						return Cursor::State::IDLE;
					}
					else
					{
						del_it->second->set_state(Button::State::MOUSEOVER);
						return Cursor::State::CANCLICK;
					}
				}
				else
				{
					if (del_it->second->get_state() == Button::State::MOUSEOVER)
						del_it->second->set_state(Button::State::NORMAL);
				}
			}
		}

		// Handle button clicks — use content_pos for hit testing since that's where buttons visually are
		Point<int16_t> bg_origin = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
		Point<int16_t> content_pos = position - bg_origin;

		// Button visual positions (must match constructor set_helper_btn offsets)
		struct BtnHit { uint16_t id; int16_t x; };
		BtnHit btn_hits[] = {
			{ Buttons::BT_MIN, dimension.x() - 18 },
			{ Buttons::BT_MAX, dimension.x() - 18 },
			{ Buttons::BT_AUTO, dimension.x() - 40 },
			{ Buttons::BT_Q, dimension.x() - 62 },
		};

		for (auto& bh : btn_hits)
		{
			auto it = buttons.find(bh.id);
			if (it == buttons.end() || !it->second || !it->second->is_active())
				continue;

			// Hit area: 20x16 at the visual button position
			Rectangle<int16_t> hit(
				content_pos + Point<int16_t>(bh.x, 0),
				content_pos + Point<int16_t>(bh.x + 20, 16)
			);

			if (hit.contains(cursorpos))
			{
				if (clicking)
				{
					Sound(Sound::Name::BUTTONCLICK).play();
					it->second->set_state(button_pressed(bh.id));
					return Cursor::State::IDLE;
				}
				else
				{
					if (it->second->get_state() == Button::State::NORMAL)
						Sound(Sound::Name::BUTTONOVER).play();
					it->second->set_state(Button::State::MOUSEOVER);
					return Cursor::State::CANCLICK;
				}
			}
			else if (it->second->get_state() == Button::State::MOUSEOVER)
			{
				it->second->set_state(Button::State::NORMAL);
			}
		}

		// Handle dragging (skip base class to avoid its button processing)
		if (dragged)
		{
			if (clicking)
			{
				position = cursorpos - cursoroffset;
				return Cursor::State::CLICKING;
			}
			else
			{
				dragged = false;
				Setting<PosQUESTHELPER>::get().save(position);
			}
		}
		else if (clicking)
		{
			// Only start drag if in drag area
			Point<int16_t> bg_org = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
			Point<int16_t> drag_tl = position - bg_org;
			Rectangle<int16_t> drag_bounds(drag_tl, drag_tl + Point<int16_t>(dragarea.x(), dragarea.y()));
			if (drag_bounds.contains(cursorpos))
			{
				cursoroffset = cursorpos - position;
				dragged = true;
				return Cursor::State::CLICKING;
			}
		}

		return Cursor::State::IDLE;
	}

	UIElement::Type UIQuestHelper::get_type() const
	{
		return TYPE;
	}

	bool UIQuestHelper::is_in_range(Point<int16_t> cursorpos) const
	{
		// Background draws at position - bg_min origin
		Point<int16_t> bg_origin = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
		Point<int16_t> topleft = position - bg_origin;
		Rectangle<int16_t> bounds(topleft, topleft + dimension);
		return bounds.contains(cursorpos);
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
			recalc_dimension();
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

		quest_name = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK, name, 140, false);

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
			Color::Name color = (have >= count) ? Color::Name::CHARTREUSE : Color::Name::BLACK;

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
				Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, line, 160, false)
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
			quest_progress = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "In progress...", 160, false);
		else
			quest_progress = Text();

		// Recalculate dimension based on content
		recalc_dimension();
	}

	void UIQuestHelper::recalc_dimension()
	{
		if (!bg_min.is_valid())
			return;

		int16_t w = bg_min.get_dimensions().x();
		int16_t min_h = bg_min.get_dimensions().y() - bg_min.get_origin().y();

		if (tracked_questid > 0)
		{
			int16_t content_h = 22 + 16; // quest name + gap
			if (!requirement_lines.empty())
				content_h += static_cast<int16_t>(requirement_lines.size()) * 16;
			else
				content_h += 16;
			content_h += 8; // padding

			// Tile to nearest bg_center tile height
			int16_t tile_h = bg_center.is_valid() ? bg_center.get_dimensions().y() : 1;
			int16_t tiles = (content_h + tile_h - 1) / tile_h;
			int16_t tiled_h = tiles * tile_h;
			int16_t bottom_h = bg_bottom.is_valid() ? bg_bottom.get_dimensions().y() : 0;

			expanded_dimension = Point<int16_t>(w, min_h + tiled_h + bottom_h);
		}
		else
		{
			// Minimal content area for "No quest tracked"
			int16_t tile_h = bg_center.is_valid() ? bg_center.get_dimensions().y() : 1;
			int16_t tiles = (28 + tile_h - 1) / tile_h;
			int16_t tiled_h = tiles * tile_h;
			int16_t bottom_h = bg_bottom.is_valid() ? bg_bottom.get_dimensions().y() : 0;

			expanded_dimension = Point<int16_t>(w, min_h + tiled_h + bottom_h);
		}

		if (!minimized)
			dimension = expanded_dimension;
	}

	Button::State UIQuestHelper::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_Q:
		{
			UI::get().emplace<UIQuestLog>(
				Stage::get().get_player().get_quests()
			);
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
			recalc_dimension();
			break;
		case Buttons::BT_MAX:
			minimized = false;
			dimension = expanded_dimension;
			if (buttons.count(Buttons::BT_MIN))
				buttons[Buttons::BT_MIN]->set_active(true);
			if (buttons.count(Buttons::BT_MAX))
				buttons[Buttons::BT_MAX]->set_active(false);
			break;
		case Buttons::BT_MIN:
			minimized = true;
			if (bg_min.is_valid())
				dimension = bg_min.get_dimensions();
			if (buttons.count(Buttons::BT_MIN))
				buttons[Buttons::BT_MIN]->set_active(false);
			if (buttons.count(Buttons::BT_MAX))
				buttons[Buttons::BT_MAX]->set_active(true);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
