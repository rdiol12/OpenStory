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

#include "KeyAction.h"

#include "../Template/Singleton.h"

#include <map>
#include <string>
#include <cstdint>

#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>

namespace ms
{
	class Gamepad : public Singleton<Gamepad>
	{
	public:
		// GLFW gamepad button IDs
		enum Button : int32_t
		{
			GP_A             = GLFW_GAMEPAD_BUTTON_A,
			GP_B             = GLFW_GAMEPAD_BUTTON_B,
			GP_X             = GLFW_GAMEPAD_BUTTON_X,
			GP_Y             = GLFW_GAMEPAD_BUTTON_Y,
			GP_LEFT_BUMPER   = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
			GP_RIGHT_BUMPER  = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
			GP_BACK          = GLFW_GAMEPAD_BUTTON_BACK,
			GP_START         = GLFW_GAMEPAD_BUTTON_START,
			GP_LEFT_THUMB    = GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
			GP_RIGHT_THUMB   = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
			GP_DPAD_UP       = GLFW_GAMEPAD_BUTTON_DPAD_UP,
			GP_DPAD_RIGHT    = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
			GP_DPAD_DOWN     = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
			GP_DPAD_LEFT     = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
			GP_NONE          = -1
		};

		Gamepad();

		void poll();
		bool is_connected() const;
		std::string get_name() const;

		void set_mapping(Button btn, KeyAction::Id action);
		KeyAction::Id get_mapping(Button btn) const;
		void reset_defaults();

		std::map<Button, KeyAction::Id>& get_mappings();

		int32_t get_last_pressed() const;
		void clear_last_pressed();

		static std::string get_button_name(int32_t btn);

	private:
		bool connected;
		int joystick_id;
		std::string name;
		std::map<Button, KeyAction::Id> button_map;
		bool prev_state[15];
		int32_t last_pressed;

		// Axis-to-dpad thresholds
		static constexpr float AXIS_THRESHOLD = 0.5f;
		bool prev_axis_left;
		bool prev_axis_right;
		bool prev_axis_up;
		bool prev_axis_down;
	};
}
