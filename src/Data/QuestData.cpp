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
#include "QuestData.h"

#include "StringData.h"
#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	QuestData::QuestData(int32_t questid)
	{
		valid = false;
		area = 0;
		order = 0;
		time_limit = 0;
		auto_start = false;
		auto_pre_complete = false;
		medal = false;

		for (int i = 0; i < 2; i++)
		{
			npc[i] = 0;
			min_level[i] = 0;
			max_level[i] = 0;
			normal_auto_start[i] = false;
			exp_reward[i] = 0;
			meso_reward[i] = 0;
			fame_reward[i] = 0;
			buff_item_reward[i] = 0;
		}

		std::string strid = std::to_string(questid);

		nl::node info_src = nl::nx::quest["QuestInfo.img"][strid];

		if (!info_src)
			return;

		name = (std::string)info_src["name"];
		parent = (std::string)info_src["parent"];
		area = info_src["area"];
		order = info_src["order"];
		time_limit = info_src["timeLimit"];
		auto_start = info_src["autoStart"].get_bool();
		auto_pre_complete = info_src["autoPreComplete"].get_bool();

		// Description is in nodes "0" (start) and "1" (complete)
		nl::node desc0 = info_src["0"];
		nl::node desc1 = info_src["1"];

		if (desc0)
			desc[0] = (std::string)desc0;

		if (desc1)
			desc[1] = (std::string)desc1;

		// Check if this is a medal quest
		nl::node medal_node = nl::nx::quest["Exclusive.img"]["medal"][strid];
		medal = (bool)medal_node;

		load_requirements(questid, true);
		load_requirements(questid, false);
		load_rewards(questid, true);
		load_rewards(questid, false);
		load_say(questid);

		valid = true;
	}

	void QuestData::load_requirements(int32_t questid, bool start)
	{
		int idx = start ? 0 : 1;
		std::string strid = std::to_string(questid);
		std::string phase = start ? "0" : "1";

		nl::node check_src = nl::nx::quest["Check.img"][strid][phase];

		if (!check_src)
			return;

		npc[idx] = check_src["npc"];
		min_level[idx] = (int16_t)check_src["lvmin"].get_integer(0);
		max_level[idx] = (int16_t)check_src["lvmax"].get_integer(0);
		normal_auto_start[idx] = check_src["normalAutoStart"].get_bool();
		end_date[idx] = (std::string)check_src["end"];

		for (auto job_node : check_src["job"])
			job_reqs[idx].push_back(job_node);

		for (auto quest_node : check_src["quest"])
		{
			int32_t qid = quest_node["id"];
			int32_t state = quest_node["state"];
			quest_reqs[idx].push_back({ qid, state });
		}

		for (auto item_node : check_src["item"])
		{
			int32_t itemid = item_node["id"];
			int32_t count = item_node["count"].get_integer(1);
			item_reqs[idx].push_back({ itemid, count });
		}

		for (auto mob_node : check_src["mob"])
		{
			int32_t mobid = mob_node["id"];
			int32_t count = mob_node["count"].get_integer(1);
			std::string mob_name = StringData::get_mob_name(mobid);
			mob_reqs[idx].push_back({ mobid, count, mob_name });
		}
	}

	void QuestData::load_rewards(int32_t questid, bool start)
	{
		int idx = start ? 0 : 1;
		std::string strid = std::to_string(questid);
		std::string phase = start ? "0" : "1";

		nl::node act_src = nl::nx::quest["Act.img"][strid][phase];

		if (!act_src)
			return;

		exp_reward[idx] = act_src["exp"];
		meso_reward[idx] = act_src["money"];
		fame_reward[idx] = act_src["pop"];
		buff_item_reward[idx] = act_src["buffItemID"];

		for (auto item_node : act_src["item"])
		{
			int32_t itemid = item_node["id"];
			int32_t count = item_node["count"].get_integer(1);
			int32_t prop = item_node["prop"].get_integer(-1);
			int8_t gender = (int8_t)item_node["gender"].get_integer(2);
			item_rewards[idx].push_back({ itemid, count, prop, gender });
		}

		for (auto skill_node : act_src["skill"])
		{
			int32_t skillid = skill_node["id"];
			int32_t level = skill_node["skillLevel"];
			int32_t master_level = skill_node["masterLevel"];
			std::vector<int32_t> jobs;

			for (auto job_node : skill_node["job"])
				jobs.push_back(job_node);

			skill_rewards[idx].push_back({ skillid, level, master_level, jobs });
		}
	}

	bool QuestData::is_valid() const
	{
		return valid;
	}

	QuestData::operator bool() const
	{
		return valid;
	}

	const std::string& QuestData::get_name() const
	{
		return name;
	}

	const std::string& QuestData::get_desc(bool start) const
	{
		return desc[start ? 0 : 1];
	}

	const std::string& QuestData::get_parent() const
	{
		return parent;
	}

	int32_t QuestData::get_npc(bool start) const
	{
		return npc[start ? 0 : 1];
	}

	int32_t QuestData::get_area() const
	{
		return area;
	}

	int16_t QuestData::get_min_level(bool start) const
	{
		return min_level[start ? 0 : 1];
	}

	int16_t QuestData::get_max_level(bool start) const
	{
		return max_level[start ? 0 : 1];
	}

	int32_t QuestData::get_time_limit() const
	{
		return time_limit;
	}

	bool QuestData::is_auto_start() const
	{
		return auto_start;
	}

	bool QuestData::is_auto_pre_complete() const
	{
		return auto_pre_complete;
	}

	const std::vector<QuestData::MobRequirement>& QuestData::get_mob_reqs(bool start) const
	{
		return mob_reqs[start ? 0 : 1];
	}

	const std::vector<QuestData::ItemRequirement>& QuestData::get_item_reqs(bool start) const
	{
		return item_reqs[start ? 0 : 1];
	}

	const std::vector<QuestData::QuestRequirement>& QuestData::get_quest_reqs(bool start) const
	{
		return quest_reqs[start ? 0 : 1];
	}

	const std::vector<int32_t>& QuestData::get_job_reqs(bool start) const
	{
		return job_reqs[start ? 0 : 1];
	}

	int32_t QuestData::get_exp_reward(bool start) const
	{
		return exp_reward[start ? 0 : 1];
	}

	int32_t QuestData::get_meso_reward(bool start) const
	{
		return meso_reward[start ? 0 : 1];
	}

	int32_t QuestData::get_fame_reward(bool start) const
	{
		return fame_reward[start ? 0 : 1];
	}

	int32_t QuestData::get_buff_item_reward(bool start) const
	{
		return buff_item_reward[start ? 0 : 1];
	}

	const std::vector<QuestData::ItemReward>& QuestData::get_item_rewards(bool start) const
	{
		return item_rewards[start ? 0 : 1];
	}

	const std::vector<QuestData::SkillReward>& QuestData::get_skill_rewards(bool start) const
	{
		return skill_rewards[start ? 0 : 1];
	}

	void QuestData::load_say(int32_t questid)
	{
		std::string strid = std::to_string(questid);
		nl::node say_src = nl::nx::quest["Say.img"][strid];

		if (!say_src)
			return;

		for (int phase = 0; phase < 2; phase++)
		{
			nl::node phase_node = say_src[std::to_string(phase)];

			if (!phase_node)
				continue;

			for (auto line_node : phase_node)
			{
				SayLine line;
				line.text = (std::string)line_node["text"];
				line.npc = line_node["npc"];
				say_lines[phase].push_back(line);
			}
		}
	}

	int32_t QuestData::get_order() const
	{
		return order;
	}

	bool QuestData::is_normal_auto_start(bool start) const
	{
		return normal_auto_start[start ? 0 : 1];
	}

	const std::string& QuestData::get_end_date(bool start) const
	{
		return end_date[start ? 0 : 1];
	}

	const std::vector<QuestData::SayLine>& QuestData::get_say_lines(bool start) const
	{
		return say_lines[start ? 0 : 1];
	}

	bool QuestData::is_medal_quest() const
	{
		return medal;
	}
}
