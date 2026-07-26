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

#include "../Graphics/Text.h"

namespace ms
{
	// The guild name shown below a character's name plate, plus the guild
	// mark parameters (emblem rendering from GuildMark.img is future work).
	class GuildTag
	{
	public:
		GuildTag();

		void draw(Point<int16_t> absp) const;

		void set_name(const std::string& name);
		void set_mark(int16_t bg, int8_t bgcolor, int16_t logo, int8_t logocolor);

	private:
		Text label;
		int16_t mark_bg = 0;
		int8_t mark_bgcolor = 0;
		int16_t mark_logo = 0;
		int8_t mark_logocolor = 0;
	};
}
