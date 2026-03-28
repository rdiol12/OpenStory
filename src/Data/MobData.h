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
#include <cstdint>

namespace ms
{
	class MobData : public Cache<MobData>
	{
	public:
		bool is_valid() const;
		explicit operator bool() const;

		const std::string& get_name() const;
		uint16_t get_level() const;
		int32_t get_hp() const;
		int32_t get_mp() const;
		uint16_t get_watk() const;
		uint16_t get_matk() const;
		uint16_t get_wdef() const;
		uint16_t get_mdef() const;
		uint16_t get_accuracy() const;
		uint16_t get_avoid() const;
		uint16_t get_knockback() const;
		int32_t get_speed() const;
		int32_t get_fly_speed() const;
		int32_t get_exp() const;
		bool is_undead() const;
		bool is_body_attack() const;
		bool is_no_flip() const;
		bool is_not_attack() const;
		bool can_move() const;
		bool can_jump() const;
		bool can_fly() const;
		bool is_boss() const;

	private:
		friend Cache<MobData>;
		MobData(int32_t mobid);

		bool valid;
		std::string name;
		uint16_t level;
		int32_t hp;
		int32_t mp;
		uint16_t watk;
		uint16_t matk;
		uint16_t wdef;
		uint16_t mdef;
		uint16_t accuracy;
		uint16_t avoid;
		uint16_t knockback;
		int32_t speed;
		int32_t fly_speed;
		int32_t exp;
		bool undead;
		bool body_attack;
		bool no_flip;
		bool not_attack;
		bool moveable;
		bool jumpable;
		bool flyable;
		bool boss;
	};
}
