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

namespace ms
{
	class Mist : public MapObject
	{
	public:
		Mist(int32_t oid, int32_t owner_id, Point<int16_t> pos, Point<int16_t> pos2,
			int32_t skill_id, int8_t skill_level, int8_t mist_type);

		void draw(double viewx, double viewy, float alpha) const override;
		int8_t update(const Physics& physics) override;
		int8_t get_layer() const override;

	private:
		int32_t owner_id;
		Point<int16_t> topleft;
		Point<int16_t> bottomright;
		int32_t skill_id;
		int8_t skill_level;
		int8_t mist_type;
	};
}
