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
#include "Gamepad.h"

#include "UI.h"
#include <algorithm>

namespace ms
{
	Gamepad::Gamepad()
	{
		connected = false;
		joystick_id = GLFW_JOYSTICK_1;
		last_pressed = GP_NONE;

		for (int i = 0; i < 15; i++)
			prev_state[i] = false;

		prev_axis_left = false;
		prev_axis_right = false;
		prev_axis_up = false;
		prev_axis_down = false;

		reset_defaults();
	}

	void Gamepad::reset_defaults()
	{
		button_map.clear();

		// D-pad for movement
		button_map[GP_DPAD_LEFT]     = KeyAction::Id::LEFT;
		button_map[GP_DPAD_RIGHT]    = KeyAction::Id::RIGHT;
		button_map[GP_DPAD_UP]       = KeyAction::Id::UP;
		button_map[GP_DPAD_DOWN]     = KeyAction::Id::DOWN;

		// Face buttons
		button_map[GP_A]             = KeyAction::Id::JUMP;
		button_map[GP_B]             = KeyAction::Id::ATTACK;
		button_map[GP_X]             = KeyAction::Id::PICKUP;
		button_map[GP_Y]             = KeyAction::Id::ITEMS;

		// Bumpers/triggers as hotkeys (mapped to skills/items slots)
		button_map[GP_LEFT_BUMPER]   = KeyAction::Id::SKILLS;
		button_map[GP_RIGHT_BUMPER]  = KeyAction::Id::STATS;

		// System
		button_map[GP_START]         = KeyAction::Id::MAINMENU;
		button_map[GP_BACK]          = KeyAction::Id::ESCAPE;

		// Thumbsticks
		button_map[GP_LEFT_THUMB]    = KeyAction::Id::MINIMAP;
		button_map[GP_RIGHT_THUMB]   = KeyAction::Id::SIT;
	}

	void Gamepad::poll()
	{
		const bool present = glfwJoystickPresent(joystick_id) == GLFW_TRUE;
		connected = present;
		const char* label = present ? glfwGetJoystickName(joystick_id) : nullptr;
		name = label ? label : "";
		GLFWgamepadstate state{};
		if (present && !(glfwJoystickIsGamepad(joystick_id) && glfwGetGamepadState(joystick_id, &state))) {
			int count = 0;
			if (auto* raw = glfwGetJoystickButtons(joystick_id, &count))
				for (int i = 0; i < std::min(count, 15); ++i) state.buttons[i] = raw[i];
			if (auto* raw = glfwGetJoystickAxes(joystick_id, &count))
				for (int i = 0; i < std::min(count, 2); ++i) state.axes[i] = raw[i];
		}
		auto key_for = [&](KeyAction::Id action) {
			switch (action) {
			case KeyAction::LEFT: return GLFW_KEY_LEFT;
			case KeyAction::RIGHT: return GLFW_KEY_RIGHT;
			case KeyAction::UP: return GLFW_KEY_UP;
			case KeyAction::DOWN: return GLFW_KEY_DOWN;
			case KeyAction::ESCAPE: return GLFW_KEY_ESCAPE;
			default: break;
			}
			// The wire key-map uses Maple indices, not GLFW key codes.
			for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
				const auto mapping = UI::get().get_keyboard().get_mapping(key);
				if (mapping.action == action && (mapping.type == KeyType::ACTION || mapping.type == KeyType::MENU))
					return key;
			}
			return GLFW_KEY_UNKNOWN;
		};
		std::set<int> next;
		for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
			const bool down = state.buttons[i] == GLFW_PRESS;
			if (down && !prev_state[i]) last_pressed = i;
			prev_state[i] = down;
			const auto it = button_map.find(static_cast<Button>(i));
			if (down && it != button_map.end()) {
				const int key = key_for(it->second);
				if (key != GLFW_KEY_UNKNOWN) next.insert(key);
			}
		}
		if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -AXIS_THRESHOLD) next.insert(GLFW_KEY_LEFT);
		if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > AXIS_THRESHOLD) next.insert(GLFW_KEY_RIGHT);
		if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -AXIS_THRESHOLD) next.insert(GLFW_KEY_UP);
		if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > AXIS_THRESHOLD) next.insert(GLFW_KEY_DOWN);
		// Union all sources before releasing a key: stick and D-pad may overlap.
		for (int key : held_keys) if (!next.count(key)) UI::get().send_key(key, false);
		for (int key : next) if (!held_keys.count(key)) UI::get().send_key(key, true);
		held_keys = std::move(next);
	}

	bool Gamepad::is_connected() const
	{
		return connected;
	}

	std::string Gamepad::get_name() const
	{
		return name;
	}

	void Gamepad::set_mapping(Button btn, KeyAction::Id action)
	{
		button_map[btn] = action;
	}

	KeyAction::Id Gamepad::get_mapping(Button btn) const
	{
		auto it = button_map.find(btn);

		if (it != button_map.end())
			return it->second;

		return KeyAction::Id::LENGTH;
	}

	std::map<Gamepad::Button, KeyAction::Id>& Gamepad::get_mappings()
	{
		return button_map;
	}

	int32_t Gamepad::get_last_pressed() const
	{
		return last_pressed;
	}

	void Gamepad::clear_last_pressed()
	{
		last_pressed = GP_NONE;
	}

	std::string Gamepad::get_button_name(int32_t btn)
	{
		switch (btn)
		{
		case GP_A:             return "A";
		case GP_B:             return "B";
		case GP_X:             return "X";
		case GP_Y:             return "Y";
		case GP_LEFT_BUMPER:   return "LB";
		case GP_RIGHT_BUMPER:  return "RB";
		case GP_BACK:          return "Back";
		case GP_START:         return "Start";
		case GP_LEFT_THUMB:    return "LS";
		case GP_RIGHT_THUMB:   return "RS";
		case GP_DPAD_UP:       return "D-Up";
		case GP_DPAD_RIGHT:    return "D-Right";
		case GP_DPAD_DOWN:     return "D-Down";
		case GP_DPAD_LEFT:     return "D-Left";
		default:               return "Btn " + std::to_string(btn);
		}
	}
}
