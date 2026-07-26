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
#include "Guild.h"

namespace ms
{
	GuildTag::GuildTag() : label(Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::MEDIUMBLUE))
	{
	}

	void GuildTag::draw(Point<int16_t> absp) const
	{
		if (!label.get_text().empty())
			label.draw(absp + Point<int16_t>(0, 8));
	}

	void GuildTag::set_name(const std::string& name)
	{
		label.change_text(name);
	}

	void GuildTag::set_mark(int16_t bg, int8_t bgcolor, int16_t logo, int8_t logocolor)
	{
		mark_bg = bg;
		mark_bgcolor = bgcolor;
		mark_logo = logo;
		mark_logocolor = logocolor;
	}
}
