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
	class Summon : public MapObject
	{
	public:
		enum MovementType : int8_t
		{
			STATIONARY = 0,
			FOLLOW = 1,
			CIRCLE_FOLLOW = 3
		};

		Summon(int32_t oid, int32_t owner_id, int32_t skill_id, int8_t skill_level,
			   int8_t stance, Point<int16_t> position, MovementType move_type, bool attacks);

		void draw(double viewx, double viewy, float alpha) const override;
		int8_t update(const Physics& physics) override;

		void send_movement(Point<int16_t> start, std::vector<Movement>&& movements);
		void apply_damage(int32_t damage);

		int32_t get_owner_id() const;
		int32_t get_skill_id() const;

	private:
		Animation animation;
		int32_t owner_id;
		int32_t skill_id;
		int8_t skill_level;
		MovementType move_type;
		bool can_attack;
		bool flip;
	};
}
