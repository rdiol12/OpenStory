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
#include "PopupSettingsChrome.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#endif

namespace ms
{
	void PopupSettingsChrome::load(const nl::node& src)
	{
		title_make     = Texture(src["titleMake"]);
		title_settings = Texture(src["titleSettings"]);
	}

	void PopupSettingsChrome::draw_title(Point<int16_t> pos, bool make_mode) const
	{
		(make_mode ? title_make : title_settings).draw(pos);
	}
}
