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

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

namespace ms
{
	// Boss HP gauge overlay loaded from MobGage.img (via UI.nx)
	class MobGage
	{
	public:
		MobGage();

		void draw(Point<int16_t> position, float hp_percent) const;
		void set_mob(int32_t mobid, const std::string& name, int8_t boss_type);
		bool is_active() const;
		void clear();

	private:
		Texture backgrnd;
		Texture bar_bg;
		Texture bar_fill;
		Texture frame;
		Text name_label;
		int32_t current_mob;
		float hp_percent;
		bool active;
	};
}
