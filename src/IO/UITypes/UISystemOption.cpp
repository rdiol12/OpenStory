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
#include "UISystemOption.h"

#include "../Components/MapleButton.h"
#include "../Keyboard.h"

#include "../../Audio/Audio.h"
#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UISystemOption::UISystemOption() : UIDragElement<PosSYSTEMOPTION>(), dragging(DRAG_NONE)
	{
		nl::node SystemOption = nl::nx::ui["UIWindow2.img"]["SystemOption"];

		if (!SystemOption)
			SystemOption = nl::nx::ui["UIWindow.img"]["SystemOption"];

		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = SystemOption["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// Load additional background sprites if present
		nl::node backgrnd2 = SystemOption["backgrnd2"];

		if (backgrnd2)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = SystemOption["backgrnd3"];

		if (backgrnd3)
			sprites.emplace_back(backgrnd3);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(SystemOption["BtOK"]);
		buttons[Buttons::BT_CANCEL] = std::make_unique<MapleButton>(SystemOption["BtCancel"]);

		// Load slider thumb textures
		nl::node slider = SystemOption["slider"];

		if (slider)
		{
			slider_thumb_normal = slider["enabled"]["thumb0"];
			slider_thumb_pressed = slider["enabled"]["thumb1"];
		}
		else
		{
			// Fallback: try alternate node names used in some NX versions
			nl::node gameOpt = SystemOption["GameOpt"];

			if (gameOpt)
			{
				nl::node optSlider = gameOpt["slider"];

				if (optSlider)
				{
					slider_thumb_normal = optSlider["enabled"]["thumb0"];
					slider_thumb_pressed = optSlider["enabled"]["thumb1"];
				}
			}
		}

		// Slider track layout constants
		// These match the v83 SystemOption panel layout
		slider_track_x = 72;
		slider_track_width = 128;
		bgm_slider_y = 56;
		sfx_slider_y = 83;

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		// Load current settings
		load_settings();
	}

	void UISystemOption::draw(float inter) const
	{
		UIElement::draw(inter);

		// Draw BGM volume slider thumb
		int16_t bgm_thumb_x = vol_to_slider_x(bgm_volume);
		Point<int16_t> bgm_thumb_pos = position + Point<int16_t>(bgm_thumb_x, bgm_slider_y);

		if (dragging == DRAG_BGM)
			slider_thumb_pressed.draw(DrawArgument(bgm_thumb_pos));
		else
			slider_thumb_normal.draw(DrawArgument(bgm_thumb_pos));

		// Draw SFX volume slider thumb
		int16_t sfx_thumb_x = vol_to_slider_x(sfx_volume);
		Point<int16_t> sfx_thumb_pos = position + Point<int16_t>(sfx_thumb_x, sfx_slider_y);

		if (dragging == DRAG_SFX)
			slider_thumb_pressed.draw(DrawArgument(sfx_thumb_pos));
		else
			slider_thumb_normal.draw(DrawArgument(sfx_thumb_pos));
	}

	void UISystemOption::update()
	{
		UIElement::update();
	}

	Cursor::State UISystemOption::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked && dragging != DRAG_NONE)
		{
			// Continue dragging the active slider
			int16_t relative_x = cursorpos.x() - position.x() - slider_track_x;

			if (relative_x < 0)
				relative_x = 0;

			if (relative_x > slider_track_width)
				relative_x = slider_track_width;

			uint8_t new_vol = slider_x_to_vol(relative_x);

			if (dragging == DRAG_BGM)
			{
				bgm_volume = new_vol;
				Music::set_bgmvolume(bgm_volume);
			}
			else if (dragging == DRAG_SFX)
			{
				sfx_volume = new_vol;
				Sound::set_sfxvolume(sfx_volume);
			}

			return Cursor::State::CLICKING;
		}
		else if (!clicked && dragging != DRAG_NONE)
		{
			// Release drag
			dragging = DRAG_NONE;
		}

		if (clicked)
		{
			// Check if clicking on a slider thumb or track
			int16_t rel_x = cursorpos.x() - position.x();
			int16_t rel_y = cursorpos.y() - position.y();

			int16_t thumb_half_w = 8;
			int16_t thumb_half_h = 8;

			// Check BGM slider area
			int16_t bgm_thumb_x = vol_to_slider_x(bgm_volume);

			if (rel_x >= slider_track_x - thumb_half_w && rel_x <= slider_track_x + slider_track_width + thumb_half_w &&
				rel_y >= bgm_slider_y - thumb_half_h && rel_y <= bgm_slider_y + thumb_half_h)
			{
				dragging = DRAG_BGM;

				int16_t track_x = rel_x - slider_track_x;

				if (track_x < 0)
					track_x = 0;

				if (track_x > slider_track_width)
					track_x = slider_track_width;

				bgm_volume = slider_x_to_vol(track_x);
				Music::set_bgmvolume(bgm_volume);

				return Cursor::State::CLICKING;
			}

			// Check SFX slider area
			int16_t sfx_thumb_x = vol_to_slider_x(sfx_volume);

			if (rel_x >= slider_track_x - thumb_half_w && rel_x <= slider_track_x + slider_track_width + thumb_half_w &&
				rel_y >= sfx_slider_y - thumb_half_h && rel_y <= sfx_slider_y + thumb_half_h)
			{
				dragging = DRAG_SFX;

				int16_t track_x = rel_x - slider_track_x;

				if (track_x < 0)
					track_x = 0;

				if (track_x > slider_track_width)
					track_x = slider_track_width;

				sfx_volume = slider_x_to_vol(track_x);
				Sound::set_sfxvolume(sfx_volume);

				return Cursor::State::CLICKING;
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UISystemOption::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				// Cancel: revert volumes to saved values and close
				load_settings();
				Music::set_bgmvolume(bgm_volume);
				Sound::set_sfxvolume(sfx_volume);
				deactivate();
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				button_pressed(Buttons::BT_OK);
			}
		}
	}

	UIElement::Type UISystemOption::get_type() const
	{
		return TYPE;
	}

	Button::State UISystemOption::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
		case Buttons::BT_CANCEL:
			// Revert to saved values
			load_settings();
			Music::set_bgmvolume(bgm_volume);
			Sound::set_sfxvolume(sfx_volume);
			deactivate();
			break;
		case Buttons::BT_OK:
			// Save current values to configuration
			save_settings();
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UISystemOption::load_settings()
	{
		bgm_volume = Setting<BGMVolume>::get().load();
		sfx_volume = Setting<SFXVolume>::get().load();
	}

	void UISystemOption::save_settings()
	{
		Setting<BGMVolume>::get().save(bgm_volume);
		Setting<SFXVolume>::get().save(sfx_volume);
	}

	int16_t UISystemOption::vol_to_slider_x(uint8_t volume) const
	{
		return slider_track_x + static_cast<int16_t>(volume * slider_track_width / 100);
	}

	uint8_t UISystemOption::slider_x_to_vol(int16_t x) const
	{
		if (x <= 0)
			return 0;

		if (x >= slider_track_width)
			return 100;

		return static_cast<uint8_t>(x * 100 / slider_track_width);
	}
}
