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

#include "../UIDragElement.h"

#include "../../Graphics/Texture.h"

namespace ms
{
	class UISystemOption : public UIDragElement<PosSYSTEMOPTION>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SYSTEMOPTION;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UISystemOption();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void load_settings();
		void save_settings();
		int16_t vol_to_slider_x(uint8_t volume) const;
		uint8_t slider_x_to_vol(int16_t x) const;

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_OK,
			BT_CANCEL
		};

		// Slider track positions (relative to window position)
		int16_t slider_track_x;
		int16_t slider_track_width;
		int16_t bgm_slider_y;
		int16_t sfx_slider_y;

		// Slider thumb textures
		Texture slider_thumb_normal;
		Texture slider_thumb_pressed;

		// Current slider values (0-100)
		uint8_t bgm_volume;
		uint8_t sfx_volume;

		// Dragging state
		enum DragTarget
		{
			DRAG_NONE,
			DRAG_BGM,
			DRAG_SFX
		};

		DragTarget dragging;
	};
}
