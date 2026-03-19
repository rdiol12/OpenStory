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
#include "../Movement.h"

#include <queue>

namespace ms
{
	struct DragonSpawn
	{
		int32_t owner_id;
		Point<int16_t> position;
		uint8_t stance;
		int16_t job;
	};

	class MapDragons
	{
	public:
		void draw(Layer::Id layer, double viewx, double viewy, float alpha) const;
		void update(const Physics& physics);

		void spawn(DragonSpawn&& spawn);
		void remove(int32_t owner_id);
		void clear();

		void send_movement(int32_t owner_id, const std::vector<Movement>& movements);

	private:
		MapObjects dragons;
		std::queue<DragonSpawn> spawns;
	};
}
