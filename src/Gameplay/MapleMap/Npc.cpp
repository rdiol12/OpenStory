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
#include "../../Character/QuestLog.h"
#include "../../Data/NpcData.h"
#include "../../Data/QuestData.h"
#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

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
		const NpcData& data = NpcData::get(id);

		hidename = data.get_hide_name();
		mouseonly = data.get_talk_mouse_only();
		scripted = !data.get_script().empty() || data.is_shop();

		// Resolve linked NPC for animations (still needs direct NX)
		std::string strid = string_format::extend_id(id, 7);
		nl::node src = nl::nx::npc[strid + ".img"];

		int32_t link_id = data.get_link();

		if (link_id > 0)
		{
			std::string linkstrid = string_format::extend_id(link_id, 7);
			src = nl::nx::npc[linkstrid + ".img"];
		}

		for (auto npcnode : src)
		{
			std::string state = npcnode.name();

			if (state != "info")
			{
				animations[state] = npcnode;
				states.push_back(state);
			}
		}

		// Use cached data for speak lines
		if (!states.empty())
			lines[states[0]] = data.get_speak_lines();

		name = data.get_name();
		func = data.get_func();

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
	}

	int8_t Npc::update(const Physics& physics)
	{
		if (!active)
			return phobj.fhlayer;

		physics.move_object(phobj);

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

		const Player& player = Stage::get().get_player();
		const QuestLog& quests = const_cast<Player&>(player).get_quests();
		int16_t player_level = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::LEVEL));
		int16_t player_job = static_cast<int16_t>(player.get_stats().get_stat(MapleStat::Id::JOB));

		const auto& started = quests.get_started();
		const auto& completed_map = quests.get_completed();

		// First check: does this NPC complete an in-progress quest?
		for (auto& iter : started)
		{
			int16_t qid = iter.first;
			const QuestData& qdata = QuestData::get(qid);

			if (!qdata.is_valid())
				continue;

			if (qdata.get_npc(false) != npcid)
				continue;

			quest_mark_type = QuestMarkType::COMPLETE;
			quest_mark_anim = mark_complete;
			return;
		}

		// Second check: does this NPC start an in-progress quest?
		for (auto& iter : started)
		{
			int16_t qid = iter.first;
			const QuestData& qdata = QuestData::get(qid);

			if (!qdata.is_valid())
				continue;

			if (qdata.get_npc(true) != npcid)
				continue;

			quest_mark_type = QuestMarkType::IN_PROGRESS;
			quest_mark_anim = mark_in_progress;
			return;
		}

		// Third check: available quests (still iterate NX since Cache is per-ID)
		nl::node quest_info = nl::nx::quest["QuestInfo.img"];

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

			if (started.count(qid) || completed_map.count(qid))
				continue;

			const QuestData& qdata = QuestData::get(qid);

			if (!qdata.is_valid())
				continue;

			if (qdata.get_npc(true) != npcid)
				continue;

			if (qdata.is_auto_start())
				continue;

			int16_t lvmin = qdata.get_min_level(true);
			int16_t lvmax = qdata.get_max_level(true);

			if (lvmin > 0 && player_level < lvmin)
				continue;

			if (lvmax > 0 && player_level > lvmax)
				continue;

			if (lvmin > 0 && (player_level - lvmin) > 10)
				continue;

			// Job check via QuestData
			const auto& jobs = qdata.get_job_reqs(true);

			if (!jobs.empty())
			{
				bool job_ok = false;

				for (int32_t required_job : jobs)
				{
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

			// Prerequisite quest check via QuestData
			const auto& prereqs = qdata.get_quest_reqs(true);
			bool prereqs_met = true;

			for (const auto& pq : prereqs)
			{
				if (pq.questid > 0)
				{
					if (pq.state == 2 && !completed_map.count(static_cast<int16_t>(pq.questid)))
					{
						prereqs_met = false;
						break;
					}

					if (pq.state == 1 && !started.count(static_cast<int16_t>(pq.questid)))
					{
						prereqs_met = false;
						break;
					}
				}
			}

			if (!prereqs_met)
				continue;

			quest_mark_type = QuestMarkType::AVAILABLE;
			quest_mark_anim = mark_available;
			return;
		}
	}
}