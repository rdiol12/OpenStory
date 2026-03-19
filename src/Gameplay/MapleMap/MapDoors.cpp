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
#include "MapDoors.h"

namespace ms
{
	void MapDoors::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		doors.draw(layer, viewx, viewy, alpha);
	}

	void MapDoors::update(const Physics& physics)
	{
		for (; !spawns.empty(); spawns.pop())
		{
			const DoorSpawn& spawn = spawns.front();

			int32_t oid = spawn.get_oid();

			if (auto door = doors.get(oid))
				door->makeactive();
			else
				doors.add(spawn.instantiate(physics));
		}

		doors.update(physics);
	}

	void MapDoors::spawn(DoorSpawn&& spawn)
	{
		spawns.emplace(std::move(spawn));
	}

	void MapDoors::remove(int32_t oid)
	{
		if (auto door = doors.get(oid))
			door->deactivate();
	}

	void MapDoors::clear()
	{
		doors.clear();
	}
}
