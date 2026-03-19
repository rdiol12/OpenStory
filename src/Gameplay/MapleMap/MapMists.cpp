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
#include "MapMists.h"

namespace ms
{
	void MapMists::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		mists.draw(layer, viewx, viewy, alpha);
	}

	void MapMists::update(const Physics& physics)
	{
		for (; !spawns.empty(); spawns.pop())
		{
			const MistSpawn& spawn = spawns.front();

			int32_t oid = spawn.get_oid();

			if (auto mist = mists.get(oid))
				mist->makeactive();
			else
				mists.add(spawn.instantiate(physics));
		}

		mists.update(physics);
	}

	void MapMists::spawn(MistSpawn&& spawn)
	{
		spawns.emplace(std::move(spawn));
	}

	void MapMists::remove(int32_t oid)
	{
		if (auto mist = mists.get(oid))
			mist->deactivate();
	}

	void MapMists::clear()
	{
		mists.clear();
	}
}
