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
// PLATFORM_MACOS / PLATFORM_IOS are decided here; this has to come before the
// checks below or every platform looks like a desktop GLFW build.
#include "../../platform/shared/PlatformConfig.h"

#include "Keyboard.h"

#include "KeyConfig.h"

#ifdef PLATFORM_IOS
#include "KeyCodes.h"
#else
#include <glfw3.h>
#endif

// Optional input instrumentation, gated by OPENSTORY_KEYDEBUG. See
// key_debug_enabled() below and the comment on its declaration in Keyboard.h.
#include <cstdlib>
#include <iostream>

namespace ms
{
	bool key_debug_enabled()
	{
		// Read once: get_mapping() is on the per-keypress path.
		static const bool enabled = []()
		{
			const char* flag = std::getenv("OPENSTORY_KEYDEBUG");

			// Anything except unset, empty and "0" turns the traces on.
			return flag != nullptr && flag[0] != '\0' && !(flag[0] == '0' && flag[1] == '\0');
		}();

		return enabled;
	}

	constexpr int32_t Keytable[90] =
	{
		0, 0, // 1
		GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_MINUS, GLFW_KEY_EQUAL,
		0, 0, // 15
		GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET,
		0, // 28
		GLFW_KEY_LEFT_CONTROL, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_BACKSLASH, GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_B, GLFW_KEY_N, GLFW_KEY_M, GLFW_KEY_COMMA, GLFW_KEY_PERIOD,
		0, 0, 0, // 55
		GLFW_KEY_LEFT_ALT, GLFW_KEY_SPACE,
		0, // 58
		GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12, GLFW_KEY_HOME,
		0, // 72
		GLFW_KEY_PAGE_UP,
		0, 0, 0, 0, 0, // 78
		GLFW_KEY_END,
		0, // 80
		GLFW_KEY_PAGE_DOWN, GLFW_KEY_INSERT, GLFW_KEY_DELETE, GLFW_KEY_ESCAPE, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_RIGHT_ALT, GLFW_KEY_SCROLL_LOCK
	};

	constexpr int32_t Shifttable[126] =
	{
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  10
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  20
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  30
		  0,  0, 49, 39, 51, 52, 53, 55,  0, 57, //  40
		 48, 56, 61,  0,  0,  0,  0,  0,  0,  0, //  50
		  0,  0,  0,  0,  0,  0,  0, 59,  0, 44, //  60
		  0, 46, 47, 50, 97, 98, 99,100,101,102, //  70
		103,104,105,106,107,108,109,110,111,112, //  80
		113,114,115,116,117,118,119,120,121,122, //  90
		  0,  0,  0, 54, 45,  0,  0,  0,  0,  0, // 100
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 110
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 120
		  0,  0, 91, 92, 93, 96					 // 126
	};

	constexpr int32_t Specialtable[96] =
	{
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 10
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 20
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 30
		  0,  0,  0,  0,  0,  0,  0,  0, 34,  0, // 40
		  0,  0,  0, 60, 95, 62, 63, 41, 33, 64, // 50
		 35, 36, 37, 94, 38, 42, 40,  0, 58,  0, // 60
		 43,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 70
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 80
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 90
		123,124,125,  0,  0,126					 // 96
	};

	// MapleStory's key config models Ctrl, Shift and Alt as a single key each --
	// UIKeyConfig binds and clears the left and right halves together -- but the
	// server's key layout only ever names one half (29 LCtrl, 42 LShift, 56 LAlt
	// in the default v83 layout). GLFW reports the two halves as different
	// keycodes, so a binding has to be installed on both or the right-hand key
	// silently does nothing.
	//
	// On macOS this is what makes the difference between jumping and not. A PC
	// keyboard has Alt immediately left of the space bar, which is where every
	// v83 player's thumb goes for JUMP. On Mac hardware that physical position
	// is the Command key, and GLFW reports it as GLFW_KEY_LEFT_SUPER/RIGHT_SUPER
	// -- a keycode that appears nowhere in Keytable, so it resolves to no
	// mapping at all. Option, one key further out, is the key GLFW reports as
	// GLFW_KEY_LEFT_ALT. Aliasing Command onto the Alt binding makes both the
	// key that is labelled Alt and the key that sits where Alt sits work.

	// The maple keycode of the other half of a paired modifier, or 0.
	static uint8_t paired_maple_key(uint8_t key)
	{
		switch (key)
		{
		case KeyConfig::Key::LEFT_CONTROL:	return KeyConfig::Key::RIGHT_CONTROL;
		case KeyConfig::Key::RIGHT_CONTROL:	return KeyConfig::Key::LEFT_CONTROL;
		case KeyConfig::Key::LEFT_SHIFT:	return KeyConfig::Key::RIGHT_SHIFT;
		case KeyConfig::Key::RIGHT_SHIFT:	return KeyConfig::Key::LEFT_SHIFT;
		case KeyConfig::Key::LEFT_ALT:		return KeyConfig::Key::RIGHT_ALT;
		case KeyConfig::Key::RIGHT_ALT:		return KeyConfig::Key::LEFT_ALT;
		default:							return 0;
		}
	}

	// Writes 'mapping' to every GLFW keycode that should answer for maple key
	// 'key': the key itself, the other half of the pair when the server left it
	// unbound, and on macOS the Command keys when 'key' is an Alt.
	void Keyboard::apply_to_aliases(uint8_t key, const Mapping& mapping)
	{
		// A maple keycode is a byte, but Keytable only defines 90 entries. The
		// KEYMAP handler happens to loop 0..89, but the quickslot layout does
		// not: MiscHandlers reads eight raw bytes off the wire into
		// set_quickslot_keys(), and UIStatusBar::clear_quickslot() feeds one of
		// them straight back here when the player clears a quickslot. A server
		// sending anything >= 90 in that packet reads past the end of the
		// table and binds whatever it finds there.
		if (key >= std::size(Keytable))
			return;

		keymap[Keytable[key]] = mapping;

		if (uint8_t pair = paired_maple_key(key))
		{
			// Only when the server has not bound that half itself -- its own
			// binding is authoritative and must not be overwritten here.
			if (!maplekeys.count(pair))
				keymap[Keytable[pair]] = mapping;
		}

#ifdef PLATFORM_MACOS
		if (key == KeyConfig::Key::LEFT_ALT || key == KeyConfig::Key::RIGHT_ALT)
		{
			// Nothing in Keytable ever produces these, so they are free to use.
			keymap[GLFW_KEY_LEFT_SUPER] = mapping;
			keymap[GLFW_KEY_RIGHT_SUPER] = mapping;
		}
#endif
	}

	Keyboard::Keyboard()
	{
		init_client_keys();
		init_default_bindings();

		textactions[GLFW_KEY_BACKSPACE] = KeyAction::Id::BACK;
		textactions[GLFW_KEY_ENTER] = KeyAction::Id::RETURN;
		textactions[GLFW_KEY_KP_ENTER] = KeyAction::Id::RETURN;
		textactions[GLFW_KEY_SPACE] = KeyAction::Id::SPACE;
		textactions[GLFW_KEY_TAB] = KeyAction::Id::TAB;
		textactions[GLFW_KEY_ESCAPE] = KeyAction::Id::ESCAPE;
		textactions[GLFW_KEY_HOME] = KeyAction::Id::HOME;
		textactions[GLFW_KEY_END] = KeyAction::Id::END;
		textactions[GLFW_KEY_DELETE] = KeyAction::Id::DELETE;

		// Default v83 quickslot key layout (maple keycodes), matching
		// Cosmic's QuickslotBinding::DEFAULT_QUICKSLOTS:
		// LShift, Insert, Home, PgUp / LCtrl, Delete, End, PgDn
		quickslotkeys = { 42, 82, 71, 73, 29, 83, 79, 81 };
	}

	void Keyboard::init_client_keys()
	{
		keymap[GLFW_KEY_LEFT] = Mapping(KeyType::Id::ACTION, KeyAction::Id::LEFT);
		keymap[GLFW_KEY_RIGHT] = Mapping(KeyType::Id::ACTION, KeyAction::Id::RIGHT);
		keymap[GLFW_KEY_UP] = Mapping(KeyType::Id::ACTION, KeyAction::Id::UP);
		keymap[GLFW_KEY_DOWN] = Mapping(KeyType::Id::ACTION, KeyAction::Id::DOWN);
		keymap[GLFW_KEY_ENTER] = Mapping(KeyType::Id::ACTION, KeyAction::Id::RETURN);
		keymap[GLFW_KEY_KP_ENTER] = Mapping(KeyType::Id::ACTION, KeyAction::Id::RETURN);
		keymap[GLFW_KEY_TAB] = Mapping(KeyType::Id::ACTION, KeyAction::Id::TAB);
	}

	// Journey seeded only the arrow keys and left everything else to the
	// server's KEYMAP packet, so a character whose layout never arrives -- or
	// whose layout leaves a slot unbound -- has no jump and no attack at all.
	// v83 ships a default layout for exactly this case; these are its action
	// keys, and they match UIKeyConfig's own default table. A KEYMAP packet
	// clears them (see Keyboard::clear_bindings) before installing the
	// character's real layout, so a server-supplied layout still wins outright
	// and behaviour with a server that sends one is unchanged.
	void Keyboard::init_default_bindings()
	{
		assign(KeyConfig::Key::LEFT_CONTROL, KeyType::Id::ACTION, KeyAction::Id::ATTACK);
		assign(KeyConfig::Key::LEFT_ALT, KeyType::Id::ACTION, KeyAction::Id::JUMP);
		assign(KeyConfig::Key::RIGHT_ALT, KeyType::Id::ACTION, KeyAction::Id::JUMP);
		assign(KeyConfig::Key::Z, KeyType::Id::ACTION, KeyAction::Id::PICKUP);
		assign(KeyConfig::Key::X, KeyType::Id::ACTION, KeyAction::Id::SIT);
	}

	void Keyboard::clear_bindings()
	{
		keymap.clear();
		maplekeys.clear();

		init_client_keys();
	}

	void Keyboard::set_quickslot_keys(const std::array<uint8_t, NUM_QUICKSLOT_KEYS>& keys)
	{
		quickslotkeys = keys;
	}

	const std::array<uint8_t, Keyboard::NUM_QUICKSLOT_KEYS>& Keyboard::get_quickslot_keys() const
	{
		return quickslotkeys;
	}

	int32_t Keyboard::leftshiftcode() const
	{
		return GLFW_KEY_LEFT_SHIFT;
	}

	int32_t Keyboard::rightshiftcode() const
	{
		return GLFW_KEY_RIGHT_SHIFT;
	}

	int32_t Keyboard::capslockcode() const
	{
		return GLFW_KEY_CAPS_LOCK;
	}

	int32_t Keyboard::leftctrlcode() const
	{
		return GLFW_KEY_LEFT_CONTROL;
	}

	int32_t Keyboard::rightctrlcode() const
	{
		return GLFW_KEY_RIGHT_CONTROL;
	}

	std::map<int32_t, Keyboard::Mapping> Keyboard::get_maplekeys() const
	{
		return maplekeys;
	}

	KeyAction::Id Keyboard::get_ctrl_action(int32_t keycode) const
	{
		switch (keycode)
		{
		case GLFW_KEY_C:
			return KeyAction::Id::COPY;
		case GLFW_KEY_V:
			return KeyAction::Id::PASTE;
			/*case GLFW_KEY_A:
				return KeyAction::Id::SELECTALL;*/
		default:
			return KeyAction::Id::LENGTH;
		}
	}

	void Keyboard::assign(uint8_t key, uint8_t tid, int32_t action)
	{
		if (KeyType::Id type = KeyType::typebyid(tid))
		{
			Mapping mapping = Mapping(type, action);

			apply_to_aliases(key, mapping);

			// maplekeys stays exactly as the server sent it: it is what
			// UIKeyConfig displays and what gets saved back, so the aliases
			// above must not show up there as bindings nobody asked for.
			maplekeys[key] = mapping;
		}
	}

	void Keyboard::remove(uint8_t key)
	{
		Mapping mapping = Mapping(KeyType::Id::NONE, 0);

		apply_to_aliases(key, mapping);

		maplekeys[key] = mapping;
	}

	Keyboard::Mapping Keyboard::get_text_mapping(int32_t keycode, bool shift) const
	{
		if (textactions.count(keycode))
		{
			return Mapping(KeyType::Id::ACTION, textactions.at(keycode));
		}
		else if (keycode == 39 || (keycode >= 44 && keycode <= 57) || keycode == 59 || keycode == 61 || (keycode >= 91 && keycode <= 93) || keycode == 96)
		{
			if (!shift)
				return Mapping(KeyType::Id::TEXT, keycode);
			else
				return Mapping(KeyType::Id::TEXT, Specialtable[keycode - 1]);
		}
		else if (keycode >= 33 && keycode <= 126)
		{
			if (shift)
				return Mapping(KeyType::Id::TEXT, keycode);
			else
				return Mapping(KeyType::Id::TEXT, Shifttable[keycode - 1]);
		}
		else
		{
			switch (keycode)
			{
			case GLFW_KEY_LEFT:
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_UP:
			case GLFW_KEY_DOWN:
				return keymap.at(keycode);
			default:
				return Mapping(KeyType::Id::NONE, 0);
			}
		}
	}

	Keyboard::Mapping Keyboard::get_mapping(int32_t keycode) const
	{
		auto iter = keymap.find(keycode);

		if (iter == keymap.end())
		{
			// Raw GLFW keycode that resolved to no binding at all.
			if (key_debug_enabled())
				std::cout << "[KEYPROBE] glfw=" << keycode << " -> NO MAPPING" << std::endl;

			return Mapping(KeyType::Id::NONE, 0);
		}

		// Raw GLFW keycode -> type/action actually resolved.
		if (key_debug_enabled())
			std::cout << "[KEYPROBE] glfw=" << keycode
				<< " -> type=" << static_cast<int32_t>(iter->second.type)
				<< " action=" << iter->second.action
				<< (iter->second.action == KeyAction::Id::JUMP && iter->second.type == KeyType::Id::ACTION ? "  (JUMP)" : "")
				<< std::endl;

		return iter->second;
	}

	Keyboard::Mapping Keyboard::get_maple_mapping(int32_t keycode) const
	{
		auto iter = maplekeys.find(keycode);

		if (iter == maplekeys.end())
			return Mapping(KeyType::Id::NONE, 0);

		return iter->second;
	}
}