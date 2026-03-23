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

#include "../UIElement.h"
#include "../../Graphics/Texture.h"

namespace ms
{
	class UISystemMenu : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SYSTEMMENU;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UISystemMenu();

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CHANNEL,
			BT_FARM,
			BT_KEY_SETTING,
			BT_GAME_OPTION,
			BT_SYSTEM_OPTION,
			BT_QUIT,
			NUM_BUTTONS
		};

		Texture top;
		Texture mid;
		Texture bottom;

		static constexpr int16_t WIDTH = 79;
		static constexpr int16_t BUTTON_PADDING_HORIZ = 8;
		static constexpr int16_t PADDING_TOP = 20;
		static constexpr int16_t STRIDE_VERT = 27;
		static constexpr int16_t PADDING_BOTTOM = 12;
		static constexpr int16_t HEIGHT = PADDING_TOP + STRIDE_VERT * NUM_BUTTONS + PADDING_BOTTOM;
	};
}
