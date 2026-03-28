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
#pragma once

#include "../Template/Cache.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace ms
{
	class QuestData : public Cache<QuestData>
	{
	public:
		struct MobRequirement
		{
			int32_t mobid;
			int32_t count;
			std::string name;
		};

		struct ItemRequirement
		{
			int32_t itemid;
			int32_t count;
		};

		struct QuestRequirement
		{
			int32_t questid;
			int32_t state;
		};

		struct ItemReward
		{
			int32_t itemid;
			int32_t count;
			int32_t prop;
			int8_t gender;
		};

		struct SkillReward
		{
			int32_t skillid;
			int32_t level;
			int32_t master_level;
			std::vector<int32_t> jobs;
		};

		struct SayLine
		{
			std::string text;
			int32_t npc;
		};

		bool is_valid() const;
		explicit operator bool() const;

		const std::string& get_name() const;
		const std::string& get_desc(bool start) const;
		const std::string& get_parent() const;
		int32_t get_npc(bool start) const;
		int32_t get_area() const;
		int32_t get_order() const;
		int16_t get_min_level(bool start) const;
		int16_t get_max_level(bool start) const;
		int32_t get_time_limit() const;
		bool is_auto_start() const;
		bool is_auto_pre_complete() const;
		bool is_normal_auto_start(bool start) const;
		const std::string& get_end_date(bool start) const;

		const std::vector<MobRequirement>& get_mob_reqs(bool start) const;
		const std::vector<ItemRequirement>& get_item_reqs(bool start) const;
		const std::vector<QuestRequirement>& get_quest_reqs(bool start) const;
		const std::vector<int32_t>& get_job_reqs(bool start) const;

		int32_t get_exp_reward(bool start) const;
		int32_t get_meso_reward(bool start) const;
		int32_t get_fame_reward(bool start) const;
		int32_t get_buff_item_reward(bool start) const;
		const std::vector<ItemReward>& get_item_rewards(bool start) const;
		const std::vector<SkillReward>& get_skill_rewards(bool start) const;

		const std::vector<SayLine>& get_say_lines(bool start) const;
		bool is_medal_quest() const;

	private:
		friend Cache<QuestData>;
		QuestData(int32_t questid);

		void load_requirements(int32_t questid, bool start);
		void load_rewards(int32_t questid, bool start);
		void load_say(int32_t questid);

		bool valid;
		std::string name;
		std::string desc[2]; // 0 = start, 1 = complete
		std::string parent;
		int32_t area;
		int32_t order;
		int32_t time_limit;
		bool auto_start;
		bool auto_pre_complete;
		bool medal;

		// Index 0 = start, 1 = complete
		int32_t npc[2];
		int16_t min_level[2];
		int16_t max_level[2];
		bool normal_auto_start[2];
		std::string end_date[2];
		std::vector<MobRequirement> mob_reqs[2];
		std::vector<ItemRequirement> item_reqs[2];
		std::vector<QuestRequirement> quest_reqs[2];
		std::vector<int32_t> job_reqs[2];

		int32_t exp_reward[2];
		int32_t meso_reward[2];
		int32_t fame_reward[2];
		int32_t buff_item_reward[2];
		std::vector<ItemReward> item_rewards[2];
		std::vector<SkillReward> skill_rewards[2];

		std::vector<SayLine> say_lines[2];
	};
}
