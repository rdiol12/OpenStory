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

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIComboCounter : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::COMBO;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIComboCounter();

		void draw(float alpha) const override;
		void update() override;
		void update_screen(int16_t new_width, int16_t new_height) override;

		UIElement::Type get_type() const override;

	private:
		void update_text();

		Text combo_text;
		Text combo_shadow;
		Text label_text;
		Text label_shadow;
		ColorBox background;

		int32_t displayed_combo;
		int64_t fade_timer;

		static constexpr int16_t BG_WIDTH = 140;
		static constexpr int16_t BG_HEIGHT = 44;
		static constexpr int64_t FADE_DELAY = 3000;
	};
}
