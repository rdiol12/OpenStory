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
#include "UISystemMenu.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "UIChannel.h"
#include "UIGameSettings.h"
#include "UIKeyConfig.h"
#include "UINotice.h"
#include "UIOptionMenu.h"

#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UISystemMenu::UISystemMenu() : UIElement(Point<int16_t>(673, 527), Point<int16_t>(WIDTH, HEIGHT), true)
	{
		nl::node source = nl::nx::ui["StatusBar2.img"]["mainBar"]["System"];

		nl::node background = source["backgrnd"];
		top = background["0"];
		mid = background["1"];
		bottom = background["2"];

		std::string button_names[NUM_BUTTONS] = {
			"BtChannel", "BtMonsterLife", "BtKeySetting",
			"BtGameOption", "BtSystemOption", "BtGameQuit"
		};

		int16_t y_offset = PADDING_TOP;

		for (uint16_t i = 0; i < NUM_BUTTONS; i++)
		{
			buttons[i] = std::make_unique<MapleButton>(
				source[button_names[i]],
				Point<int16_t>(BUTTON_PADDING_HORIZ, y_offset)
			);
			y_offset += STRIDE_VERT;
		}
	}

	void UISystemMenu::draw(float inter) const
	{
		Point<int16_t> bg_pos = position;

		top.draw(DrawArgument(bg_pos));
		bg_pos.shift_y(top.get_dimensions().y());

		int16_t mid_height = HEIGHT - top.get_dimensions().y() - bottom.get_dimensions().y();

		for (int16_t y = 0; y < mid_height; y++)
			mid.draw(DrawArgument(bg_pos + Point<int16_t>(0, y)));

		bg_pos.shift_y(mid_height);
		bottom.draw(DrawArgument(bg_pos));

		UIElement::draw_buttons(inter);
	}

	Cursor::State UISystemMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Close if clicked outside the menu bounds
		if (clicked)
		{
			Rectangle<int16_t> menu_area(position, position + Point<int16_t>(WIDTH, HEIGHT));

			if (!menu_area.contains(cursorpos))
			{
				active = false;
				return Cursor::State::IDLE;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	Button::State UISystemMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CHANNEL:
			UI::get().emplace<UIChannel>();
			break;
		case BT_FARM:
			break;
		case BT_KEY_SETTING:
			UI::get().emplace<UIKeyConfig>(
				Stage::get().get_player().get_inventory(),
				Stage::get().get_player().get_skills()
			);
			break;
		case BT_GAME_OPTION:
			UI::get().emplace<UIGameSettings>();
			break;
		case BT_SYSTEM_OPTION:
			UI::get().emplace<UIOptionMenu>();
			break;
		case BT_QUIT:
		{
			auto onok = [](bool yes)
			{
				if (yes)
					UI::get().quit();
			};

			UI::get().emplace<UIOk>("Are you sure you want to exit the game?", onok);
			break;
		}
		}

		active = false;
		return Button::State::NORMAL;
	}

	UIElement::Type UISystemMenu::get_type() const
	{
		return TYPE;
	}
}
