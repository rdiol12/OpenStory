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
#include "UISystemOption.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UISystemOption::UISystemOption() : UIDragElement<PosSYSTEMOPTION>()
	{
		nl::node SystemOption = nl::nx::ui["UIWindow2.img"]["SystemOption"];

		if (!SystemOption)
			SystemOption = nl::nx::ui["UIWindow.img"]["SystemOption"];

		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = SystemOption["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(SystemOption["BtOK"]);
		buttons[Buttons::BT_CANCEL] = std::make_unique<MapleButton>(SystemOption["BtCancel"]);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UISystemOption::draw(float inter) const
	{
		UIElement::draw(inter);
	}

	void UISystemOption::update()
	{
		UIElement::update();
	}

	Cursor::State UISystemOption::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UISystemOption::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UISystemOption::get_type() const
	{
		return TYPE;
	}

	Button::State UISystemOption::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
		case Buttons::BT_CANCEL:
			deactivate();
			break;
		case Buttons::BT_OK:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
