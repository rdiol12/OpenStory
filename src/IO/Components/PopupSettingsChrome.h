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
	// Shared chrome for the small `UIWindow2.img/UserList/PopupSettings`
	// dialog. Cosmic v83 only ships one such backdrop (260x103) along
	// with two title-strip swaps (`titleMake` / `titleSettings`) and
	// `BtSave` / `BtCancel` buttons. UIPartySettings (the no-op
	// "edit settings" shell) and UIPartySearchStart (the start-search
	// mini-form) both consume it, so the texture loads + dimension
	// constants live here.
	struct PopupSettingsChrome
	{
		static constexpr int16_t WIDTH    = 260;
		static constexpr int16_t HEIGHT   = 103;
		static constexpr int16_t DRAGAREA_H = 22;

		// Two swap-in title strips. `make` is the recruiting / start-
		// search variant; `settings` is the regular party-settings
		// variant. Pick one at draw time.
		Texture title_make;
		Texture title_settings;

		// Load all sprites + caller's BtSave/BtCancel from the given
		// PopupSettings node (the consuming UI emplaces the buttons
		// in its own buttons map).
		void load(const nl::node& src);

		// Draw the active title sprite at `pos`. The shared
		// `backgrnd` is loaded as a `sprite` by the caller (so that
		// UIElement::draw_sprites picks it up); only the title varies.
		void draw_title(Point<int16_t> pos, bool make_mode) const;
	};
}
