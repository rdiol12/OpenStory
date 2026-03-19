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
#include "Mist.h"
#include "Layer.h"

#include "../../Graphics/Geometry.h"

namespace ms
{
	Mist::Mist(int32_t oid, int32_t owner_id, Point<int16_t> pos, Point<int16_t> pos2,
		int32_t skill_id, int8_t skill_level, int8_t mist_type)
		: MapObject(oid, pos), owner_id(owner_id), topleft(pos), bottomright(pos2),
		  skill_id(skill_id), skill_level(skill_level), mist_type(mist_type)
	{
		active = true;
	}

	void Mist::draw(double viewx, double viewy, float alpha) const
	{
		if (!active)
			return;

		int16_t w = static_cast<int16_t>(bottomright.x() - topleft.x());
		int16_t h = static_cast<int16_t>(bottomright.y() - topleft.y());

		if (w <= 0 || h <= 0)
			return;

		// Pick color based on mist type
		Color::Name color;

		if (mist_type == 1) // Poison mist
			color = Color::Name::LEMONGRASS;
		else if (mist_type == 2) // Smoke screen
			color = Color::Name::WHITE;
		else // Mob mist
			color = Color::Name::VIOLET;

		ColorBox box(w, h, color, 0.3f);

		Point<int16_t> absp(
			static_cast<int16_t>(topleft.x() + viewx),
			static_cast<int16_t>(topleft.y() + viewy)
		);

		box.draw(DrawArgument(absp));
	}

	int8_t Mist::update(const Physics& physics)
	{
		return get_layer();
	}

	int8_t Mist::get_layer() const
	{
		return Layer::SEVEN;
	}
}
