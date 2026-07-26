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

#include "../../Graphics/Animation.h"

#include <vector>

namespace ms
{
	class MountLook
	{
	public:
		enum class Gait
		{
			STAND,
			WALK,
			JUMP,
			ROPE,
			LADDER,
			PRONE,
			FLY,
			LENGTH
		};

		void set_mount(int32_t itemid);

		int32_t get_itemid() const
		{
			return itemid;
		}

		bool is_active() const
		{
			return itemid != 0;
		}

		void update(Gait gait);
		void draw(Point<int16_t> absp, Gait gait, bool flip, float alpha) const;

		// Seat point of the mount's CURRENT animation frame — the rider is
		// re-anchored every frame so they bob with the gallop like vanilla.
		Point<int16_t> seat_offset(Gait gait) const;

	private:
		static constexpr size_t NUM_GAITS = static_cast<size_t>(Gait::LENGTH);

		const Animation& current_ani(Gait gait) const;
		Point<int16_t> current_navel(Gait gait) const;

		int32_t itemid = 0;
		Animation anis[NUM_GAITS];
		std::vector<Point<int16_t>> navels[NUM_GAITS];
	};
}
