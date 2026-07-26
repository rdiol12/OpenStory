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
#include "MountLook.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void MountLook::set_mount(int32_t mount_itemid)
	{
		if (mount_itemid == itemid)
			return;

		itemid = mount_itemid;

		if (mount_itemid == 0)
		{
			for (size_t i = 0; i < NUM_GAITS; i++)
				anis[i] = Animation();

			return;
		}

		std::string strid = std::to_string(mount_itemid);
		while (strid.size() < 8)
			strid.insert(0, 1, '0');

		nl::node base = nl::nx::character["TamingMob"][strid + ".img"];

		auto collect_navels = [](nl::node anim)
		{
			std::vector<Point<int16_t>> out;

			for (int i = 0;; i++)
			{
				nl::node fr = anim[std::to_string(i)];

				if (!fr)
					break;

				nl::node nav = fr["0"]["map"]["navel"];

				if (!nav)
					nav = fr["map"]["navel"];

				out.push_back(nav ? Point<int16_t>(nav) : Point<int16_t>(0, -30));
			}

			return out;
		};

		auto load = [&](Gait gait, nl::node src)
		{
			size_t i = static_cast<size_t>(gait);
			anis[i] = Animation(src);
			navels[i] = collect_navels(src);
		};

		load(Gait::STAND, base["stand1"] ? base["stand1"] : base["stand"]);
		load(Gait::WALK, base["walk1"] ? base["walk1"] : base["walk"]);
		load(Gait::JUMP, base["jump"]);
		load(Gait::ROPE, base["rope"]);
		load(Gait::LADDER, base["ladder"]);
		load(Gait::PRONE, base["prone"]);
		load(Gait::FLY, base["fly"]);
	}

	void MountLook::update(Gait gait)
	{
		const_cast<Animation&>(current_ani(gait)).update();
	}

	void MountLook::draw(Point<int16_t> absp, Gait gait, bool flip, float alpha) const
	{
		current_ani(gait).draw(DrawArgument(absp, flip), alpha);
	}

	Point<int16_t> MountLook::seat_offset(Gait gait) const
	{
		return current_navel(gait) + Point<int16_t>(0, 14);
	}

	const Animation& MountLook::current_ani(Gait gait) const
	{
		size_t i = static_cast<size_t>(gait);

		if (gait != Gait::STAND && anis[i].get_delay(0) > 0)
			return anis[i];

		return anis[static_cast<size_t>(Gait::STAND)];
	}

	Point<int16_t> MountLook::current_navel(Gait gait) const
	{
		size_t i = static_cast<size_t>(gait);
		const std::vector<Point<int16_t>>* v = &navels[static_cast<size_t>(Gait::STAND)];

		if (gait != Gait::STAND && !navels[i].empty())
			v = &navels[i];

		if (v->empty())
			return Point<int16_t>(0, -30);

		int16_t f = current_ani(gait).get_frame_index();

		if (f < 0 || f >= static_cast<int16_t>(v->size()))
			f = 0;

		return (*v)[f];
	}
}
