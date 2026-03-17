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
#include "UIGuild.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuild::UIGuild() : UIDragElement<PosGUILD>(), tab(0)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["GuildWindow"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(src["backgrnd2"]);

		tabbar = src["tabbar"];

		for (size_t i = 0; i <= 5; i++)
		{
			nl::node tab_node = src["tab" + std::to_string(i)];

			if (tab_node)
				backgrounds.emplace_back(tab_node["backgrnd"]);
		}

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg.get_dimensions().x() - 19, 5));
		buttons[Buttons::BT_TAB0] = std::make_unique<MapleButton>(src["BtTab"]["0"]);
		buttons[Buttons::BT_TAB1] = std::make_unique<MapleButton>(src["BtTab"]["1"]);
		buttons[Buttons::BT_TAB2] = std::make_unique<MapleButton>(src["BtTab"]["2"]);
		buttons[Buttons::BT_TAB3] = std::make_unique<MapleButton>(src["BtTab"]["3"]);
		buttons[Buttons::BT_TAB4] = std::make_unique<MapleButton>(src["BtTab"]["4"]);
		buttons[Buttons::BT_TAB5] = std::make_unique<MapleButton>(src["BtTab"]["5"]);

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIGuild::draw(float inter) const
	{
		UIElement::draw(inter);

		tabbar.draw(DrawArgument(position));

		if (tab < backgrounds.size())
			backgrounds[tab].draw(DrawArgument(position));
	}

	void UIGuild::update()
	{
		UIElement::update();
	}

	void UIGuild::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UIGuild::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIGuild::get_type() const
	{
		return TYPE;
	}

	Button::State UIGuild::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
		case Buttons::BT_TAB5:
			tab = buttonid - Buttons::BT_TAB0;
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
