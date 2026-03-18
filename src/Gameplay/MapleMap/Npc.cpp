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

		nl::node guide = nl::nx::ui["UIWindow2.img"]["QuestGuide"];

		// Available quest — QuestMark (animated lightbulb, frames 0/1/2/3)
		nl::node qmark = guide["QuestMark"];
		if (qmark && qmark.size() > 0)
			mark_available = Animation(qmark);

		// In-progress quest — LowLVQuestMark/forNPC (distinct from available)
		nl::node inprog = guide["LowLVQuestMark"]["forNPC"];
		if (inprog && inprog.size() > 0)
			mark_in_progress = Animation(inprog);
		else if (qmark && qmark.size() > 0)
			mark_in_progress = Animation(qmark);

		// Completable quest — RepeatQuestMark/forNPC (book-like icon)
		nl::node complete = guide["RepeatQuestMark"]["forNPC"];
		if (complete && complete.size() > 0)
			mark_complete = Animation(complete);
		else if (qmark && qmark.size() > 0)
			mark_complete = Animation(qmark);

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

		std::string link = src["info"]["link"];

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
				lines[state].push_back(strsrc[speaknode.get_string()]);
		}

		name = strsrc["name"];
		func = strsrc["func"];

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
			Point<int16_t> mark_pos = absp + Point<int16_t>(0, -origin.y() - 10);
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

			int32_t end_npc = end_check["npc"].get_integer();
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

			int32_t start_npc = start_check["npc"].get_integer();
			if (start_npc != npcid) continue;

			quest_mark_type = QuestMarkType::IN_PROGRESS;
			quest_mark_anim = mark_in_progress;
			return;
		}

		// Third check: does this NPC have an available quest the player qualifies for?
		// Only show for quests within 5 levels of the player to avoid marking every NPC
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

			// Check if this NPC starts the quest
			int32_t start_npc = start_check["npc"].get_integer();
			if (start_npc <= 0 || start_npc != npcid)
				continue;

			// Level check — stricter: only show if quest is close to player level
			int32_t lvmin = start_check["lvmin"].get_integer();
			int32_t lvmax = start_check["lvmax"].get_integer();

			if (lvmin > 0 && player_level < lvmin)
				continue;

			if (lvmax > 0 && player_level > lvmax)
				continue;

			// Skip quests that are way below player level
			if (lvmin > 0 && (player_level - lvmin) > 10)
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
					int32_t prereq_id = qp["id"].get_integer();
					int32_t prereq_state = qp["state"].get_integer();

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