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

#include "MapObjects.h"
#include "Mist.h"

#include "../Spawn.h"

#include <queue>

namespace ms
{
	// Collection of mist/smoke screen effects on a map
	class MapMists
	{
	public:
		// Draw all mists on a layer
		void draw(Layer::Id layer, double viewx, double viewy, float alpha) const;
		// Update all mists
		void update(const Physics& physics);

		// Spawn a new mist
		void spawn(MistSpawn&& spawn);
		// Remove a mist by oid
		void remove(int32_t oid);
		// Remove all mists
		void clear();

	private:
		MapObjects mists;

		std::queue<MistSpawn> spawns;
	};
}
