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
#include "OtherChar.h"

#include "../Constants.h"

#include <cmath>

namespace ms
{
	OtherChar::OtherChar(int32_t id, const CharLook& lk, uint8_t lvl, int16_t jb, const std::string& nm, int8_t st, Point<int16_t> pos) : Char(id, lk, nm)
	{
		level = lvl;
		job = jb;
		set_position(pos);

		lastmove.xpos = pos.x();
		lastmove.ypos = pos.y();
		lastmove.newstate = st;
		timer = 0;

		attackspeed = 6;
		attacking = false;

		apply_nametag_style(jb == 900 || jb == 910);
		refresh_ring_effect();
	}

	int8_t OtherChar::update(const Physics& physics)
	{
		if (!attacking)
			set_state(lastmove.newstate);

		// Damped follow toward the latest reported position. A gentle factor
		// spreads motion across the gaps between (sparse) move packets, so the
		// character glides instead of stepping once per packet.
		double dx = lastmove.xpos - phobj.crnt_x();
		double dy = lastmove.ypos - phobj.crnt_y();

		phobj.hspeed = dx * 0.18;
		phobj.vspeed = dy * 0.18;
		phobj.move();

		bool aniend = Char::update(physics, get_stancespeed());

		if (aniend && attacking)
			attacking = false;

		return get_layer();
	}

	void OtherChar::send_movement(const std::vector<Movement>& newmoves)
	{
		if (newmoves.empty())
			return;

		const Movement& target = newmoves.back();

		// Recover from big jumps (teleport, flash jump, or a position that
		// drifted out of sync) by snapping straight to the server position
		// instead of sliding the character across the map — otherwise a
		// foreign player lingers where they used to be and appears to
		// attack empty space.
		double dx = target.xpos - phobj.crnt_x();
		double dy = target.ypos - phobj.crnt_y();

		if ((dx * dx + dy * dy) > (200.0 * 200.0))
		{
			phobj.set_x(target.xpos);
			phobj.set_y(target.ypos);
			phobj.hspeed = 0.0;
			phobj.vspeed = 0.0;
		}

		lastmove = target;
	}

	void OtherChar::update_skill(int32_t skillid, uint8_t skilllevel)
	{
		skilllevels[skillid] = skilllevel;
	}

	void OtherChar::update_speed(uint8_t as)
	{
		attackspeed = as;
	}

	void OtherChar::update_look(const LookEntry& newlook)
	{
		look = newlook;

		uint8_t laststate = lastmove.newstate;
		set_state(laststate);

		// A newly-equipped effect ring should start (or stop) its aura.
		refresh_ring_effect();
	}

	int8_t OtherChar::get_integer_attackspeed() const
	{
		return attackspeed;
	}

	uint16_t OtherChar::get_level() const
	{
		return level;
	}

	int16_t OtherChar::get_job() const
	{
		return job;
	}

	int32_t OtherChar::get_skilllevel(int32_t skillid) const
	{
		auto iter = skilllevels.find(skillid);

		if (iter == skilllevels.end())
			return 0;

		return iter->second;
	}
}