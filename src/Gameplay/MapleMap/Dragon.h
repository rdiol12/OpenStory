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

#include "MapObject.h"

#include "../../Graphics/Animation.h"
#include "../Movement.h"

namespace ms
{
	// Represents an Evan dragon on the map
	class Dragon : public MapObject
	{
	public:
		Dragon(int32_t owner_id, Point<int16_t> position, uint8_t stance, int16_t job);

		void draw(double viewx, double viewy, float alpha) const override;
		int8_t update(const Physics& physics) override;

		void send_movement(const std::vector<Movement>& movements);

		int32_t get_owner_id() const;

	private:
		int32_t owner_id;
		int16_t job;
		Animation animation;
		bool flip;
	};
}
