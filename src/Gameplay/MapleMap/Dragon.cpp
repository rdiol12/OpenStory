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
#include "Dragon.h"

#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Dragon::Dragon(int32_t owner_id, Point<int16_t> position, uint8_t stance, int16_t job)
		: MapObject(owner_id), owner_id(owner_id), job(job), flip(false)
	{
		// Evan dragon growth stage depends on job advancement:
		// 2200 (Evan beginner) -> 9300089
		// 2210 (1st job) -> 9300090
		// 2211/2212 (2nd job) -> 9300091
		// 2213/2214 (3rd job) -> 9300092
		// 2215/2216/2217/2218 (4th job) -> 9300093
		int32_t dragon_mob_id;

		if (job >= 2215)
			dragon_mob_id = 9300093;
		else if (job >= 2213)
			dragon_mob_id = 9300092;
		else if (job >= 2211)
			dragon_mob_id = 9300091;
		else if (job >= 2210)
			dragon_mob_id = 9300090;
		else
			dragon_mob_id = 9300089;

		std::string strid = string_format::extend_id(dragon_mob_id, 7);
		nl::node src = nl::nx::mob[strid + ".img"];

		if (src["stand"])
			animation = Animation(src["stand"]["0"]);
		else if (src["fly"])
			animation = Animation(src["fly"]["0"]);
		else if (src["move"])
			animation = Animation(src["move"]["0"]);

		set_position(position);
		phobj.fhid = 0;
	}

	void Dragon::draw(double viewx, double viewy, float alpha) const
	{
		if (!active)
			return;

		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
		animation.draw(DrawArgument(absp, flip), alpha);
	}

	int8_t Dragon::update(const Physics& physics)
	{
		animation.update();
		physics.move_object(phobj);

		return phobj.fhlayer;
	}

	void Dragon::send_movement(const std::vector<Movement>& movements)
	{
		if (movements.empty())
			return;

		const Movement& last = movements.back();

		phobj.set_x(last.xpos);
		phobj.set_y(last.ypos);
		phobj.fhid = last.fh;

		flip = last.newstate % 2 == 1;
	}

	int32_t Dragon::get_owner_id() const
	{
		return owner_id;
	}
}
