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
#include "UIOptionMenu.h"

#include "../KeyAction.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "../Window.h"
#include "UIMiniMap.h"

#include "../../Audio/Audio.h"
#include "../../Configuration.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIOptionMenu::UIOptionMenu() : UIDragElement<PosOPTIONMENU>(Point<int16_t>(283, 20)), active_slider(SL_NONE)
	{
		// Always position the menu at the center of the screen
		int16_t screen_w = Constants::Constants::get().get_viewwidth();
		int16_t screen_h = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>(
			(screen_w - 283) / 2,
			(screen_h - 558) / 2
		);

		nl::node OptionMenu = nl::nx::ui["UIWindow2.img"]["OptionMenu"];

		if (OptionMenu)
		{
			sprites.emplace_back(OptionMenu["backgrnd"]);
			sprites.emplace_back(OptionMenu["backgrnd2"]);
			sprites.emplace_back(OptionMenu["backgrnd3"]);

			buttons[BT_OK] = std::make_unique<MapleButton>(OptionMenu["BtOK"]);
			buttons[BT_CANCEL] = std::make_unique<MapleButton>(OptionMenu["BtCancle"]);

			nl::node scroll = OptionMenu["scroll"];
			scroll_normal = scroll["0"];
			scroll_mouseover = scroll["1"];
			scroll_pressed = scroll["2"];
			scroll_disabled = scroll["3"];

			check_sprite = OptionMenu["check"];

			dimension = Point<int16_t>(283, 558);
			dragarea = Point<int16_t>(283, 20);
		}
		else
		{
			dimension = Point<int16_t>(283, 558);
			dragarea = Point<int16_t>(283, 20);
		}

		// Slider layout — per-slider track position and width
		// Common sliders
		for (int i = 0; i < NUM_SLIDERS; i++)
		{
			slider_x[i] = 95;
			slider_w[i] = 110;
		}

		// Sound sliders have a shorter track
		slider_x[SL_BGM] = 95;
		slider_w[SL_BGM] = 65;
		slider_x[SL_SFX] = 95;
		slider_w[SL_SFX] = 65;

		// Slider Y positions (matching background sections)
		slider_y[SL_GRAPHICS]    = 30;
		slider_y[SL_EFFECTS]     = 60;
		slider_y[SL_BGM]         = 150;
		slider_y[SL_SFX]         = 180;
		slider_y[SL_MOUSE_SPEED] = 240;
		slider_y[SL_HP_WARNING]  = 270;
		slider_y[SL_MP_WARNING]  = 300;

		// Checkbox positions — using confirmed slider Y positions as anchors:
		// Sliders: GRAPHICS=30, EFFECTS=60, BGM=150, SFX=180, MOUSE=240, HP=270, MP=300
		// RESOLUTION is between EFFECTS(60) and BGM(150)
		check_positions[CHK_RES_800]  = Point<int16_t>(69, 94);
		check_positions[CHK_RES_1024] = Point<int16_t>(170, 94);
		check_positions[CHK_WINDOWED] = Point<int16_t>(70, 123);
		check_positions[CHK_MUTE_BGM] = Point<int16_t>(228, 154);
		check_positions[CHK_MUTE_SFX] = Point<int16_t>(228, 184);
		check_positions[CHK_SCREEN_SHAKE] = Point<int16_t>(70, 335);
		check_positions[CHK_MINIMAP_NORMAL] = Point<int16_t>(69, 393);
		check_positions[CHK_MINIMAP_SIMPLE] = Point<int16_t>(171, 392);

		// Social — left column
		check_positions[CHK_WHISPER]           = Point<int16_t>(128, 423);
		check_positions[CHK_CHAT_INVITE]       = Point<int16_t>(127, 442);
		check_positions[CHK_PARTY_INVITE]      = Point<int16_t>(129, 459);
		check_positions[CHK_GUILD_CHAT]        = Point<int16_t>(127, 477);
		check_positions[CHK_ALLIANCE_CHAT]     = Point<int16_t>(128, 495);
		check_positions[CHK_FAMILY_INVITE]     = Point<int16_t>(128, 512);

		// Social — right column
		check_positions[CHK_FRIEND_CHAT]       = Point<int16_t>(261, 423);
		check_positions[CHK_TRADE_REQUEST]     = Point<int16_t>(260, 442);
		check_positions[CHK_EXPEDITION_INVITE] = Point<int16_t>(260, 459);
		check_positions[CHK_GUILD_INVITE]      = Point<int16_t>(262, 479);
		check_positions[CHK_ALLIANCE_INVITE]   = Point<int16_t>(261, 496);
		check_positions[CHK_FOLLOW]            = Point<int16_t>(260, 513);

		// Initialize all checkbox values
		for (int i = 0; i < NUM_CHECKS; i++)
		{
			check_val[i] = false;
			check_orig[i] = false;
		}

		// Default social checkboxes to checked (ALLOW)
		for (int i = CHK_WHISPER; i <= CHK_FOLLOW; i++)
		{
			check_val[i] = true;
			check_orig[i] = true;
		}

		// Default minimap to normal
		check_val[CHK_MINIMAP_NORMAL] = true;
		check_orig[CHK_MINIMAP_NORMAL] = true;

		// Initialize slider percentage labels
		for (int i = 0; i < NUM_SLIDERS; i++)
			slider_labels[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);

		load_settings();
	}

	void UIOptionMenu::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw all slider thumbs and percentage labels
		for (int i = 0; i < NUM_SLIDERS; i++)
		{
			int16_t thumb_x = slider_x[i] + static_cast<int16_t>(slider_val[i] * slider_w[i] / 100);
			Point<int16_t> thumb_pos = position + Point<int16_t>(thumb_x, slider_y[i]);

			if (active_slider == i && scroll_pressed.is_valid())
				scroll_pressed.draw(DrawArgument(thumb_pos));
			else if (scroll_normal.is_valid())
				scroll_normal.draw(DrawArgument(thumb_pos));

			// Draw percentage text to the right of the slider track
			slider_labels[i].change_text(std::to_string(slider_val[i]) + "%");
			int16_t label_x = slider_x[i] + slider_w[i] + 14;
			Point<int16_t> label_pos = position + Point<int16_t>(label_x, slider_y[i] - 6);
			slider_labels[i].draw(DrawArgument(label_pos));
		}

		// Draw checkboxes
		if (check_sprite.is_valid())
		{
			for (int i = 0; i < NUM_CHECKS; i++)
			{
				if (check_val[i])
					check_sprite.draw(DrawArgument(position + check_positions[i], 1.0f));
			}
		}

		UIElement::draw_buttons(inter);
	}

	Button::State UIOptionMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CANCEL:
			revert_settings();
			deactivate();
			return Button::State::NORMAL;
		case BT_OK:
			save_settings();
			deactivate();
			return Button::State::NORMAL;
		default:
			return Button::State::DISABLED;
		}
	}

	Cursor::State UIOptionMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Continue slider drag
		if (clicked && active_slider != SL_NONE)
		{
			int16_t tx = slider_x[active_slider];
			int16_t tw = slider_w[active_slider];
			int16_t relative_x = cursorpos.x() - position.x() - tx;
			if (relative_x < 0) relative_x = 0;
			if (relative_x > tw) relative_x = tw;

			slider_val[active_slider] = static_cast<uint8_t>(relative_x * 100 / tw);

			// Apply live audio preview for BGM/SFX
			if (active_slider == SL_BGM && !check_val[CHK_MUTE_BGM])
				Music::set_bgmvolume(slider_val[SL_BGM]);
			else if (active_slider == SL_SFX && !check_val[CHK_MUTE_SFX])
				Sound::set_sfxvolume(slider_val[SL_SFX]);

			return Cursor::State::CLICKING;
		}
		else if (!clicked && active_slider != SL_NONE)
		{
			active_slider = SL_NONE;
		}

		if (clicked)
		{
			int16_t rel_x = cursorpos.x() - position.x();
			int16_t rel_y = cursorpos.y() - position.y();
			int16_t thumb_half = 10;



			// Check all slider areas
			for (int i = 0; i < NUM_SLIDERS; i++)
			{
				int16_t tx = slider_x[i];
				int16_t tw = slider_w[i];

				if (rel_x >= tx - thumb_half && rel_x <= tx + tw + thumb_half &&
					rel_y >= slider_y[i] - thumb_half && rel_y <= slider_y[i] + thumb_half)
				{
					active_slider = static_cast<SliderType>(i);
					int16_t track_x = rel_x - tx;
					if (track_x < 0) track_x = 0;
					if (track_x > tw) track_x = tw;
					slider_val[i] = static_cast<uint8_t>(track_x * 100 / tw);

					if (i == SL_BGM && !check_val[CHK_MUTE_BGM])
						Music::set_bgmvolume(slider_val[SL_BGM]);
					else if (i == SL_SFX && !check_val[CHK_MUTE_SFX])
						Sound::set_sfxvolume(slider_val[SL_SFX]);

					return Cursor::State::CLICKING;
				}
			}

			// Check all checkbox areas
			for (int i = 0; i < NUM_CHECKS; i++)
			{
				Point<int16_t> chk = check_positions[i];
				if (rel_x >= chk.x() - 2 && rel_x <= chk.x() + CHECK_HIT_SIZE &&
					rel_y >= chk.y() - 2 && rel_y <= chk.y() + CHECK_HIT_SIZE)
				{
					// Handle radio button groups (only one can be active)
					if (i == CHK_RES_800 || i == CHK_RES_1024)
					{
						check_val[CHK_RES_800] = (i == CHK_RES_800);
						check_val[CHK_RES_1024] = (i == CHK_RES_1024);
					}
					else if (i == CHK_MINIMAP_NORMAL || i == CHK_MINIMAP_SIMPLE)
					{
						check_val[CHK_MINIMAP_NORMAL] = (i == CHK_MINIMAP_NORMAL);
						check_val[CHK_MINIMAP_SIMPLE] = (i == CHK_MINIMAP_SIMPLE);
					}
					else
					{
						check_val[i] = !check_val[i];

						// Apply mute immediately
						if (i == CHK_MUTE_BGM)
							Music::set_bgmvolume(check_val[CHK_MUTE_BGM] ? 0 : slider_val[SL_BGM]);
						else if (i == CHK_MUTE_SFX)
							Sound::set_sfxvolume(check_val[CHK_MUTE_SFX] ? 0 : slider_val[SL_SFX]);
					}

					return Cursor::State::CANCLICK;
				}
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIOptionMenu::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				revert_settings();
				deactivate();
			}
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(BT_OK);
		}
	}

	UIElement::Type UIOptionMenu::get_type() const
	{
		return TYPE;
	}

	void UIOptionMenu::load_settings()
	{
		slider_val[SL_BGM] = Setting<BGMVolume>::get().load();
		slider_val[SL_SFX] = Setting<SFXVolume>::get().load();
		slider_val[SL_GRAPHICS] = Setting<GraphicsQuality>::get().load();
		slider_val[SL_EFFECTS] = Setting<EffectsQuality>::get().load();
		slider_val[SL_MOUSE_SPEED] = Setting<MouseSpeed>::get().load();
		slider_val[SL_HP_WARNING] = Setting<HPWarning>::get().load();
		slider_val[SL_MP_WARNING] = Setting<MPWarning>::get().load();

		// Resolution radio buttons (cosmetic only — does not change resolution)
		check_val[CHK_RES_800] = false;
		check_val[CHK_RES_1024] = true;

		// Windowed checkbox (inverted from Fullscreen setting)
		check_val[CHK_WINDOWED] = !Setting<Fullscreen>::get().load();

		// Screen shake
		check_val[CHK_SCREEN_SHAKE] = Setting<ScreenShake>::get().load();

		// Minimap mode
		bool simple_mode = Setting<MiniMapSimpleMode>::get().load();
		check_val[CHK_MINIMAP_NORMAL] = !simple_mode;
		check_val[CHK_MINIMAP_SIMPLE] = simple_mode;

		// Social filters
		check_val[CHK_WHISPER]           = Setting<AllowWhisper>::get().load();
		check_val[CHK_CHAT_INVITE]       = Setting<AllowChatInvite>::get().load();
		check_val[CHK_PARTY_INVITE]      = Setting<AllowPartyInvite>::get().load();
		check_val[CHK_GUILD_CHAT]        = Setting<AllowGuildChat>::get().load();
		check_val[CHK_ALLIANCE_CHAT]     = Setting<AllowAllianceChat>::get().load();
		check_val[CHK_FAMILY_INVITE]     = Setting<AllowFamilyInvite>::get().load();
		check_val[CHK_FRIEND_CHAT]       = Setting<AllowFriendChat>::get().load();
		check_val[CHK_TRADE_REQUEST]     = Setting<AllowTradeRequest>::get().load();
		check_val[CHK_EXPEDITION_INVITE] = Setting<AllowExpeditionInvite>::get().load();
		check_val[CHK_GUILD_INVITE]      = Setting<AllowGuildInvite>::get().load();
		check_val[CHK_ALLIANCE_INVITE]   = Setting<AllowAllianceInvite>::get().load();
		check_val[CHK_FOLLOW]            = Setting<AllowFollow>::get().load();

		// Save originals for cancel/revert
		for (int i = 0; i < NUM_SLIDERS; i++)
			slider_orig[i] = slider_val[i];

		for (int i = 0; i < NUM_CHECKS; i++)
			check_orig[i] = check_val[i];
	}

	void UIOptionMenu::save_settings()
	{
		// Audio sliders
		Setting<BGMVolume>::get().save(slider_val[SL_BGM]);
		Setting<SFXVolume>::get().save(slider_val[SL_SFX]);

		// Other sliders
		Setting<GraphicsQuality>::get().save(slider_val[SL_GRAPHICS]);
		Setting<EffectsQuality>::get().save(slider_val[SL_EFFECTS]);
		Setting<MouseSpeed>::get().save(slider_val[SL_MOUSE_SPEED]);
		Window::get().apply_mouse_speed(slider_val[SL_MOUSE_SPEED]);
		Setting<HPWarning>::get().save(slider_val[SL_HP_WARNING]);
		Setting<MPWarning>::get().save(slider_val[SL_MP_WARNING]);

		// Windowed checkbox (inverted — windowed means NOT fullscreen)
		bool want_fullscreen = !check_val[CHK_WINDOWED];
		bool was_fullscreen = Setting<Fullscreen>::get().load();

		if (want_fullscreen != was_fullscreen)
		{
			Setting<Fullscreen>::get().save(want_fullscreen);
			Window::get().toggle_fullscreen();
		}

		// Screen shake
		Setting<ScreenShake>::get().save(check_val[CHK_SCREEN_SHAKE]);

		// Minimap mode — recreate minimap if mode changed
		bool old_simple = Setting<MiniMapSimpleMode>::get().load();
		Setting<MiniMapSimpleMode>::get().save(check_val[CHK_MINIMAP_SIMPLE]);

		if (check_val[CHK_MINIMAP_SIMPLE] != old_simple)
		{
			UI::get().remove(UIElement::Type::MINIMAP);
			UI::get().emplace<UIMiniMap>(Stage::get().get_player().get_stats());
		}

		// Social filters
		Setting<AllowWhisper>::get().save(check_val[CHK_WHISPER]);
		Setting<AllowChatInvite>::get().save(check_val[CHK_CHAT_INVITE]);
		Setting<AllowPartyInvite>::get().save(check_val[CHK_PARTY_INVITE]);
		Setting<AllowGuildChat>::get().save(check_val[CHK_GUILD_CHAT]);
		Setting<AllowAllianceChat>::get().save(check_val[CHK_ALLIANCE_CHAT]);
		Setting<AllowFamilyInvite>::get().save(check_val[CHK_FAMILY_INVITE]);
		Setting<AllowFriendChat>::get().save(check_val[CHK_FRIEND_CHAT]);
		Setting<AllowTradeRequest>::get().save(check_val[CHK_TRADE_REQUEST]);
		Setting<AllowExpeditionInvite>::get().save(check_val[CHK_EXPEDITION_INVITE]);
		Setting<AllowGuildInvite>::get().save(check_val[CHK_GUILD_INVITE]);
		Setting<AllowAllianceInvite>::get().save(check_val[CHK_ALLIANCE_INVITE]);
		Setting<AllowFollow>::get().save(check_val[CHK_FOLLOW]);

		// Apply final audio
		Music::set_bgmvolume(check_val[CHK_MUTE_BGM] ? 0 : slider_val[SL_BGM]);
		Sound::set_sfxvolume(check_val[CHK_MUTE_SFX] ? 0 : slider_val[SL_SFX]);

		Configuration::get().save();

		// Update originals
		for (int i = 0; i < NUM_SLIDERS; i++)
			slider_orig[i] = slider_val[i];

		for (int i = 0; i < NUM_CHECKS; i++)
			check_orig[i] = check_val[i];
	}

	void UIOptionMenu::revert_settings()
	{
		for (int i = 0; i < NUM_SLIDERS; i++)
			slider_val[i] = slider_orig[i];

		for (int i = 0; i < NUM_CHECKS; i++)
			check_val[i] = check_orig[i];

		Music::set_bgmvolume(check_val[CHK_MUTE_BGM] ? 0 : slider_val[SL_BGM]);
		Sound::set_sfxvolume(check_val[CHK_MUTE_SFX] ? 0 : slider_val[SL_SFX]);
	}

}
