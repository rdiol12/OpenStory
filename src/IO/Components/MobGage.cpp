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
#include "MobGage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	MobGage::MobGage() : current_mob(0), hp_percent(0.0f), active(false)
	{
		nl::node src = nl::nx::ui["MobGage.img"];

		backgrnd = src["backgrnd"];
		bar_bg = src["bar"]["0"];
		bar_fill = src["bar"]["1"];
		frame = src["frame"];

		name_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE);
	}

	void MobGage::draw(Point<int16_t> position, float percent) const
	{
		if (!active)
			return;

		frame.draw(DrawArgument(position));
		backgrnd.draw(DrawArgument(position));

		if (percent > 0.0f)
		{
			int16_t bar_width = static_cast<int16_t>(bar_fill.width() * percent);

			bar_bg.draw(DrawArgument(position));
			bar_fill.draw(DrawArgument(position, Point<int16_t>(bar_width, 0)));
		}

		name_label.draw(DrawArgument(position + Point<int16_t>(0, -6)));
	}

	void MobGage::set_mob(int32_t mobid, const std::string& name, int8_t boss_type)
	{
		current_mob = mobid;
		active = true;

		name_label.change_text(name);
	}

	bool MobGage::is_active() const
	{
		return active;
	}

	void MobGage::clear()
	{
		active = false;
		current_mob = 0;
	}
}
