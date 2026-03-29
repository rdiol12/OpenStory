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
	class UIClock : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CLOCK;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIClock();

		void draw(float alpha) const override;
		void update() override;
		void update_screen(int16_t new_width, int16_t new_height) override;

		UIElement::Type get_type() const override;

	private:
		void draw_number(int value, int digits, Point<int16_t> pos) const;

		Texture clock_bg;
		Texture digit[10];
		Texture colon;

		// Background dimensions for centering
		int16_t bg_width;
		int16_t bg_height;

		// Digit dimensions for spacing
		int16_t digit_width;
		int16_t colon_width;

		// Countdown timer tracking
		int64_t last_update_time;
	};
}
