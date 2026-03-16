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

#include <cstdint>

namespace ms
{
	namespace KeyAction
	{
		// v83 Maple-specific keycodes, sent via the Keymap Packet.
		enum Id : int32_t
		{
			// Menu actions (type 4)
			EQUIPMENT = 0,
			ITEMS = 1,
			STATS = 2,
			SKILLS = 3,
			FRIENDS = 4,
			WORLDMAP = 5,
			MAPLECHAT = 6,
			MINIMAP = 7,
			QUESTLOG = 8,
			KEYBINDINGS = 9,
			SAY = 10,
			WHISPER = 11,
			PARTYCHAT = 12,
			FRIENDSCHAT = 13,
			MENU = 14,
			QUICKSLOTS = 15,
			TOGGLECHAT = 16,
			GUILD = 17,
			GUILDCHAT = 18,
			PARTY = 19,
			NOTIFIER = 20,
			CHARINFO = 21,
			MONSTERBOOK = 22,
			CASHSHOP = 23,
			ALLIANCECHAT = 24,
			CHANGECHANNEL = 25,
			MEDALS = 26,
			BOSSPARTY = 27,
			MAINMENU = 28,
			SCREENSHOT = 29,
			PROFESSION = 30,
			ITEMPOT = 31,
			EVENT = 32,
			SILENTCRUSADE = 33,
			MANAGELEGION = 34,
			BATTLEANALYSIS = 35,
			BITS = 36,
			GUIDE = 37,
			ENHANCEEQUIP = 38,
			MONSTERCOLLECTION = 39,
			SOULWEAPON = 40,
			MAPLENEWS = 41,
			// Game actions (type 5)
			PICKUP = 50,
			SIT = 51,
			ATTACK = 52,
			JUMP = 53,
			INTERACT_HARVEST = 54,
			// Face expressions (type 6)
			FACE1 = 100,
			FACE2 = 101,
			FACE3 = 102,
			FACE4 = 103,
			FACE5 = 104,
			FACE6 = 105,
			FACE7 = 106,
			// Client-only actions (not sent to server)
			LEFT = 200,
			RIGHT = 201,
			UP = 202,
			DOWN = 203,
			BACK = 204,
			TAB = 205,
			RETURN = 206,
			ESCAPE = 207,
			SPACE = 208,
			DELETE = 209,
			HOME = 210,
			END = 211,
			COPY = 212,
			PASTE = 213,
			LENGTH
		};

		inline Id actionbyid(int32_t id)
		{
			return static_cast<Id>(id);
		}
	}
}
