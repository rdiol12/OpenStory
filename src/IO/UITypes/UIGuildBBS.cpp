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
#include "UIGuildBBS.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuildBBS::UIGuildBBS() : UIDragElement<PosGUILDBBS>()
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["GuildBBS"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(src["backgrnd2"]);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg.get_dimensions().x() - 19, 5));
		buttons[Buttons::BT_WRITE] = std::make_unique<MapleButton>(src["BtWrite"]);
		buttons[Buttons::BT_PREV] = std::make_unique<MapleButton>(src["BtPrev"]);
		buttons[Buttons::BT_NEXT] = std::make_unique<MapleButton>(src["BtNext"]);

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIGuildBBS::draw(float inter) const
	{
		UIElement::draw(inter);
	}

	void UIGuildBBS::update()
	{
		UIElement::update();
	}

	void UIGuildBBS::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UIGuildBBS::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIGuildBBS::get_type() const
	{
		return TYPE;
	}

	Button::State UIGuildBBS::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_WRITE:
			break;
		case Buttons::BT_PREV:
			break;
		case Buttons::BT_NEXT:
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
