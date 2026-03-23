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
#include "UIGameSettings.h"

#include "../Components/MapleButton.h"
#include "../KeyAction.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGameSettings::UIGameSettings() : UIDragElement<PosGAMESETTINGS>(Point<int16_t>(WIDTH, DRAG_HEIGHT)), checks_state(0)
	{
		nl::node source = nl::nx::ui["UIWindow2.img"]["GameOpt"];

		sprites.emplace_back(source["backgrnd"]);
		sprites.emplace_back(source["backgrnd2"]);
		sprites.emplace_back(source["backgrnd3"]);

		buttons[BT_CANCEL] = std::make_unique<MapleButton>(source["BtCancle"]);
		buttons[BT_OK] = std::make_unique<MapleButton>(source["BtOK"]);

		check_texture = source["check"];

		dimension = Point<int16_t>(WIDTH, HEIGHT);
		dragarea = Point<int16_t>(WIDTH, DRAG_HEIGHT);
	}

	void UIGameSettings::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> check_pos = position + Point<int16_t>(WIDTH - 20, 28);

		for (uint16_t i = 0; i < NUM_CHECKS; i++)
		{
			bool checked = checks_state & static_cast<uint16_t>(1 << i);
			float alpha = checked ? 1.0f : 0.1f;
			check_texture.draw(DrawArgument(check_pos, alpha));
			check_pos.shift_y(STRIDE_VERT);
		}
	}

	Cursor::State UIGameSettings::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> check_offset = cursorpos - position - Point<int16_t>(WIDTH - 20, 28);

		if (check_offset.x() < 0 || check_offset.x() > CHECK_SIDE_LEN || check_offset.y() < 0)
			return UIElement::send_cursor(clicked, cursorpos);

		uint16_t check_idx = check_offset.y() / STRIDE_VERT;

		if (check_idx >= NUM_CHECKS || STRIDE_VERT * check_idx + CHECK_SIDE_LEN < check_offset.y())
			return UIElement::send_cursor(clicked, cursorpos);

		if (clicked)
			checks_state ^= static_cast<uint16_t>(1 << check_idx);

		return Cursor::State::CANCLICK;
	}

	void UIGameSettings::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				deactivate();
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(BT_OK);
		}
	}

	Button::State UIGameSettings::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CANCEL:
			checks_state = 0;
			deactivate();
			return Button::State::NORMAL;
		case BT_OK:
			deactivate();
			return Button::State::NORMAL;
		default:
			return Button::State::PRESSED;
		}
	}

	UIElement::Type UIGameSettings::get_type() const
	{
		return TYPE;
	}
}
