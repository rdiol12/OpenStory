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

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIOptionMenu : public UIDragElement<PosOPTIONMENU>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::OPTIONMENU;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIOptionMenu();

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void load_settings();
		void save_settings();
		void revert_settings();

		enum Buttons : uint16_t
		{
			BT_OK,
			BT_CANCEL
		};

		// NX sprites
		Texture scroll_normal;
		Texture scroll_mouseover;
		Texture scroll_pressed;
		Texture scroll_disabled;
		Texture check_sprite;

		// Sliders
		enum SliderType : int16_t
		{
			SL_NONE = -1,
			SL_GRAPHICS,
			SL_EFFECTS,
			SL_BGM,
			SL_SFX,
			SL_MOUSE_SPEED,
			SL_HP_WARNING,
			SL_MP_WARNING,
			NUM_SLIDERS
		};

		int16_t slider_y[NUM_SLIDERS];
		int16_t slider_x[NUM_SLIDERS];
		int16_t slider_w[NUM_SLIDERS];
		uint8_t slider_val[NUM_SLIDERS];
		uint8_t slider_orig[NUM_SLIDERS];
		SliderType active_slider = SL_NONE;

		// Checkboxes — each has its own position
		enum CheckType : uint16_t
		{
			CHK_RES_800, CHK_RES_1024,
			CHK_WINDOWED,
			CHK_MUTE_BGM, CHK_MUTE_SFX,
			CHK_SCREEN_SHAKE,
			CHK_MINIMAP_NORMAL, CHK_MINIMAP_SIMPLE,
			// Social (left column)
			CHK_WHISPER, CHK_CHAT_INVITE, CHK_PARTY_INVITE,
			CHK_GUILD_CHAT, CHK_ALLIANCE_CHAT, CHK_FAMILY_INVITE,
			// Social (right column)
			CHK_FRIEND_CHAT, CHK_TRADE_REQUEST, CHK_EXPEDITION_INVITE,
			CHK_GUILD_INVITE, CHK_ALLIANCE_INVITE, CHK_FOLLOW,
			NUM_CHECKS
		};

		Point<int16_t> check_positions[NUM_CHECKS];
		bool check_val[NUM_CHECKS];
		bool check_orig[NUM_CHECKS];

		static constexpr int16_t CHECK_HIT_SIZE = 14;

		// Slider percentage labels
		mutable Text slider_labels[NUM_SLIDERS];
	};
}
