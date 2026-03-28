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
#include "MobData.h"

#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	MobData::MobData(int32_t mobid)
	{
		valid = false;
		level = 0;
		hp = 0;
		mp = 0;
		watk = 0;
		matk = 0;
		wdef = 0;
		mdef = 0;
		accuracy = 0;
		avoid = 0;
		knockback = 0;
		speed = 0;
		fly_speed = 0;
		exp = 0;
		undead = false;
		body_attack = false;
		no_flip = false;
		not_attack = false;
		moveable = false;
		jumpable = false;
		flyable = false;
		boss = false;

		std::string strid = string_format::extend_id(mobid, 7);
		nl::node src = nl::nx::mob[strid + ".img"];

		if (!src)
			return;

		nl::node info = src["info"];

		level = info["level"];
		hp = info["maxHP"];
		mp = info["maxMP"];
		watk = info["PADamage"];
		matk = info["MADamage"];
		wdef = info["PDDamage"];
		mdef = info["MDDamage"];
		accuracy = info["acc"];
		avoid = info["eva"];
		knockback = info["pushed"];
		speed = info["speed"];
		fly_speed = info["flySpeed"];
		exp = info["exp"];
		undead = info["undead"].get_bool();
		body_attack = info["bodyAttack"].get_bool();
		no_flip = info["noFlip"].get_bool();
		not_attack = info["notAttack"].get_bool();
		boss = info["boss"].get_bool();

		jumpable = src["jump"].size() > 0;
		flyable = src["fly"].size() > 0;
		moveable = src["move"].size() > 0 || flyable;

		name = (std::string)nl::nx::string["Mob.img"][std::to_string(mobid)]["name"];

		valid = true;
	}

	bool MobData::is_valid() const
	{
		return valid;
	}

	MobData::operator bool() const
	{
		return valid;
	}

	const std::string& MobData::get_name() const
	{
		return name;
	}

	uint16_t MobData::get_level() const
	{
		return level;
	}

	int32_t MobData::get_hp() const
	{
		return hp;
	}

	int32_t MobData::get_mp() const
	{
		return mp;
	}

	uint16_t MobData::get_watk() const
	{
		return watk;
	}

	uint16_t MobData::get_matk() const
	{
		return matk;
	}

	uint16_t MobData::get_wdef() const
	{
		return wdef;
	}

	uint16_t MobData::get_mdef() const
	{
		return mdef;
	}

	uint16_t MobData::get_accuracy() const
	{
		return accuracy;
	}

	uint16_t MobData::get_avoid() const
	{
		return avoid;
	}

	uint16_t MobData::get_knockback() const
	{
		return knockback;
	}

	int32_t MobData::get_speed() const
	{
		return speed;
	}

	int32_t MobData::get_fly_speed() const
	{
		return fly_speed;
	}

	int32_t MobData::get_exp() const
	{
		return exp;
	}

	bool MobData::is_undead() const
	{
		return undead;
	}

	bool MobData::is_body_attack() const
	{
		return body_attack;
	}

	bool MobData::is_no_flip() const
	{
		return no_flip;
	}

	bool MobData::is_not_attack() const
	{
		return not_attack;
	}

	bool MobData::can_move() const
	{
		return moveable;
	}

	bool MobData::can_jump() const
	{
		return jumpable;
	}

	bool MobData::can_fly() const
	{
		return flyable;
	}

	bool MobData::is_boss() const
	{
		return boss;
	}
}
