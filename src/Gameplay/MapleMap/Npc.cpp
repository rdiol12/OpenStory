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
#include "Npc.h"

#include "../Stage.h"
#include "../MapleTVBroadcast.h"
#include "../../Character/QuestLog.h"
#include "../../Graphics/Text.h"
#include "../../Net/NpcResponseTracker.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace {
	// NoLifeNx's get_integer() on a string node uses std::stoll internally and
	// throws "invalid stoll argument" for non-numeric strings (e.g. v83 quest
	// Check.img entries where fields like startscript hold a script name).
	// Use these helpers instead of raw get_integer() when reading WZ fields
	// whose type may vary.
	int64_t safe_int(const nl::node& n)
	{
		if (!n) return 0;
		try { return n.get_integer(); } catch (...) {}
		return 0;
	}

	bool is_truthy(const nl::node& n)
	{
		if (!n) return false;
		try { if (n.get_integer() != 0) return true; } catch (...) {}
		try { if (!n.get_string().empty()) return true; } catch (...) {}
		return false;
	}
}

namespace ms
{
	// Static quest mark animations
	bool Npc::marks_initialized = false;
	Animation Npc::mark_available;
	Animation Npc::mark_in_progress;
	Animation Npc::mark_complete;
	Animation Npc::mark_repeat;
	Animation Npc::mark_low_level;
	Animation Npc::mark_high_level;

	void Npc::init_quest_marks()
	{
		if (marks_initialized)
			return;

		// Primary: UIWindow.img/QuestIcon/0 (animated lightbulb in speech bubble)
		nl::node quest_icon_src = nl::nx::ui["UIWindow.img"]["QuestIcon"];
		nl::node qi0 = quest_icon_src["0"];
		if (qi0 && qi0.size() > 0)
			mark_available = Animation(qi0);

		// In-progress: QuestIcon/1, Complete: QuestIcon/2
		nl::node qi1 = quest_icon_src["1"];
		if (qi1 && qi1.size() > 0)
			mark_in_progress = Animation(qi1);

		nl::node qi2 = quest_icon_src["2"];
		if (qi2 && qi2.size() > 0)
			mark_complete = Animation(qi2);

		// Fallback to MapHelper.img/quest
		nl::node quest_marks = nl::nx::map["MapHelper.img"]["quest"];

		if (!mark_available.get_dimensions().x())
		{
			nl::node chat_next = quest_marks["chatNext"];
			if (chat_next && chat_next.size() > 0)
				mark_available = Animation(chat_next);
		}
		if (!mark_in_progress.get_dimensions().x())
		{
			nl::node chat_self = quest_marks["chatSelf"];
			if (chat_self && chat_self.size() > 0)
				mark_in_progress = Animation(chat_self);
		}
		if (!mark_complete.get_dimensions().x())
		{
			nl::node chat_complete = quest_marks["chatComplete"];
			if (chat_complete && chat_complete.size() > 0)
				mark_complete = Animation(chat_complete);
		}

		// Final fallback to UIWindow2.img/QuestGuide
		if (!mark_available.get_dimensions().x())
		{
			nl::node guide = nl::nx::ui["UIWindow2.img"]["QuestGuide"];
			nl::node qmark = guide["QuestMark"];
			if (qmark && qmark.size() > 0)
				mark_available = Animation(qmark);
			if (!mark_in_progress.get_dimensions().x() && qmark)
				mark_in_progress = Animation(qmark);
			if (!mark_complete.get_dimensions().x() && qmark)
				mark_complete = Animation(qmark);
		}

		marks_initialized = true;
	}

	Npc::Npc(int32_t id, int32_t o, bool fl, uint16_t f, bool cnt, Point<int16_t> position) :
		MapObject(o), quest_mark_type(QuestMarkType::NONE)
	{
		std::string strid = std::to_string(id);
		if (strid.size() < 7)
			strid.insert(0, 7 - strid.size(), '0');
		strid.append(".img");

		nl::node src = nl::nx::npc[strid];
		nl::node strsrc = nl::nx::string["Npc.img"][std::to_string(id)];

		std::string link = (std::string)src["info"]["link"];

		if (link.size() > 0)
		{
			link.append(".img");
			src = nl::nx::npc[link];
		}

		nl::node info = src["info"];

		hidename = info["hideName"].get_bool();
		mouseonly = info["talkMouseOnly"].get_bool();
		scripted = info["script"].size() > 0 || info["shop"].get_bool();

		for (auto npcnode : src)
		{
			std::string state = npcnode.name();

			if (state != "info")
			{
				animations[state] = npcnode;
				states.push_back(state);
			}

			for (auto speaknode : npcnode["speak"])
				lines[state].push_back((std::string)strsrc[speaknode.get_string()]);
		}

		name = (std::string)strsrc["name"];
		func = (std::string)strsrc["func"];

		namelabel = Text(Text::Font::A13B, Text::Alignment::CENTER, Color::Name::YELLOW, Text::Background::NAMETAG, name);
		funclabel = Text(Text::Font::A13B, Text::Alignment::CENTER, Color::Name::YELLOW, Text::Background::NAMETAG, func);

		npcid = id;
		flip = !fl;
		control = cnt;
		stance = "stand";

		phobj.fhid = f;
		set_position(position);

		// Initialize shared marks if needed
		init_quest_marks();

		// Determine quest mark for this NPC
		update_quest_mark();
	}

	void Npc::draw(double viewx, double viewy, float alpha) const
	{
		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);

		if (animations.count(stance))
			animations.at(stance).draw(DrawArgument(absp, flip), alpha);

		if (!hidename)
		{
			// If ever changing code for namelabel confirm placements with map 10000
			namelabel.draw(absp + Point<int16_t>(0, -4));
			funclabel.draw(absp + Point<int16_t>(0, 18));
		}

		// Draw quest mark above NPC
		if (quest_mark_type != QuestMarkType::NONE)
		{
			// absp is at the NPC's feet; offset up to place mark above head
			// Use the current animation's origin to find head position
			auto& anim = animations.count(stance) ? animations.at(stance) : animations.begin()->second;
			Point<int16_t> origin = anim.get_origin();
			// origin.y() is how far above feet the sprite extends
			Point<int16_t> mark_pos = absp + Point<int16_t>(22, -origin.y() - 10);
			quest_mark_anim.draw(DrawArgument(mark_pos), alpha);
		}

		// MapleTV NPCs display the current world-wide broadcast on their
		// screen. v83 uses several id bands for per-town sets:
		//   9201xxx — New Leaf City (9201066)
		//   9250020..9250100 — classic towns (Aquarium 23, Henesys 42,
		//     Kerning 43, Ellinia 44, Perion 45, Orbis 46, Ludibrium 26,
		//     El Nath 24, …)
		//   9270000..9270999 — later variants (Amoria 00, Lith Harbor 01,
		//     Sleepywood 02, Omega 03, Korean Folk Town 04, Murung 06,
		//     Bak Cho 07, …).
		const bool is_tv =
			(npcid == 9201066) ||
			(npcid >= 9250020 && npcid <= 9250100) ||
			(npcid >= 9270000 && npcid < 9271000);
		if (is_tv)
		{
			auto& b = MapleTVBroadcast::get();

			// The NPC's info node exposes MapleTVmsgX / MapleTVmsgY — the
			// exact pixel offset (relative to the NPC's feet) where the
			// broadcast message should be anchored. Values are packed as
			// short so a raw uint16 > 32767 decodes to a negative int16_t.
#ifdef USE_NX
			std::string strid = std::to_string(npcid);
			if (strid.size() < 7)
				strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");
			nl::node info = nl::nx::npc[strid]["info"];

			auto read_short = [](nl::node n) -> int16_t
			{
				if (!n) return 0;
				int64_t v = n.get_integer();
				return static_cast<int16_t>(static_cast<uint16_t>(v));
			};
			int16_t msg_x = read_short(info["MapleTVmsgX"]);
			int16_t msg_y = read_short(info["MapleTVmsgY"]);
#else
			int16_t msg_x = -200;
			int16_t msg_y = -350;
#endif

			int16_t screen_cx = absp.x() + msg_x;
			int16_t screen_top_y = absp.y() + msg_y;
			int16_t screen_w = 180;

			// Diagnostic marker on every MapleTV NPC so we can confirm the
			// id detection + draw path even when no broadcast is active.
			{
				std::string tag = "TV #" + std::to_string(npcid)
					+ (b.active() ? " [LIVE]" : " [idle]");
				Text marker(Text::Font::A11M, Text::Alignment::CENTER,
					b.active() ? Color::Name::YELLOW : Color::Name::LIGHTGREEN,
					tag, static_cast<uint16_t>(screen_w));
				marker.draw(Point<int16_t>(screen_cx, screen_top_y - 14));
			}

			if (b.active())
			{
				int16_t y = screen_top_y;
				if (!b.sender_name().empty())
				{
					Text hdr(Text::Font::A12B, Text::Alignment::CENTER,
						Color::Name::WHITE, b.sender_name(), static_cast<uint16_t>(screen_w));
					hdr.draw(Point<int16_t>(screen_cx, y));
					y += 14;
				}
				for (const std::string& ln : b.lines())
				{
					if (ln.empty()) continue;
					Text t(Text::Font::A11M, Text::Alignment::CENTER,
						Color::Name::WHITE, ln, static_cast<uint16_t>(screen_w));
					t.draw(Point<int16_t>(screen_cx, y));
					y += 12;
				}
			}
		}
	}

	int8_t Npc::update(const Physics& physics)
	{
		if (!active)
			return phobj.fhlayer;

		physics.move_object(phobj);

		// When walking, flip direction if physics reports we hit a foothold
		// edge (move_object clears TURNATEDGES on collision).
		if (stance == "move" || stance == "walk")
		{
			if (phobj.is_flag_not_set(PhysicsObject::Flag::TURNATEDGES))
			{
				flip = !flip;
				phobj.hspeed = flip ? 1.0 : -1.0;
				phobj.set_flag(PhysicsObject::Flag::TURNATEDGES);
			}
		}

		if (animations.count(stance))
		{
			bool aniend = animations.at(stance).update();

			if (aniend && states.size() > 0)
			{
				size_t next_stance = random.next_int(states.size());
				std::string new_stance = states[next_stance];
				set_stance(new_stance);
			}
		}

		if (quest_mark_type != QuestMarkType::NONE)
			quest_mark_anim.update();

		return phobj.fhlayer;
	}

	void Npc::set_stance(const std::string& st)
	{
		if (stance != st)
		{
			stance = st;

			auto iter = animations.find(stance);

			if (iter == animations.end())
				return;

			iter->second.reset();

			// Move NPC horizontally when in a walk/move state
			if (stance == "move" || stance == "walk")
			{
				phobj.hspeed = flip ? 1.0 : -1.0;
				phobj.set_flag(PhysicsObject::Flag::TURNATEDGES);
			}
			else
			{
				phobj.hspeed = 0.0;
			}
		}
	}

	bool Npc::isscripted() const
	{
		return scripted;
	}

	bool Npc::inrange(Point<int16_t> cursorpos, Point<int16_t> viewpos) const
	{
		if (!active)
			return false;

		Point<int16_t> absp = get_position() + viewpos;

		Point<int16_t> dim =
			animations.count(stance) ?
			animations.at(stance).get_dimensions() :
			Point<int16_t>();

		return Rectangle<int16_t>(
			absp.x() - dim.x() / 2,
			absp.x() + dim.x() / 2,
			absp.y() - dim.y(),
			absp.y()
			).contains(cursorpos);
	}

	std::string Npc::get_name()
	{
		return name;
	}

	std::string Npc::get_func()
	{
		return func;
	}

	int32_t Npc::get_npcid() const
	{
		return npcid;
	}

	void Npc::update_quest_mark()
	{
		quest_mark_type = QuestMarkType::NONE;

		// If a previous TalkToNPC attempt timed out (server had no handler /
		// the quest data is missing from this server), never show an
		// indicator bulb above this NPC.
		if (NpcResponseTracker::get().is_unavailable(npcid))
			return;

		const Player& player = Stage::get().get_player();
		const QuestLog& quests = const_cast<Player&>(player).get_quests();
		int16_t player_level = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::LEVEL));
		int16_t player_job = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::JOB));

		const auto& started = quests.get_started();
		const auto& completed_map = quests.get_completed();

		nl::node quest_check = nl::nx::quest["Check.img"];
		nl::node quest_info = nl::nx::quest["QuestInfo.img"];

		// First check: does this NPC have an in-progress quest ready to complete?
		for (auto& iter : started)
		{
			int16_t qid = iter.first;
			nl::node check = quest_check[std::to_string(qid)];
			if (!check) continue;

			nl::node end_check = check["1"];
			if (!end_check) continue;

			int32_t end_npc = static_cast<int32_t>(safe_int(end_check["npc"]));
			if (end_npc != npcid) continue;

			// This NPC completes an in-progress quest — show complete mark
			quest_mark_type = QuestMarkType::COMPLETE;
			quest_mark_anim = mark_complete;
			return;
		}

		// Second check: does this NPC have an in-progress quest (not yet completable)?
		for (auto& iter : started)
		{
			int16_t qid = iter.first;
			nl::node check = quest_check[std::to_string(qid)];
			if (!check) continue;

			nl::node start_check = check["0"];
			if (!start_check) continue;

			int32_t start_npc = static_cast<int32_t>(safe_int(start_check["npc"]));
			if (start_npc != npcid) continue;

			quest_mark_type = QuestMarkType::IN_PROGRESS;
			quest_mark_anim = mark_in_progress;
			return;
		}

		// Third check: does this NPC have an available quest the player qualifies for?
		for (auto qnode : quest_info)
		{
			std::string qid_str = qnode.name();
			if (qid_str.empty() || qid_str[0] < '0' || qid_str[0] > '9')
				continue;

			int32_t qid_full;
			try { qid_full = std::stoi(qid_str); }
			catch (...) { continue; }
			if (qid_full > 32767 || qid_full < 0) continue;
			int16_t qid = static_cast<int16_t>(qid_full);

			// Skip already started or completed
			if (started.count(qid) || completed_map.count(qid))
				continue;

			nl::node check = quest_check[qid_str];
			if (!check) continue;

			nl::node start_check = check["0"];
			if (!start_check) continue;

			// Must also have an end-check — quests without one are typically
			// internal/scripted stubs that aren't player-pickable.
			if (!check["1"])
				continue;

			// Check if this NPC starts the quest
			int32_t start_npc = static_cast<int32_t>(safe_int(start_check["npc"]));
			if (start_npc <= 0 || start_npc != npcid)
				continue;

			// Skip auto-start / scripted / pre-complete quests (no bulb). These
			// fields can be either integers or script-name strings (e.g. v83
			// Cygnus tutorial), so use is_truthy instead of get_integer().
			if (is_truthy(start_check["normalAutoStart"]))
				continue;
			if (is_truthy(start_check["autoStart"]))
				continue;
			if (is_truthy(start_check["autoPreComplete"]))
				continue;
			if (is_truthy(start_check["startscript"]))
				continue;

			// QuestInfo gating — hidden/blocked/scripted quests should not show
			if (is_truthy(qnode["blocked"]))
				continue;
			if (is_truthy(qnode["autoStart"]))
				continue;
			if (is_truthy(qnode["autoPreComplete"]))
				continue;

			// Real player-facing quests have a name and an area; event/hidden
			// quests that share an NPC ID usually lack one or both.
			std::string qname = qnode["name"].get_string();
			if (qname.empty())
				continue;
			if (safe_int(qnode["area"]) <= 0)
				continue;

			// Level checks
			int32_t lvmin = static_cast<int32_t>(safe_int(start_check["lvmin"]));
			int32_t lvmax = static_cast<int32_t>(safe_int(start_check["lvmax"]));

			if (lvmin > 0 && player_level < lvmin)
				continue;

			if (lvmax > 0 && player_level > lvmax)
				continue;

			// Skip quests that are way below player level
			if (lvmin > 0 && (player_level - lvmin) > 10)
				continue;

			// Skip expired event quests
			std::string end_date = start_check["end"].get_string();
			if (!end_date.empty())
				continue;

			// Job check
			nl::node job_node = start_check["job"];
			if (job_node && job_node.size() > 0)
			{
				bool job_ok = false;
				for (auto j : job_node)
				{
					int32_t required_job = static_cast<int32_t>(safe_int(j));
					if (required_job == 0 || required_job == player_job)
					{
						job_ok = true;
						break;
					}
					if (required_job > 0 && (player_job / 100) == (required_job / 100) && player_job >= required_job)
					{
						job_ok = true;
						break;
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
					int32_t prereq_id = static_cast<int32_t>(safe_int(qp["id"]));
					int32_t prereq_state = static_cast<int32_t>(safe_int(qp["state"]));

					if (prereq_id > 0)
					{
						if (prereq_state == 2 && !completed_map.count(static_cast<int16_t>(prereq_id)))
						{
							prereqs_met = false;
							break;
						}
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

			// Player qualifies — show available mark
			quest_mark_type = QuestMarkType::AVAILABLE;
			quest_mark_anim = mark_available;
			return;
		}
	}
}