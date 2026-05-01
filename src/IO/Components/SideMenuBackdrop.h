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
#include "../../Template/Point.h"

#include <cstdint>

namespace nl { class node; }

namespace ms
{
	// Shared 3-piece chrome used by every right-click "side menu" in
	// the user-list family (buddy context menu, party member context
	// menu, and any future variant). The NX node is expected to ship
	// `top` (header strip), `center` (single-row body slice tiled
	// vertically) and `bottom` (footer strip).
	//
	// Both consumers had identical load+draw+height code copy-pasted
	// before; routing through this struct keeps the geometry numbers
	// in one place.
	struct SideMenuBackdrop
	{
		// Stock dimensions for the SideMenu/* sprites Cosmic v83
		// ships under UIWindow2.img/UserList/Main/<tab>/SideMenu.
		static constexpr int16_t WIDTH    = 88;
		static constexpr int16_t TOP_H    = 22;
		static constexpr int16_t BOTTOM_H = 22;
		static constexpr int16_t BUTTON_H = 14;
		static constexpr int16_t BUTTON_X = 4;

		Texture top;
		Texture mid;
		Texture bottom;

		// Load top/center/bottom from the given SideMenu node.
		void load(const nl::node& menu);

		// Total panel height for `visible_count` body buttons.
		static int16_t height_for(size_t visible_count)
		{
			return TOP_H
				+ static_cast<int16_t>(visible_count) * BUTTON_H
				+ BOTTOM_H;
		}

		// Draw the 3-piece backdrop at `pos` for `visible_count` rows.
		// Center slice is tiled vertically once per row.
		void draw(Point<int16_t> pos, size_t visible_count) const;
	};
}
