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
#include "MapDragons.h"
#include "Dragon.h"

namespace ms
{
	void MapDragons::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		dragons.draw(layer, viewx, viewy, alpha);
	}

	void MapDragons::update(const Physics& physics)
	{
		while (!spawns.empty())
		{
			const DragonSpawn& sp = spawns.front();

			auto dragon = std::make_unique<Dragon>(
				sp.owner_id, sp.position, sp.stance, sp.job
			);

			dragons.add(std::move(dragon));
			spawns.pop();
		}

		dragons.update(physics);
	}

	void MapDragons::spawn(DragonSpawn&& sp)
	{
		spawns.push(std::move(sp));
	}

	void MapDragons::remove(int32_t owner_id)
	{
		if (auto dragon = dragons.get(owner_id))
			dragon->deactivate();
	}

	void MapDragons::clear()
	{
		dragons.clear();
	}

	void MapDragons::send_movement(int32_t owner_id, const std::vector<Movement>& movements)
	{
		if (auto obj = dragons.get(owner_id))
		{
			Dragon* dragon = static_cast<Dragon*>(obj.get());
			dragon->send_movement(movements);
		}
	}
}
