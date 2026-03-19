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
#include "Door.h"
#include "Layer.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Door::Door(int32_t oid, int32_t owner_id, Point<int16_t> position, bool launched)
		: MapObject(oid, position), owner_id(owner_id), launched(launched)
	{
		nl::node portal_src = nl::nx::map["MapHelper.img"]["portal"]["game"]["portal"];

		if (portal_src)
			animation = Animation(portal_src);

		active = true;
	}

	void Door::draw(double viewx, double viewy, float alpha) const
	{
		if (!active)
			return;

		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
		animation.draw(DrawArgument(absp), alpha);
	}

	int8_t Door::update(const Physics& physics)
	{
		animation.update();
		return get_layer();
	}

	int8_t Door::get_layer() const
	{
		return Layer::SEVEN;
	}
}
