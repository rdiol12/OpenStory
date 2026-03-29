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

#include "../Template/Point.h"

#include <vector>

namespace ms
{
	// A packet to be sent to the server
	// Used as a base class to create specific packets
	class OutPacket
	{
	public:
		// Construct a packet by writing its opcode
		OutPacket(int16_t opcode);

		void dispatch();

		// Opcodes for OutPackets associated with version 83 of the game
		enum Opcode : uint16_t
		{
			/// Login
			LOGIN = 1,
			CHARLIST_REQUEST = 5,
			SERVERSTATUS_REQUEST = 6,
			ACCEPT_TOS = 7,
			SET_GENDER = 8,
			SERVERLIST_REQUEST = 11,
			SELECT_CHAR = 19,
			PLAYER_LOGIN = 20,
			NAME_CHAR = 21,
			CREATE_CHAR = 22,
			DELETE_CHAR = 23,
			PONG = 24,
			REGISTER_PIC = 29,
			SELECT_CHAR_PIC = 30,
			LOGIN_START = 35, // Custom name

			/// Monster Book
			MONSTER_BOOK_COVER = 57, // 0x39

			/// Gameplay 1
			CHANGEMAP = 38,
			CHANGE_CHANNEL = 39,
			ENTER_CASHSHOP = 40,
			MOVE_PLAYER = 41,
			CLOSE_ATTACK = 44,
			RANGED_ATTACK = 45,
			MAGIC_ATTACK = 46,
			TAKE_DAMAGE = 48,

			/// Messaging
			GENERAL_CHAT = 49,
			MULTI_CHAT = 119,

			/// NPC Interaction
			TALK_TO_NPC = 58,
			NPC_TALK_MORE = 60,
			NPC_SHOP_ACTION = 61,
			STORAGE_ACTION = 62,

			/// Player Interaction
			CHAR_INFO_REQUEST = 97,
			PLAYER_INTERACTION = 123,

			/// Chair
			CANCEL_CHAIR = 42,
			USE_CHAIR = 43,

			/// Inventory
			GATHER_ITEMS = 69,
			SORT_ITEMS = 70,
			MOVE_ITEM = 71,
			USE_ITEM = 72,
			SCROLL_EQUIP = 86,

			/// Player
			SPEND_AP = 87,
			HEAL_OVER_TIME = 89,
			SPEND_SP = 90,
			CHANGE_KEYMAP = 135,

			/// Skill
			USE_SKILL = 91,
			SKILL_MACRO_MODIFIED = 92,

			/// Gameplay 2
			PARTY_OPERATION = 124,
			DENY_PARTY_REQUEST = 125,
			DENY_GUILD_REQUEST = 127,
			ADMIN_COMMAND = 128,
			MOVE_MONSTER = 188,
			PICKUP_ITEM = 202,
			DAMAGE_REACTOR = 205,
			PLAYER_MAP_TRANSFER = 207,
			PLAYER_UPDATE = 223,

			/// Social
			GIVE_FAME = 95,
			QUEST_ACTION = 107,
			WHISPER = 120,
			BUDDYLIST_MODIFY = 130,

			/// Cash Shop
			BUY_CS_ITEM = 229,
			COUPON_CODE = 230,

			/// MTS (Maple Trading System)
			ENTER_MTS = 156,
			MTS_OPERATION = 253,

			/// Guild
			GUILD_OPERATION = 138,

			/// Alliance
			ALLIANCE_OPERATION = 143,
			DENY_ALLIANCE_REQUEST = 144,

			/// Messenger
			MESSENGER = 126,

			/// Family
			FAMILY_OPERATION = 364,

			/// RPS Game
			RPS_ACTION = 231,

			/// Report
			REPORT = 106,

			/// Party Search
			PARTY_SEARCH_REGISTER = 221,
			PARTY_SEARCH_START = 222,

			/// Event
			REQUEST_EVENT_INFO = 241, // 0xF1

			/// Wedding
			WEDDING_ACTION = 232,

			/// Bot Inventory (custom)
			BOT_INV_ACTION = 0x168
		};

	protected:
		// Skip a number of bytes (filled with zeros)
		void skip(size_t count);
		// Write a byte
		void write_byte(int8_t ch);
		// Write a short
		void write_short(int16_t sh);
		// Write an int
		void write_int(int32_t in);
		// Write a long
		void write_long(int64_t lg);

		// Write a point
		// One short for x and one for y
		void write_point(Point<int16_t> point);
		// Write a timestamp as an integer
		void write_time();
		// Write a string
		// Writes the length as a short and then each individual character as a byte
		void write_string(const std::string& str);
		// Write a random int
		void write_random();

		// Write the MACS and then write the HWID of the computer
		void write_hardware_info();
		// Function to convert hexadecimal to decimal
		int32_t hex_to_dec(std::string hexVal);

	private:
		std::vector<int8_t> bytes;
		int16_t opcode;
	};
}