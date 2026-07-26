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

#include "../../Template/Point.h"

namespace ms
{
	class CharLook;

	// The falling/landed tomb (Effect/Tomb.img) and the hovering ghost (the
	// ghost stance baked into the body files) shown while a character is dead.
	class DeathArt
	{
	public:
		void reset();
		void update();
		void draw(Point<int16_t> absp, bool flip, const CharLook& look, float alpha) const;

	private:
		int16_t tomb_yoff = 0;
		bool tomb_landed = false;
		uint8_t tomb_frame = 0;
		uint16_t tomb_elapsed = 0;
		uint8_t ghost_frame = 0;
		uint16_t ghost_elapsed = 0;
		uint16_t ghost_bob = 0;
	};
}
