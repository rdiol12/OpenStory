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
#include "UIRPSGame.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIRPSGame::UIRPSGame() : UIElement(Point<int16_t>(400, 250), Point<int16_t>(0, 0))
	{
		nl::node main = nl::nx::ui["UIWindow2.img"]["RPSGame"];

		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[Buttons::BT_ROCK] = std::make_unique<MapleButton>(main["BtRock"]);
		buttons[Buttons::BT_PAPER] = std::make_unique<MapleButton>(main["BtPaper"]);
		buttons[Buttons::BT_SCISSORS] = std::make_unique<MapleButton>(main["BtScissors"]);

		dimension = bg_dimensions;
	}

	void UIRPSGame::draw(float inter) const
	{
		UIElement::draw(inter);
	}

	void UIRPSGame::update()
	{
		UIElement::update();
	}

	Cursor::State UIRPSGame::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIRPSGame::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIRPSGame::get_type() const
	{
		return TYPE;
	}

	Button::State UIRPSGame::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_ROCK:
		case Buttons::BT_PAPER:
		case Buttons::BT_SCISSORS:
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
