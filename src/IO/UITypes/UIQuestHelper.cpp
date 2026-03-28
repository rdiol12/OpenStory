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
#include "../../Data/QuestData.h"
#include "../../Data/StringData.h"
#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	QuestDragState UIQuestHelper::quest_drag;

	UIQuestHelper::UIQuestHelper(const QuestLog& ql) :
		UIDragElement<PosQUESTHELPER>(Point<int16_t>(200, 20)), questlog(ql),
		minimized(false), show_messenger(false), hovered_close_questid(-1),
		reorder_drag_index(-1)
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

		// === Per-quest close button textures ===
		nl::node btdel_src = alarm2 ? alarm2["BtDelete"] : nl::node();
		if (!btdel_src)
			btdel_src = alarm1 ? alarm1["BtDelete"] : nl::node();
		if (btdel_src)
		{
			close_btn_normal = Texture(btdel_src["normal"]["0"]);
			close_btn_mouseover = Texture(btdel_src["mouseOver"]["0"]);
			close_btn_pressed = Texture(btdel_src["pressed"]["0"]);
		}

		// === QuestMessenger ===
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

		// === QuestGuide marks ===
		nl::node guide = nl::nx::ui["UIWindow2.img"]["QuestGuide"];
		if (guide)
		{
			quest_mark = Animation(guide["QuestMark"]);

			nl::node repeat = guide["RepeatQuestMark"];
			if (repeat) repeat_quest_mark = Animation(repeat["forMiniMap"]);

			nl::node high = guide["HighLVQuestMark"];
			if (high) high_lv_mark = Animation(high["forMiniMap"]);

			nl::node low = guide["LowLVQuestMark"];
			if (low) low_lv_mark = Animation(low["forMiniMap"]);

			nl::node low_repeat = guide["LowLVRepeatQuestMark"];
			if (low_repeat) low_lv_repeat_mark = Animation(low_repeat["forMiniMap"]);
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

		// === Extended QuestIcon ===
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

		// Set dimension
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

		// Button positions
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

		if (buttons.count(Buttons::BT_DELETE))
		{
			buttons[Buttons::BT_DELETE]->set_position(Point<int16_t>());
			buttons[Buttons::BT_DELETE]->set_active(false);
		}

		if (buttons.count(Buttons::BT_MAX))
			buttons[Buttons::BT_MAX]->set_active(false);

		title_text = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK, "", 170, false);
		no_quest_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "No quest tracked", 170, false);
		drag_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK, "", 0, false);

		auto_track();
	}

	int16_t UIQuestHelper::get_quest_entry_height(const TrackedQuest& tq) const
	{
		int16_t h = 18; // header line with bold name
		if (!tq.collapsed)
		{
			if (!tq.requirements.empty())
				h += static_cast<int16_t>(tq.requirements.size()) * 15;
			else
				h += 15; // "In progress..."
		}
		return h;
	}

	int16_t UIQuestHelper::draw_quest_entry(const TrackedQuest& tq, Point<int16_t> pos, float alpha) const
	{
		int16_t y = 0;

		// Draw quest name (bold dark) — click to collapse/expand
		tq.name.draw(pos + Point<int16_t>(0, y));

		// Draw X button (BtDelete sprite) right after quest name
		if (close_btn_normal.is_valid())
		{
			int16_t name_w = tq.name.width();
			Point<int16_t> x_btn_pos = pos + Point<int16_t>(name_w + 4, y);
			bool hovered = (tq.questid == hovered_close_questid);
			const Texture& btn_tex = hovered ? close_btn_mouseover : close_btn_normal;
			btn_tex.draw(DrawArgument(x_btn_pos));
		}
		y += 18;

		if (!tq.collapsed)
		{
			if (!tq.requirements.empty())
			{
				for (auto& line : tq.requirements)
				{
					line.draw(pos + Point<int16_t>(4, y));
					y += 15;
				}
			}
			else
			{
				tq.progress.draw(pos + Point<int16_t>(4, y));
				y += 15;
			}
		}

		return y;
	}

	void UIQuestHelper::draw(float alpha) const
	{
		Point<int16_t> content_pos = position;

		if (bg_min.is_valid())
		{
			bg_min.draw(DrawArgument(position));
			content_pos = position - bg_min.get_origin();
		}

		if (!minimized)
		{
			int16_t min_h = bg_min.is_valid() ? (bg_min.get_dimensions().y() - bg_min.get_origin().y()) : 20;
			Point<int16_t> below = position + Point<int16_t>(0, min_h);

			// Calculate total content height
			int16_t content_h = 8; // top padding
			for (auto& tq : tracked_quests)
				content_h += get_quest_entry_height(tq) + 8; // 8px spacing between quests

			if (tracked_quests.empty())
				content_h = 28;

			content_h = std::max(content_h, (int16_t)20);

			// Tile bg_center
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
					if (bg_bottom.is_valid())
						bg_bottom.draw(DrawArgument(below + Point<int16_t>(0, drawn) + bg_bottom.get_origin()));
				}
			}
		}

		// Draw title
		{
			int16_t count = static_cast<int16_t>(tracked_quests.size());
			std::string title = "Quest Helper (" + std::to_string(count) + "/" + std::to_string(MAX_TRACKED) + ")";
			title_text.change_text(title);
			title_text.draw(content_pos + Point<int16_t>(8, 1));
		}

		UIElement::draw_buttons(alpha);

		if (minimized)
			return;

		// Draw quest entries
		quest_hit_areas.clear();

		if (tracked_quests.empty())
		{
			no_quest_text.draw(content_pos + Point<int16_t>(15, 22));
		}
		else
		{
			int16_t y = 22;
			for (auto& tq : tracked_quests)
			{
				Point<int16_t> entry_pos = content_pos + Point<int16_t>(8, y);
				int16_t entry_h = draw_quest_entry(tq, entry_pos, alpha);

				// Store hit areas for click handling
				QuestHitArea hit;
				hit.questid = tq.questid;
				// X button area right after quest name
				int16_t btn_w = close_btn_normal.is_valid() ? close_btn_normal.get_dimensions().x() : 12;
				int16_t btn_h = close_btn_normal.is_valid() ? close_btn_normal.get_dimensions().y() : 14;
				int16_t name_w = tq.name.width();
				int16_t x_btn_x = name_w + 4;
				hit.close_btn = Rectangle<int16_t>(
					entry_pos + Point<int16_t>(x_btn_x, 0),
					entry_pos + Point<int16_t>(x_btn_x + btn_w, btn_h)
				);
				// Header area for collapse toggle: quest name
				hit.header_area = Rectangle<int16_t>(
					entry_pos,
					entry_pos + Point<int16_t>(name_w, 16)
				);
				quest_hit_areas.push_back(hit);

				y += entry_h + 8; // 8px spacing between quests
			}
		}

		// Draw drag overlay — quest being dragged from quest log or reordered
		if (quest_drag.active)
		{
			drag_text.change_text(quest_drag.quest_name);
			drag_text.draw(DrawArgument(quest_drag.cursor_pos + Point<int16_t>(12, -8)));
		}
		else if (reorder_drag_index >= 0 && reorder_drag_index < static_cast<int16_t>(tracked_quests.size()))
		{
			int16_t dx = reorder_cursor_pos.x() - reorder_start_pos.x();
			int16_t dy = reorder_cursor_pos.y() - reorder_start_pos.y();
			if ((dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD))
			{
				drag_text.change_text(tracked_quests[reorder_drag_index].name.get_text());
				drag_text.draw(DrawArgument(reorder_cursor_pos + Point<int16_t>(12, -8)));
			}
		}
	}

	void UIQuestHelper::toggle_active()
	{
		UIElement::toggle_active();

		if (active)
		{
			minimized = false;
			dimension = expanded_dimension;
			if (buttons.count(Buttons::BT_MIN))
				buttons[Buttons::BT_MIN]->set_active(true);
			if (buttons.count(Buttons::BT_MAX))
				buttons[Buttons::BT_MAX]->set_active(false);
			if (buttons.count(Buttons::BT_AUTO))
				buttons[Buttons::BT_AUTO]->set_active(true);
			if (tracked_quests.empty())
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
		// While quest is being dragged from quest log, show grabbing cursor
		if (quest_drag.active)
		{
			quest_drag.cursor_pos = cursorpos;
			return Cursor::State::GRABBING;
		}

		// Handle internal reorder drag
		if (reorder_drag_index >= 0)
		{
			reorder_cursor_pos = cursorpos;
			int16_t dx = cursorpos.x() - reorder_start_pos.x();
			int16_t dy = cursorpos.y() - reorder_start_pos.y();
			bool moved = (dx * dx + dy * dy) > (DRAG_THRESHOLD * DRAG_THRESHOLD);

			if (!clicking)
			{
				int16_t idx = reorder_drag_index;
				reorder_drag_index = -1;

				if (!moved)
				{
					// Short click — collapse/expand
					if (idx >= 0 && idx < static_cast<int16_t>(tracked_quests.size()))
					{
						tracked_quests[idx].collapsed = !tracked_quests[idx].collapsed;
						recalc_dimension();
					}
					return Cursor::State::IDLE;
				}

				// Find drop target by cursor Y position
				int16_t drop_index = -1;
				for (size_t i = 0; i < quest_hit_areas.size(); i++)
				{
					if (quest_hit_areas[i].header_area.contains(cursorpos) ||
						quest_hit_areas[i].close_btn.contains(cursorpos))
					{
						drop_index = static_cast<int16_t>(i);
						break;
					}
				}

				if (drop_index >= 0 && drop_index != idx &&
					drop_index < static_cast<int16_t>(tracked_quests.size()))
				{
					TrackedQuest moved_q = std::move(tracked_quests[idx]);
					tracked_quests.erase(tracked_quests.begin() + idx);
					tracked_quests.insert(tracked_quests.begin() + drop_index, std::move(moved_q));
					recalc_dimension();
				}

				return Cursor::State::IDLE;
			}

			return moved ? Cursor::State::GRABBING : Cursor::State::CANCLICK;
		}

		// Check per-quest hit areas (X button and collapse/reorder)
		hovered_close_questid = -1;
		if (!minimized)
		{
			for (size_t i = 0; i < quest_hit_areas.size(); i++)
			{
				auto& hit = quest_hit_areas[i];

				// X button
				if (hit.close_btn.contains(cursorpos))
				{
					hovered_close_questid = hit.questid;
					if (clicking)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						untrack_quest(hit.questid);
						return Cursor::State::IDLE;
					}
					return Cursor::State::CANCLICK;
				}
				// Header: click to collapse, hold and drag to reorder
				if (hit.header_area.contains(cursorpos))
				{
					if (clicking)
					{
						// Start reorder drag (or click-to-collapse if no movement)
						reorder_drag_index = static_cast<int16_t>(i);
						reorder_start_pos = cursorpos;
						reorder_cursor_pos = cursorpos;
						return Cursor::State::CLICKING;
					}
					return Cursor::State::CANCLICK;
				}
			}
		}

		// Title bar buttons
		Point<int16_t> bg_origin = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
		Point<int16_t> content_pos = position - bg_origin;

		struct BtnHit { uint16_t id; int16_t x; };
		BtnHit btn_hits[] = {
			{ Buttons::BT_MIN, static_cast<int16_t>(dimension.x() - 18) },
			{ Buttons::BT_MAX, static_cast<int16_t>(dimension.x() - 18) },
			{ Buttons::BT_AUTO, static_cast<int16_t>(dimension.x() - 40) },
			{ Buttons::BT_Q, static_cast<int16_t>(dimension.x() - 62) },
		};

		for (auto& bh : btn_hits)
		{
			auto it = buttons.find(bh.id);
			if (it == buttons.end() || !it->second || !it->second->is_active())
				continue;

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

		// Window dragging
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
		Point<int16_t> bg_origin = bg_min.is_valid() ? bg_min.get_origin() : Point<int16_t>();
		Point<int16_t> topleft = position - bg_origin;
		Rectangle<int16_t> bounds(topleft, topleft + dimension);
		return bounds.contains(cursorpos);
	}

	void UIQuestHelper::track_quest(int16_t questid)
	{
		if (questid <= 0)
			return;

		// Don't track duplicates
		for (auto& tq : tracked_quests)
			if (tq.questid == questid)
				return;

		// Max limit
		if (static_cast<int>(tracked_quests.size()) >= MAX_TRACKED)
			return;

		TrackedQuest tq;
		tq.questid = questid;
		tq.collapsed = false;
		refresh_quest_info(tq);
		tracked_quests.push_back(std::move(tq));
		recalc_dimension();
	}

	void UIQuestHelper::untrack_quest(int16_t questid)
	{
		tracked_quests.erase(
			std::remove_if(tracked_quests.begin(), tracked_quests.end(),
				[questid](const TrackedQuest& tq) { return tq.questid == questid; }),
			tracked_quests.end()
		);
		recalc_dimension();
	}

	void UIQuestHelper::auto_track()
	{
		const auto& started = questlog.get_started();

		if (started.empty())
		{
			tracked_quests.clear();
			recalc_dimension();
			return;
		}

		// Track all started quests (up to MAX_TRACKED) if none are tracked
		if (tracked_quests.empty())
		{
			int count = 0;
			for (auto it = started.rbegin(); it != started.rend() && count < MAX_TRACKED; ++it, ++count)
			{
				TrackedQuest tq;
				tq.questid = it->first;
				tq.collapsed = false;
				refresh_quest_info(tq);
				tracked_quests.push_back(std::move(tq));
			}
			recalc_dimension();
		}
	}

	void UIQuestHelper::refresh_quest_info(TrackedQuest& tq)
	{
		tq.requirements.clear();

		if (tq.questid <= 0)
			return;

		const QuestData& qdata_info = QuestData::get(tq.questid);

		// Quest name — bold dark
		std::string name = qdata_info.get_name();
		if (name.empty())
			name = "Quest " + std::to_string(tq.questid);

		tq.name = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::MINESHAFT, name, 0, false);

		if (!qdata_info.is_valid())
			return;

		const Inventory& inventory = Stage::get().get_player().get_inventory();

		// Item requirements
		for (auto& item_req : qdata_info.get_item_reqs(false))
		{
			if (item_req.itemid <= 0) continue;

			int16_t have = inventory.get_total_item_count(item_req.itemid);
			const ItemData& idata = ItemData::get(item_req.itemid);
			std::string item_name = idata.get_name();
			if (item_name.empty())
				item_name = "Item " + std::to_string(item_req.itemid);

			std::string line = item_name + " " + std::to_string(have) + "/" + std::to_string(item_req.count);
			Color::Name color = Color::Name::BLACK;

			tq.requirements.push_back(
				Text(Text::Font::A11M, Text::Alignment::LEFT, color, line, 145, false)
			);
		}

		// Mob requirements — parse progress from quest data string
		// v83 format: each mob has a 3-digit zero-padded kill count in the quest data string
		std::string qdata;
		{
			auto it = questlog.get_started().find(tq.questid);
			if (it != questlog.get_started().end())
				qdata = it->second;
		}

		int mob_idx = 0;
		for (auto& mob_req : qdata_info.get_mob_reqs(false))
		{
			if (mob_req.mobid <= 0) continue;

			// Parse kill count from quest data string (3 chars per mob)
			int32_t killed = 0;
			size_t offset = static_cast<size_t>(mob_idx) * 3;
			if (offset + 3 <= qdata.size())
			{
				try { killed = std::stoi(qdata.substr(offset, 3)); }
				catch (...) { killed = 0; }
			}

			std::string mob_name = mob_req.name;
			if (mob_name.empty())
				mob_name = StringData::get_mob_name(mob_req.mobid);
			if (mob_name.empty())
				mob_name = "Monster " + std::to_string(mob_req.mobid);

			std::string line = mob_name + " " + std::to_string(killed) + "/" + std::to_string(mob_req.count);
			Color::Name color = Color::Name::BLACK;

			tq.requirements.push_back(
				Text(Text::Font::A11M, Text::Alignment::LEFT, color, line, 145, false)
			);
			mob_idx++;
		}

		// NPC requirement
		int32_t end_npc = qdata_info.get_npc(false);
		if (end_npc > 0)
		{
			std::string npc_name = StringData::get_npc_name(end_npc);
			if (!npc_name.empty())
			{
				tq.requirements.push_back(
					Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Talk to " + npc_name, 145, false)
				);
			}
		}

		// Progress fallback
		if (tq.requirements.empty())
			tq.progress = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "In progress...", 145, false);
		else
			tq.progress = Text();
	}

	void UIQuestHelper::refresh_all()
	{
		for (auto& tq : tracked_quests)
			refresh_quest_info(tq);
		recalc_dimension();
	}

	void UIQuestHelper::recalc_dimension()
	{
		if (!bg_min.is_valid())
			return;

		int16_t w = bg_min.get_dimensions().x();
		int16_t min_h = bg_min.get_dimensions().y() - bg_min.get_origin().y();

		int16_t content_h = 8; // top padding
		if (!tracked_quests.empty())
		{
			for (auto& tq : tracked_quests)
				content_h += get_quest_entry_height(tq) + 8; // 8px spacing
		}
		else
		{
			content_h = 28;
		}

		content_h = std::max(content_h, (int16_t)20);

		int16_t tile_h = bg_center.is_valid() ? bg_center.get_dimensions().y() : 1;
		int16_t tiles = (content_h + tile_h - 1) / tile_h;
		int16_t tiled_h = tiles * tile_h;
		int16_t bottom_h = bg_bottom.is_valid() ? bg_bottom.get_dimensions().y() : 0;

		expanded_dimension = Point<int16_t>(w, min_h + tiled_h + bottom_h);

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
			tracked_quests.clear();
			auto_track();
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
