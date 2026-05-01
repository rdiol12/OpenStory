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
#include "SideMenuBackdrop.h"

#include "../../Graphics/DrawArgument.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#endif

namespace ms
{
	void SideMenuBackdrop::load(const nl::node& menu)
	{
		top    = menu["top"];
		mid    = menu["center"];
		bottom = menu["bottom"];
	}

	void SideMenuBackdrop::draw(Point<int16_t> pos, size_t visible_count) const
	{
		Point<int16_t> p = pos;

		top.draw(DrawArgument(p));
		p.shift_y(TOP_H);

		for (size_t row = 0; row < visible_count; row++)
			mid.draw(DrawArgument(p + Point<int16_t>(0,
				static_cast<int16_t>(row) * BUTTON_H)));

		p.shift_y(static_cast<int16_t>(visible_count) * BUTTON_H);
		bottom.draw(DrawArgument(p));
	}
}
