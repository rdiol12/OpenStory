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
#include "UIJoypad.h"

#include "../Components/MapleButton.h"
#include "../Gamepad.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Maps each setting slot (excluding NAME) to a KeyAction
	static constexpr KeyAction::Id SETTING_ACTIONS[] = {
		KeyAction::Id::LENGTH,          // NAME (no action)
		KeyAction::Id::ATTACK,
		KeyAction::Id::JUMP,
		KeyAction::Id::PICKUP,
		KeyAction::Id::EQUIPMENT,
		KeyAction::Id::ITEMS,
		KeyAction::Id::STATS,
		KeyAction::Id::SKILLS,
		KeyAction::Id::FRIENDS,
		KeyAction::Id::WORLDMAP,
		KeyAction::Id::MINIMAP,
		KeyAction::Id::QUESTLOG
	};

	UIJoypad::UIJoypad() : UIDragElement<PosJOYPAD>(Point<int16_t>(250, 20))
	{
		alternative_settings = false;
		selected_row = -1;
		waiting_for_input = false;

		nl::node JoyPad = nl::nx::ui["UIWindow.img"]["JoyPad"];
		nl::node Basic = nl::nx::ui["Basic.img"];

		backgrnd[true] = JoyPad["backgrnd_alternative"];
		backgrnd[false] = JoyPad["backgrnd_classic"];

		buttons[Buttons::DEFAULT] = std::make_unique<MapleButton>(JoyPad["BtDefault"], Point<int16_t>(18, 303));
		buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(Basic["BtCancel4"], Point<int16_t>(124, 303));
		buttons[Buttons::OK] = std::make_unique<MapleButton>(Basic["BtOK4"], Point<int16_t>(82, 303));

		for (size_t i = 0; i < Setting::SETTING_NUM; i++)
			assigned_button[i] = Gamepad::GP_NONE;

		for (Text& text : key_text)
			text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, "None");

		load_from_gamepad();
		refresh_texts();

		dimension = backgrnd[true].get_dimensions();
	}

	void UIJoypad::load_from_gamepad()
	{
		auto& gp = Gamepad::get();

		for (size_t i = 0; i < Setting::SETTING_NUM; i++)
			assigned_button[i] = Gamepad::GP_NONE;

		if (gp.is_connected())
		{
			auto& mappings = gp.get_mappings();

			for (auto& pair : mappings)
			{
				for (size_t i = 1; i < Setting::SETTING_NUM; i++)
				{
					if (pair.second == SETTING_ACTIONS[i])
					{
						assigned_button[i] = pair.first;
						break;
					}
				}
			}
		}
	}

	void UIJoypad::refresh_texts()
	{
		auto& gp = Gamepad::get();

		if (gp.is_connected())
			key_text[NAME].change_text(gp.get_name());
		else
			key_text[NAME].change_text("No gamepad");

		for (size_t i = 1; i < Setting::SETTING_NUM; i++)
		{
			if (assigned_button[i] == Gamepad::GP_NONE)
				key_text[i].change_text("None");
			else
				key_text[i].change_text(Gamepad::get_button_name(assigned_button[i]));
		}
	}

	void UIJoypad::draw(float inter) const
	{
		backgrnd[alternative_settings].draw(position);

		int16_t x = 79;
		int16_t y = 24;
		int16_t y_adj = 18;

		for (size_t i = 0; i < Setting::SETTING_NUM; i++)
		{
			if (i == 0)
				key_text[i].draw(position + Point<int16_t>(x, y));
			else if (i > 0 && i < 4)
				key_text[i].draw(position + Point<int16_t>(x - 16, y + 44 + y_adj * (i - 1)));
			else
				key_text[i].draw(position + Point<int16_t>(x - 16, y + 123 + y_adj * (i - 4)));
		}

		UIElement::draw(inter);
	}

	void UIJoypad::update()
	{
		UIElement::update();

		if (waiting_for_input && selected_row >= 1)
		{
			auto& gp = Gamepad::get();
			int32_t btn = gp.get_last_pressed();

			if (btn != Gamepad::GP_NONE)
			{
				gp.clear_last_pressed();

				// Remove this button from any other slot
				for (size_t i = 1; i < Setting::SETTING_NUM; i++)
				{
					if (assigned_button[i] == btn)
						assigned_button[i] = Gamepad::GP_NONE;
				}

				assigned_button[selected_row] = btn;
				waiting_for_input = false;
				selected_row = -1;
				refresh_texts();
			}
		}
	}

	Cursor::State UIJoypad::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked)
		{
			Point<int16_t> cursoroffset = cursorpos - position;
			int16_t row = row_by_position(cursoroffset.y());

			if (row >= 1 && row < Setting::SETTING_NUM)
			{
				if (selected_row == row && waiting_for_input)
				{
					selected_row = -1;
					waiting_for_input = false;
				}
				else
				{
					selected_row = row;
					waiting_for_input = true;
					Gamepad::get().clear_last_pressed();
					key_text[row].change_text("...");
				}
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	int16_t UIJoypad::row_by_position(int16_t y) const
	{
		int16_t base_y = 24;
		int16_t y_adj = 18;

		// Rows 1-3 (ATTACK, JUMP, PICKUP)
		for (int16_t i = 1; i < 4; i++)
		{
			int16_t ry = base_y + 44 + y_adj * (i - 1);

			if (y >= ry && y < ry + y_adj)
				return i;
		}

		// Rows 4-11 (HOTKEY0-7)
		for (int16_t i = 4; i < Setting::SETTING_NUM; i++)
		{
			int16_t ry = base_y + 123 + y_adj * (i - 4);

			if (y >= ry && y < ry + y_adj)
				return i;
		}

		return -1;
	}

	void UIJoypad::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				if (waiting_for_input)
				{
					waiting_for_input = false;
					selected_row = -1;
					refresh_texts();
				}
				else
				{
					cancel();
				}
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				save();
			}
		}
	}

	UIElement::Type UIJoypad::get_type() const
	{
		return TYPE;
	}

	Button::State UIJoypad::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::DEFAULT:
			Gamepad::get().reset_defaults();
			load_from_gamepad();
			refresh_texts();
			selected_row = -1;
			waiting_for_input = false;
			break;
		case Buttons::CANCEL:
			cancel();
			break;
		case Buttons::OK:
			save();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIJoypad::cancel()
	{
		load_from_gamepad();
		deactivate();
	}

	void UIJoypad::save()
	{
		auto& gp = Gamepad::get();
		auto& mappings = gp.get_mappings();

		// Remove existing mappings for our settings' actions
		for (size_t i = 1; i < Setting::SETTING_NUM; i++)
		{
			KeyAction::Id action = SETTING_ACTIONS[i];
			auto it = mappings.begin();

			while (it != mappings.end())
			{
				if (it->second == action)
					it = mappings.erase(it);
				else
					++it;
			}
		}

		// Apply our assignments
		for (size_t i = 1; i < Setting::SETTING_NUM; i++)
		{
			if (assigned_button[i] != Gamepad::GP_NONE)
				gp.set_mapping(static_cast<Gamepad::Button>(assigned_button[i]), SETTING_ACTIONS[i]);
		}

		deactivate();
	}
}
