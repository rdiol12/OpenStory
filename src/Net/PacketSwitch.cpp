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
#include "PacketSwitch.h"

#include "Handlers/AttackHandlers.h"
#include "Handlers/CashShopHandlers.h"
#include "Handlers/CommonHandlers.h"
#include "Handlers/InventoryHandlers.h"
#include "Handlers/LoginHandlers.h"
#include "Handlers/MapObjectHandlers.h"
#include "Handlers/MessagingHandlers.h"
#include "Handlers/NpcInteractionHandlers.h"
#include "Handlers/PlayerHandlers.h"
#include "Handlers/PlayerInteractionHandlers.h"
#include "Handlers/SetFieldHandlers.h"
#include "Handlers/TestingHandlers.h"
#include "Handlers/MTSHandlers.h"

#include "../Configuration.h"

namespace ms
{
	// Opcodes for InPackets — aligned with Cosmic (P0nk/Cosmic) SendOpcodes
	enum Opcode : uint16_t
	{
		/// Login
		LOGIN_RESULT = 0,       // 0x00
		SERVERSTATUS = 3,       // 0x03
		SERVERLIST = 10,        // 0x0A
		CHARLIST = 11,          // 0x0B
		SERVER_IP = 12,         // 0x0C
		CHARNAME_RESPONSE = 13, // 0x0D
		ADD_NEWCHAR_ENTRY = 14, // 0x0E
		DELCHAR_RESPONSE = 15,  // 0x0F
		CHANGE_CHANNEL = 16,    // 0x10
		PING = 17,              // 0x11
		RELOG_RESPONSE = 22,    // 0x16
		LAST_CONNECTED_WORLD = 26, // 0x1A
		RECOMMENDED_WORLDS = 27,// 0x1B
		CHECK_SPW_RESULT = 28,  // 0x1C

		/// Inventory
		MODIFY_INVENTORY = 29,  // 0x1D
		GATHER_RESULT = 52,     // 0x34
		SORT_RESULT = 53,       // 0x35

		/// Player
		CHANGE_STATS = 31,      // 0x1F
		GIVE_BUFF = 32,         // 0x20
		CANCEL_BUFF = 33,       // 0x21
		FORCED_STAT_SET = 34,   // 0x22
		RECALCULATE_STATS = 35, // 0x23
		UPDATE_SKILL = 36,      // 0x24
		FAME_RESPONSE = 38,     // 0x26
		SHOW_STATUS_INFO = 39,  // 0x27
		CLAIM_STATUS_CHANGED = 47, // 0x2F

		SET_GENDER = 58,        // 0x3A

		/// Player Interaction
		CHAR_INFO = 61,         // 0x3D

		/// Social
		PARTY_OPERATION = 62,   // 0x3E
		BUDDY_LIST = 63,        // 0x3F

		/// Messaging
		SERVER_MESSAGE = 68,    // 0x44
		WEEK_EVENT_MESSAGE = 77,// 0x4D

		/// Family
		FAMILY = 95,            // 0x5F
		FAMILY_PRIVILEGE_LIST = 100, // 0x64

		/// Map / Field
		SCRIPT_PROGRESS_MESSAGE = 122, // 0x7A
		SKILL_MACROS = 124,     // 0x7C
		SET_FIELD = 125,        // 0x7D
		SET_ITC = 126,          // 0x7E — MTS/CS transition (ignored)
		SET_CASH_SHOP = 127,    // 0x7F
		FIELD_EFFECT = 138,     // 0x8A
		FIELD_OBSTACLE_ONOFF = 140, // 0x8C
		ADMIN_RESULT = 144,     // 0x90
		BLOW_WEATHER = 142,     // 0x8E
		CLOCK = 147,            // 0x93
		QUICKSLOT_INIT = 159,   // 0x9F

		/// MapObject — Characters
		SPAWN_CHAR = 160,       // 0xA0
		REMOVE_CHAR = 161,      // 0xA1
		CHAT_RECEIVED = 162,    // 0xA2
		SCROLL_RESULT = 167,    // 0xA7
		SPAWN_PET = 168,        // 0xA8
		CHAR_MOVED = 185,       // 0xB9

		/// Attack
		ATTACKED_CLOSE = 186,   // 0xBA
		SKILL_EFFECT = 190,     // 0xBE
		ATTACKED_RANGED = 187,  // 0xBB
		ATTACKED_MAGIC = 188,   // 0xBC

		/// Foreign character visuals
		DAMAGE_PLAYER = 192,    // 0xC0
		FACIAL_EXPRESSION = 193,// 0xC1
		SHOW_ITEM_EFFECT = 194, // 0xC2
		UPDATE_CHARLOOK = 197,  // 0xC5
		SHOW_FOREIGN_EFFECT = 198, // 0xC6
		GIVE_FOREIGN_BUFF = 199,   // 0xC7
		CANCEL_FOREIGN_BUFF = 200, // 0xC8
		UPDATE_PARTYMEMBER_HP = 201, // 0xC9
		GUILD_NAME_CHANGED = 202,  // 0xCA
		GUILD_MARK_CHANGED = 203,  // 0xCB
		CANCEL_CHAIR = 205,     // 0xCD
		SHOW_ITEM_GAIN_INCHAT = 206, // 0xCE

		/// Quest
		UPDATE_QUEST_INFO = 211, // 0xD3
		PLAYER_HINT = 214,      // 0xD6

		/// Player
		ADD_COOLDOWN = 234,     // 0xEA

		/// MapObject — Mobs
		SPAWN_MOB = 236,        // 0xEC
		KILL_MOB = 237,         // 0xED
		SPAWN_MOB_C = 238,      // 0xEE
		MOB_MOVED = 239,        // 0xEF
		SHOW_MOB_HP = 250,      // 0xFA
		CATCH_MONSTER = 251,    // 0xFB

		/// MapObject — NPCs
		SPAWN_NPC = 257,        // 0x101
		SPAWN_NPC_C = 259,      // 0x103
		NPC_ACTION = 260,       // 0x104
		SET_NPC_SCRIPTABLE = 263, // 0x107

		/// MapObject — Drops / Reactors
		DROP_LOOT = 268,        // 0x10C
		REMOVE_LOOT = 269,      // 0x10D
		HIT_REACTOR = 277,      // 0x115
		SPAWN_REACTOR = 279,    // 0x117
		REMOVE_REACTOR = 280,   // 0x118

		/// Storage
		STORAGE = 309,          // 0x135

		/// Player Interaction (Trade, etc.)
		PLAYER_INTERACTION = 314, // 0x13A

		/// NPC Interaction
		NPC_DIALOGUE = 304,     // 0x130
		OPEN_NPC_SHOP = 305,    // 0x131
		CONFIRM_SHOP_TRANSACTION = 306, // 0x132

		/// Cash Shop
		QUERY_CASH_RESULT = 324, // 0x144
		CS_OPERATION = 325,     // 0x145

		/// Keymap
		KEYMAP = 335,           // 0x14F
		AUTO_HP_POT = 336,      // 0x150
		AUTO_MP_POT = 337,      // 0x151

		/// MTS (Maple Trading System)
		MTS_OPERATION = 348,    // 0x15C
		MTS_OPERATION2 = 347    // 0x15B
	};

	PacketSwitch::PacketSwitch()
	{
		// Common handlers
		emplace<PING, PingHandler>();

		// Login handlers
		emplace<LOGIN_RESULT, LoginResultHandler>();
		emplace<SERVERSTATUS, ServerStatusHandler>();
		emplace<SERVERLIST, ServerlistHandler>();
		emplace<CHARLIST, CharlistHandler>();
		emplace<SERVER_IP, ServerIPHandler>();
		emplace<CHARNAME_RESPONSE, CharnameResponseHandler>();
		emplace<ADD_NEWCHAR_ENTRY, AddNewCharEntryHandler>();
		emplace<DELCHAR_RESPONSE, DeleteCharResponseHandler>();
		emplace<RECOMMENDED_WORLDS, RecommendedWorldsHandler>();

		// SetField handlers
		emplace<SET_FIELD, SetFieldHandler>();
		emplace<SET_ITC, SetITCHandler>();

		// MapObject handlers
		emplace<SPAWN_CHAR, SpawnCharHandler>();
		emplace<CHAR_MOVED, CharMovedHandler>();
		emplace<UPDATE_CHARLOOK, UpdateCharLookHandler>();
		emplace<SHOW_FOREIGN_EFFECT, ShowForeignEffectHandler>();
		emplace<REMOVE_CHAR, RemoveCharHandler>();
		emplace<SPAWN_PET, SpawnPetHandler>();
		emplace<SPAWN_NPC, SpawnNpcHandler>();
		emplace<SPAWN_NPC_C, SpawnNpcControllerHandler>();
		emplace<SPAWN_MOB, SpawnMobHandler>();
		emplace<SPAWN_MOB_C, SpawnMobControllerHandler>();
		emplace<MOB_MOVED, MobMovedHandler>();
		emplace<SHOW_MOB_HP, ShowMobHpHandler>();
		emplace<KILL_MOB, KillMobHandler>();
		emplace<DROP_LOOT, DropLootHandler>();
		emplace<REMOVE_LOOT, RemoveLootHandler>();
		emplace<HIT_REACTOR, HitReactorHandler>();
		emplace<SPAWN_REACTOR, SpawnReactorHandler>();
		emplace<REMOVE_REACTOR, RemoveReactorHandler>();

		// Attack handlers
		emplace<ATTACKED_CLOSE, CloseAttackHandler>();
		emplace<ATTACKED_RANGED, RangedAttackHandler>();
		emplace<ATTACKED_MAGIC, MagicAttackHandler>();

		// Player handlers
		emplace<CHANGE_CHANNEL, ChangeChannelHandler>();
		emplace<KEYMAP, KeymapHandler>();
		emplace<SKILL_MACROS, SkillMacrosHandler>();
		emplace<CHANGE_STATS, ChangeStatsHandler>();
		emplace<GIVE_BUFF, ApplyBuffHandler>();
		emplace<CANCEL_BUFF, CancelBuffHandler>();
		emplace<RECALCULATE_STATS, RecalculateStatsHandler>();
		emplace<UPDATE_SKILL, UpdateSkillHandler>();
		emplace<ADD_COOLDOWN, AddCooldownHandler>();

		// Messaging handlers
		emplace<SHOW_STATUS_INFO, ShowStatusInfoHandler>();
		emplace<CHAT_RECEIVED, ChatReceivedHandler>();
		emplace<SCROLL_RESULT, ScrollResultHandler>();
		emplace<SERVER_MESSAGE, ServerMessageHandler>();
		emplace<WEEK_EVENT_MESSAGE, WeekEventMessageHandler>();
		emplace<SHOW_ITEM_GAIN_INCHAT, ShowItemGainInChatHandler>();

		// Inventory Handlers
		emplace<MODIFY_INVENTORY, ModifyInventoryHandler>();
		emplace<GATHER_RESULT, GatherResultHandler>();
		emplace<SORT_RESULT, SortResultHandler>();

		// NPC Interaction Handlers
		emplace<NPC_DIALOGUE, NpcDialogueHandler>();
		emplace<OPEN_NPC_SHOP, OpenNpcShopHandler>();

		// Player Interaction
		emplace<CHAR_INFO, CharInfoHandler>();

		// Cash Shop
		emplace<SET_CASH_SHOP, SetCashShopHandler>();
		emplace<QUERY_CASH_RESULT, QueryCashResultHandler>();
		emplace<CS_OPERATION, CashShopOperationHandler>();

		// Additional v83 handlers
		emplace<CHECK_SPW_RESULT, CheckSpwResultHandler>();
		emplace<FIELD_EFFECT, FieldEffectHandler>();
		emplace<BLOW_WEATHER, BlowWeatherHandler>();

		// Stub handlers for unhandled v83 packets
		emplace<RELOG_RESPONSE, RelogResponseHandler>();
		emplace<UPDATE_QUEST_INFO, UpdateQuestInfoHandler>();
		emplace<FAME_RESPONSE, FameResponseHandler>();
		emplace<BUDDY_LIST, BuddyListHandler>();
		emplace<FAMILY, FamilyHandler>();
		emplace<PARTY_OPERATION, PartyOperationHandler>();
		emplace<SCRIPT_PROGRESS_MESSAGE, ScriptProgressMessageHandler>();
		emplace<CLOCK, ClockHandler>();
		emplace<FORCED_STAT_SET, ForcedStatSetHandler>();
		emplace<NPC_ACTION, NpcActionHandler>();
		emplace<PLAYER_HINT, YellowTipHandler>();
		emplace<CATCH_MONSTER, CatchMonsterHandler>();
		emplace<QUICKSLOT_INIT, QuickSlotInitHandler>();
		emplace<LAST_CONNECTED_WORLD, LastConnectedWorldHandler>();
		emplace<CLAIM_STATUS_CHANGED, ClaimStatusChangedHandler>();
		emplace<SET_GENDER, SetGenderHandler>();
		emplace<FAMILY_PRIVILEGE_LIST, FamilyPrivilegeListHandler>();
		emplace<ADMIN_RESULT, AdminResultHandler>();
		emplace<SKILL_EFFECT, SkillEffectHandler>();
		emplace<SET_NPC_SCRIPTABLE, SetNpcScriptableHandler>();
		emplace<AUTO_HP_POT, AutoHpPotHandler>();
		emplace<AUTO_MP_POT, AutoMpPotHandler>();
		emplace<FIELD_OBSTACLE_ONOFF, FieldObstacleOnOffHandler>();

		// Foreign character effect handlers
		emplace<DAMAGE_PLAYER, DamagePlayerHandler>();
		emplace<FACIAL_EXPRESSION, FacialExpressionHandler>();
		emplace<GIVE_FOREIGN_BUFF, GiveForeignBuffHandler>();
		emplace<CANCEL_FOREIGN_BUFF, CancelForeignBuffHandler>();
		emplace<UPDATE_PARTYMEMBER_HP, UpdatePartyMemberHPHandler>();
		emplace<GUILD_NAME_CHANGED, GuildNameChangedHandler>();
		emplace<GUILD_MARK_CHANGED, GuildMarkChangedHandler>();
		emplace<CANCEL_CHAIR, CancelChairHandler>();
		emplace<SHOW_ITEM_EFFECT, ShowItemEffectHandler>();
		emplace<STORAGE, StorageHandler>();
		emplace<PLAYER_INTERACTION, PlayerInteractionHandler>();

		// MTS (Maple Trading System)
		emplace<MTS_OPERATION, MTSOperationHandler>();
		emplace<MTS_OPERATION2, MTSCashHandler>();
	}

	void PacketSwitch::forward(const int8_t* bytes, size_t length) const
	{
		// Wrap the bytes with a parser
		InPacket recv = { bytes, length };

		// Read the opcode to determine handler responsible
		uint16_t opcode = recv.read_short();

		if (Configuration::get().get_show_packets())
		{
			if (opcode == PING)
				std::cout << "Received Packet: PING" << std::endl;
			else
				std::cout << "Received Packet: " << std::to_string(opcode) << std::endl;
		}

		if (opcode < NUM_HANDLERS)
		{
			if (auto& handler = handlers[opcode])
			{
				// Handler is good, packet is passed on

				try
				{
					handler->handle(recv);
				}
				catch (const PacketError& err)
				{
					// Notice about an error
					warn(err.what(), opcode);
				}
			}
			else
			{
				// Warn about an unhandled packet
				warn(MSG_UNHANDLED, opcode);
			}
		}
		else
		{
			// Warn about a packet with opcode out of bounds
			warn(MSG_OUTOFBOUNDS, opcode);
		}
	}

	void PacketSwitch::warn(const std::string& message, size_t opcode) const
	{
		std::cout << "Opcode [" << opcode << "] Error: " << message << std::endl;

		// Also write to log file for debugging
		static FILE* logfile = fopen("unhandled_packets.txt", "a");
		if (logfile)
		{
			fprintf(logfile, "Opcode [%zu]: %s\n", opcode, message.c_str());
			fflush(logfile);
		}
	}
}