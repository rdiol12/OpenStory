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
#include "Summon.h"

#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Summon::Summon(int32_t oi, int32_t owner, int32_t skillid, int8_t skilllv,
				   int8_t st, Point<int16_t> pos, MovementType mt, bool attacks)
		: MapObject(oi), owner_id(owner), skill_id(skillid), skill_level(skilllv),
		  move_type(mt), can_attack(attacks)
	{
		// Determine flip from stance byte (even = flip)
		flip = (st % 2) == 0;

		set_position(pos);

		// Load summon animation from Skill.img
		// Skill nodes are organized as: Skill.img/[job prefix]/[skill id]/summon/[frame]
		// or sometimes: Skill.img/[job prefix]/[skill id]/effect/[frame]
		std::string strid = string_format::extend_id(skillid / 10000, 3);
		nl::node skillsrc = nl::nx::skill[strid + ".img"][std::to_string(skillid)];

		nl::node summon_node = skillsrc["summon"];

		if (summon_node.size() > 0)
		{
			// Some summons have stance sub-nodes (stand, move, attack, die)
			nl::node stand = summon_node["stand"];
			nl::node fly = summon_node["fly"];

			if (stand.size() > 0)
				animation = Animation(stand);
			else if (fly.size() > 0)
				animation = Animation(fly);
			else
				animation = Animation(summon_node);
		}
		else
		{
			// Fallback: try the effect node
			nl::node effect_node = skillsrc["effect"];

			if (effect_node.size() > 0)
				animation = Animation(effect_node);
		}

		// Set physics type based on movement
		if (move_type == STATIONARY)
			phobj.type = PhysicsObject::Type::FIXATED;
	}

	void Summon::draw(double viewx, double viewy, float alpha) const
	{
		if (!active)
			return;

		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
		animation.draw(DrawArgument(absp, flip), alpha);
	}

	int8_t Summon::update(const Physics& physics)
	{
		if (!active)
			return phobj.fhlayer;

		animation.update();

		physics.move_object(phobj);

		return phobj.fhlayer;
	}

	void Summon::send_movement(Point<int16_t> start, std::vector<Movement>&& movements)
	{
		if (movements.empty())
			return;

		const Movement& lastmove = movements.back();

		flip = (lastmove.newstate % 2) == 0;

		phobj.set_x(lastmove.xpos);
		phobj.set_y(lastmove.ypos);
		phobj.fhid = lastmove.fh;
	}

	void Summon::apply_damage(int32_t damage)
	{
		// Visual feedback could be added here
	}

	int32_t Summon::get_owner_id() const
	{
		return owner_id;
	}

	int32_t Summon::get_skill_id() const
	{
		return skill_id;
	}
}
