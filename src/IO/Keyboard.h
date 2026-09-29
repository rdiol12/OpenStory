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
#include "KeyType.h"

#include <array>
#include <map>

namespace ms
{
	// Runtime switch for the "[KEYPROBE]" input traces (Keyboard::get_mapping
	// and the KEYMAP packet handler in Net/Handlers/PlayerHandlers.cpp).
	// They are off unless OPENSTORY_KEYDEBUG is set in the environment to
	// anything other than "0" or the empty string, e.g.
	//
	//     OPENSTORY_KEYDEBUG=1 ./OpenStory
	//
	// The variable is read once, on first use, because get_mapping() runs on
	// every key event and getenv() per keypress would be wasteful.
	bool key_debug_enabled();

	class Keyboard
	{
	public:
		struct Mapping
		{
			KeyType::Id type;
			int32_t action;

			Mapping() : type(KeyType::Id::NONE), action(0) {}
			Mapping(KeyType::Id in_type, int32_t in_action) : type(in_type), action(in_action) {}

			bool operator==(const Mapping& other) const
			{
				return type == other.type && action == other.action;
			}

			bool operator!=(const Mapping& other) const
			{
				return type != other.type || action != other.action;
			}
		};

		Keyboard();

		// Number of keys on the quickslot bar.
		static constexpr size_t NUM_QUICKSLOT_KEYS = 8;

		void assign(uint8_t key, uint8_t type, int32_t action);
		void remove(uint8_t key);

		// Drop every assignable binding, keeping only the keys the client owns
		// itself (arrows, Enter, Tab). The server's KEYMAP packet carries the
		// character's complete layout, so it calls this before applying it and
		// the built-in defaults never leak into a server-supplied layout.
		void clear_bindings();

		// Store the quickslot bar's key layout (eight maple keycodes),
		// e.g. from the server's QUICKSLOT_INIT packet.
		void set_quickslot_keys(const std::array<uint8_t, NUM_QUICKSLOT_KEYS>& keys);
		// Return the quickslot bar's key layout.
		const std::array<uint8_t, NUM_QUICKSLOT_KEYS>& get_quickslot_keys() const;

		int32_t leftshiftcode() const;
		int32_t rightshiftcode() const;
		int32_t capslockcode() const;
		int32_t leftctrlcode() const;
		int32_t rightctrlcode() const;
		std::map<int32_t, Mapping> get_maplekeys() const;
		KeyAction::Id get_ctrl_action(int32_t keycode) const;
		Mapping get_mapping(int32_t keycode) const;
		Mapping get_maple_mapping(int32_t keycode) const;
		Mapping get_text_mapping(int32_t keycode, bool shift) const;

	private:
		// Seed the keys the client handles on its own, independently of any
		// server key layout: the arrows, Enter and Tab.
		void init_client_keys();
		// Seed the stock v83 action bindings (attack, jump, pick up, sit) so a
		// character is playable even when no KEYMAP packet arrives.
		void init_default_bindings();
		// Install a mapping on every GLFW keycode that should answer for a
		// maple keycode -- the paired modifier half, and the Command keys on
		// macOS. See the comment above paired_maple_key in Keyboard.cpp.
		void apply_to_aliases(uint8_t key, const Mapping& mapping);

		std::map<int32_t, Mapping> keymap;
		std::map<int32_t, Mapping> maplekeys;
		std::map<int32_t, KeyAction::Id> textactions;
		std::map<int32_t, bool> keystate;
		std::array<uint8_t, NUM_QUICKSLOT_KEYS> quickslotkeys;
	};
}