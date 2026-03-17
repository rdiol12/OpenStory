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

#include "../../Template/Singleton.h"
#include "../../Graphics/Texture.h"
#include "../../Graphics/Animation.h"

#include <unordered_map>
#include <string>

namespace ms
{
	// Singleton holding shared Basic.img textures and animations.
	// Prevents duplication of commonly used UI resources across all UIs.
	class UIResources : public Singleton<UIResources>
	{
	public:
		UIResources();

		// Close button variants
		nl::node get_close_button(int variant = 3) const;

		// Minimize button
		nl::node get_hide_button() const;

		// Scrollbar node
		nl::node get_scrollbar() const;

		// Checkbox nodes
		nl::node get_checkbox(bool right_aligned = false) const;

		// Tab bar nodes
		nl::node get_tabbar(int variant = 1) const;

		// Dropdown node
		nl::node get_dropdown() const;

		// Gauge node
		nl::node get_gauge() const;

		// Progress bar node
		nl::node get_progressbar() const;

		// Tooltip node
		nl::node get_tooltip() const;

		// Slider bar node
		nl::node get_sliderbar() const;

		// Dialog balloon node
		nl::node get_dialog_balloon() const;

		// Item slot node
		nl::node get_itemslot() const;

		// Check if resources loaded successfully
		bool loaded() const;

	private:
		nl::node basic;
		bool is_loaded;
	};
}
