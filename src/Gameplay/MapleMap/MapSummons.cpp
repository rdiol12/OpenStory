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
#include "MapSummons.h"

namespace ms
{
	void MapSummons::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		summons.draw(layer, viewx, viewy, alpha);
	}

	void MapSummons::update(const Physics& physics)
	{
		for (; !spawns.empty(); spawns.pop())
		{
			const SummonSpawn& sp = spawns.front();

			auto summon = std::make_unique<Summon>(
				sp.oid, sp.owner_id, sp.skill_id, sp.skill_level,
				sp.stance, sp.position, sp.move_type, sp.attacks
			);

			summons.add(std::move(summon));
		}

		summons.update(physics);
	}

	void MapSummons::spawn(SummonSpawn&& sp)
	{
		spawns.emplace(std::move(sp));
	}

	void MapSummons::remove(int32_t oid, bool animated)
	{
		if (auto summon = summons.get(oid))
			summon->deactivate();
	}

	void MapSummons::clear()
	{
		summons.clear();
	}

	void MapSummons::send_movement(int32_t oid, Point<int16_t> start, std::vector<Movement>&& movements)
	{
		if (Optional<Summon> summon = summons.get(oid))
			summon->send_movement(start, std::move(movements));
	}

	void MapSummons::apply_damage(int32_t oid, int32_t damage)
	{
		if (Optional<Summon> summon = summons.get(oid))
			summon->apply_damage(damage);
	}
}
