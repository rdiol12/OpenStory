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
#include "UIResources.h"

#include <iostream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIResources::UIResources()
	{
		basic = nl::nx::ui["Basic.img"];
		is_loaded = basic.size() > 0;

		if (is_loaded)
			std::cout << "[UIResources] Basic.img loaded successfully (" << basic.size() << " nodes)" << std::endl;
		else
			std::cout << "[UIResources] Warning: Basic.img failed to load" << std::endl;
	}

	nl::node UIResources::get_close_button(int variant) const
	{
		switch (variant)
		{
		case 1:
			return basic["BtClose"];
		case 2:
			return basic["BtClose2"];
		case 3:
		default:
			return basic["BtClose3"];
		}
	}

	nl::node UIResources::get_hide_button() const
	{
		return basic["BtHide"];
	}

	nl::node UIResources::get_scrollbar() const
	{
		return basic["ScrollBar"];
	}

	nl::node UIResources::get_checkbox(bool right_aligned) const
	{
		if (right_aligned)
			return basic["CheckBoxR"];

		return basic["CheckBox"];
	}

	nl::node UIResources::get_tabbar(int variant) const
	{
		if (variant == 2)
			return basic["TabBar2"];

		return basic["TabBar"];
	}

	nl::node UIResources::get_dropdown() const
	{
		return basic["DropDown"];
	}

	nl::node UIResources::get_gauge() const
	{
		return basic["Gauge"];
	}

	nl::node UIResources::get_progressbar() const
	{
		return basic["ProgressBar"];
	}

	nl::node UIResources::get_tooltip() const
	{
		return basic["Tooltip"];
	}

	nl::node UIResources::get_sliderbar() const
	{
		return basic["SliderBar"];
	}

	nl::node UIResources::get_dialog_balloon() const
	{
		return basic["DialogBalloon"];
	}

	nl::node UIResources::get_itemslot() const
	{
		return basic["ItemSlot"];
	}

	bool UIResources::loaded() const
	{
		return is_loaded;
	}
}
