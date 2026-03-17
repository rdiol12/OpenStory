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

#include "Template/Singleton.h"

#include <cstdint>

namespace ms
{
	namespace Constants
	{
		// Timestep, e.g. the granularity in which the game advances.
		constexpr uint16_t TIMESTEP = 8;

		class Constants : public Singleton<Constants>
		{
		public:
			Constants()
			{
				PHYSWIDTH = 1920;
				PHYSHEIGHT = 1080;
				UI_SCALE = 1.5f;
			};

			~Constants() {};

			// Logical dimensions (what game/UI code uses for positioning)
			int16_t get_viewwidth()
			{
				return static_cast<int16_t>(PHYSWIDTH / UI_SCALE);
			}

			int16_t get_viewheight()
			{
				return static_cast<int16_t>(PHYSHEIGHT / UI_SCALE);
			}

			// Physical dimensions (actual window/viewport size)
			int16_t get_physicalwidth()
			{
				return PHYSWIDTH;
			}

			int16_t get_physicalheight()
			{
				return PHYSHEIGHT;
			}

			void set_viewwidth(int16_t width)
			{
				PHYSWIDTH = width;
			}

			void set_viewheight(int16_t height)
			{
				PHYSHEIGHT = height;
			}

			float get_ui_scale()
			{
				return UI_SCALE;
			}

			void set_ui_scale(float scale)
			{
				if (scale >= 1.0f && scale <= 3.0f)
					UI_SCALE = scale;
			}

		private:
			// Physical window dimensions
			int16_t PHYSWIDTH;
			int16_t PHYSHEIGHT;
			// UI scale factor (physical / logical)
			float UI_SCALE;
		};
	}
}