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
#include "Handlers/ForeignCharHandlers.h"
#include "Handlers/ShopStorageHandlers.h"
#include "Handlers/MTSHandlers.h"

#include "../Configuration.h"

#include <iostream>

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
		QUEST_CLEAR = 49,          // 0x31
		SUE_CHARACTER_RESULT = 55, // 0x37

		SET_GENDER = 58,        // 0x3A

		/// Skills / Items
		SKILL_LEARN_ITEM_RESULT = 51, // 0x33

		/// Monster Book
		MONSTER_BOOK_SET_CARD = 215, // 0xD7
		MONSTER_BOOK_SET_COVER = 216, // 0xD8

		/// Player Interaction
		CHAR_INFO = 61,         // 0x3D

		/// Social
		PARTY_OPERATION = 62,   // 0x3E
		BUDDY_LIST = 63,        // 0x3F
		GUILD_OPERATION = 65,   // 0x41
		GUILD_BBS = 67,         // 0x43

		/// Messaging
		SERVER_MESSAGE = 68,    // 0x44
		WEEK_EVENT_MESSAGE = 77,// 0x4D
		MULTICHAT = 134,        // 0x86
		WHISPER = 135,          // 0x87

		/// Family
		FAMILY = 95,            // 0x5F
		FAMILY_PRIVILEGE_LIST = 100, // 0x64

		SET_BACK_EFFECT = 128,  // 0x80
		BLOCKED_MAP = 131,      // 0x83
		BLOCKED_SERVER = 132,   // 0x84
		FORCED_MAP_EQUIP = 133, // 0x85
		SPOUSE_CHAT = 136,      // 0x88
		ALLIANCE_OPERATION = 66, // 0x42
		MARRIAGE_REQUEST = 72,  // 0x48
		MARRIAGE_RESULT = 73,   // 0x49
		WEDDING_GIFT_RESULT = 74, // 0x4A
		NOTIFY_MARRIED_PARTNER = 75, // 0x4B
		FAMILY_CHART_RESULT = 94, // 0x5E
		FAMILY_RESULT = 96,     // 0x60
		FAMILY_JOIN_REQUEST = 97, // 0x61
		FAMILY_JOIN_REQUEST_RESULT = 98, // 0x62
		FAMILY_JOIN_ACCEPTED = 99, // 0x63
		FAMILY_REP_GAIN = 101,  // 0x65
		FAMILY_NOTIFY_LOGIN = 102, // 0x66
		FAMILY_SUMMON_REQUEST = 104, // 0x68
		NOTIFY_LEVELUP = 105,   // 0x69
		NOTIFY_JOB_CHANGE = 107, // 0x6B
		FAKE_GM_NOTICE = 116,       // 0x74
		SET_AVATAR_MEGAPHONE = 111, // 0x6F
		CLEAR_AVATAR_MEGAPHONE = 112, // 0x70
		NEW_YEAR_CARD = 118,    // 0x76
		SET_EXTRA_PENDANT_SLOT = 121, // 0x79

		/// Map / Field
		SCRIPT_PROGRESS_MESSAGE = 122, // 0x7A
		SKILL_MACROS = 124,     // 0x7C
		SET_FIELD = 125,        // 0x7D
		SET_ITC = 126,          // 0x7E — MTS/CS transition (ignored)
		SET_CASH_SHOP = 127,    // 0x7F
		FIELD_EFFECT = 138,     // 0x8A
		SET_TRACTION = 141, // 0x8D
		FIELD_OBSTACLE_ONOFF = 140, // 0x8C
		CONTI_MOVE = 148,       // 0x94
		CONTI_STATE = 149,      // 0x95
		ARIANT_ARENA_SHOW_RESULT = 155, // 0x9B
		PYRAMID_GAUGE = 157,    // 0x9D
		PYRAMID_SCORE = 158,    // 0x9E
		STOP_CLOCK = 154,       // 0x9A
		ADMIN_RESULT = 144,     // 0x90
		BLOW_WEATHER = 142,     // 0x8E
		OX_QUIZ = 145,          // 0x91
		CLOCK = 147,            // 0x93
		MAKER_RESULT = 217,     // 0xD9
		OPEN_UI = 220,          // 0xDC
		LOCK_UI = 221,          // 0xDD
		DISABLE_UI = 222,       // 0xDE
		SPAWN_GUIDE = 223,      // 0xDF
		TALK_GUIDE = 224,       // 0xE0
		SHOW_COMBO = 225,       // 0xE1
		QUICKSLOT_INIT = 159,   // 0x9F

		/// MapObject — Characters
		SPAWN_CHAR = 160,       // 0xA0
		REMOVE_CHAR = 161,      // 0xA1
		CHAT_RECEIVED = 162,    // 0xA2
		CHALKBOARD = 164,       // 0xA4
		SCROLL_RESULT = 167,    // 0xA7
		SPAWN_PET = 168,        // 0xA8
		MOVE_PET = 170,         // 0xAA
		PET_CHAT = 171,         // 0xAB
		PET_NAMECHANGE = 172,   // 0xAC
		PET_EXCEPTION_LIST = 173, // 0xAD
		PET_COMMAND = 174,      // 0xAE
		SUMMON_SKILL = 180,     // 0xB4
		SPAWN_DRAGON = 181,     // 0xB5
		MOVE_DRAGON = 182,      // 0xB6
		REMOVE_DRAGON = 183,    // 0xB7
		CHAR_MOVED = 185,       // 0xB9

		/// Attack
		ATTACKED_CLOSE = 186,   // 0xBA
		SKILL_EFFECT = 190,     // 0xBE
		ATTACKED_RANGED = 187,  // 0xBB
		ATTACKED_MAGIC = 188,   // 0xBC

		/// Summons
		SPAWN_SUMMON = 175,     // 0xAF
		REMOVE_SUMMON = 176,    // 0xB0
		MOVE_SUMMON = 177,      // 0xB1
		SUMMON_ATTACK = 178,    // 0xB2
		DAMAGE_SUMMON = 179,    // 0xB3

		/// Foreign character visuals
		THROW_GRENADE = 204,    // 0xCC
		DOJO_WARP_UP = 207,     // 0xCF
		CANCEL_SKILL_EFFECT = 191, // 0xBF
		DAMAGE_PLAYER = 192,    // 0xC0
		FACIAL_EXPRESSION = 193,// 0xC1
		SHOW_ITEM_EFFECT = 194, // 0xC2
		SHOW_CHAIR = 196,       // 0xC4
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
		BROADCAST_MSG = 212,    // 0xD4
		PLAYER_HINT = 214,      // 0xD6

		/// Player
		ADD_COOLDOWN = 234,     // 0xEA

		/// MapObject — Mobs
		SPAWN_MOB = 236,        // 0xEC
		KILL_MOB = 237,         // 0xED
		SPAWN_MOB_C = 238,      // 0xEE
		MOB_MOVED = 239,        // 0xEF
		MOVE_MONSTER_RESPONSE = 240, // 0xF0
		APPLY_MONSTER_STATUS = 242, // 0xF2
		CANCEL_MONSTER_STATUS = 243, // 0xF3
		DAMAGE_MONSTER = 246,   // 0xF6
		SHOW_MOB_HP = 250,      // 0xFA
		CATCH_MONSTER = 251,    // 0xFB
		CATCH_MONSTER_WITH_ITEM = 252, // 0xFC

		/// MapObject — NPCs
		SPAWN_NPC = 257,        // 0x101
		REMOVE_NPC = 258,       // 0x102
		SPAWN_NPC_C = 259,      // 0x103
		NPC_ACTION = 260,       // 0x104
		SET_NPC_SCRIPTABLE = 263, // 0x107

		/// MapObject — Drops / Reactors
		DROP_LOOT = 268,        // 0x10C
		REMOVE_LOOT = 269,      // 0x10D
		CANNOT_SPAWN_KITE = 270, // 0x10E
		SPAWN_KITE = 271,       // 0x10F
		REMOVE_KITE = 272,      // 0x110
		SPAWN_HIRED_MERCHANT = 265, // 0x109
		DESTROY_HIRED_MERCHANT = 266, // 0x10A
		UPDATE_HIRED_MERCHANT = 267, // 0x10B
		SPAWN_MIST = 273,       // 0x111
		REMOVE_MIST = 274,      // 0x112
		SPAWN_DOOR = 275,       // 0x113
		REMOVE_DOOR = 276,      // 0x114
		HIT_REACTOR = 277,      // 0x115
		SPAWN_REACTOR = 279,    // 0x117
		REMOVE_REACTOR = 280,   // 0x118

		/// Events
		SNOWBALL_STATE = 281,   // 0x119
		HIT_SNOWBALL = 282,     // 0x11A
		SNOWBALL_MESSAGE = 283, // 0x11B
		COCONUT_HIT = 285,     // 0x11D
		COCONUT_SCORE = 286,   // 0x11E
		ARIANT_ARENA_USER_SCORE = 297, // 0x129

		/// Monster Carnival
		MONSTER_CARNIVAL_START = 289,  // 0x121
		MONSTER_CARNIVAL_CP = 290,     // 0x122
		MONSTER_CARNIVAL_PARTY_CP = 291, // 0x123
		MONSTER_CARNIVAL_SUMMON = 292,  // 0x124
		MONSTER_CARNIVAL_MESSAGE = 293, // 0x125
		MONSTER_CARNIVAL_DIED = 294,    // 0x126

		/// Tournament
		TOURNAMENT = 315,       // 0x13B
		TOURNAMENT_MATCH_TABLE = 316, // 0x13C
		TOURNAMENT_SET_PRIZE = 317, // 0x13D
		TOURNAMENT_UEW = 318,   // 0x13E

		/// Wedding
		WEDDING_PROGRESS = 320, // 0x140
		WEDDING_CEREMONY_END = 321, // 0x141

		/// NPC Services
		FREDRICK_MESSAGE = 310, // 0x136
		FREDRICK = 311,         // 0x137
		RPS_GAME = 312,         // 0x138
		MESSENGER_OP = 313,     // 0x139

		/// Storage
		STORAGE = 309,          // 0x135

		/// Parcel (Duey delivery)
		PARCEL = 322,           // 0x142

		/// Player Interaction (Trade, etc.)
		PLAYER_INTERACTION = 314, // 0x13A

		/// NPC Interaction
		NPC_DIALOGUE = 304,     // 0x130
		OPEN_NPC_SHOP = 305,    // 0x131
		CONFIRM_SHOP_TRANSACTION = 306, // 0x132

		/// Cash Shop
		QUERY_CASH_RESULT = 324, // 0x144
		CS_OPERATION = 325,     // 0x145
		CS_CHECK_NAME_CHANGE = 328, // 0x148
		CS_NAME_CHANGE_POSSIBLE = 329, // 0x149
		CS_TRANSFER_WORLD = 331, // 0x14B
		CS_CASH_GACHAPON_RESULT = 333, // 0x14D

		/// Keymap
		KEYMAP = 335,           // 0x14F
		AUTO_HP_POT = 336,      // 0x150
		AUTO_MP_POT = 337,      // 0x151

		/// MapleTV
		SEND_TV = 341,          // 0x155
		REMOVE_TV = 342,        // 0x156
		ENABLE_TV = 343,        // 0x157

		/// MTS (Maple Trading System)
		MTS_OPERATION = 348,    // 0x15C
		MTS_OPERATION2 = 347,   // 0x15B

		/// Item Enhancement
		VICIOUS_HAMMER = 354,   // 0x162
		VEGA_SCROLL = 358       // 0x166
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
		emplace<MOVE_PET, MovePetHandler>();
		emplace<PET_CHAT, PetChatHandler>();
		emplace<PET_COMMAND, PetCommandHandler>();
		emplace<CHALKBOARD, ChalkboardHandler>();
		emplace<CANCEL_SKILL_EFFECT, CancelSkillEffectHandler>();
		emplace<SUMMON_SKILL, SummonSkillHandler>();
		emplace<SPAWN_DRAGON, SpawnDragonHandler>();
		emplace<MOVE_DRAGON, MoveDragonHandler>();
		emplace<REMOVE_DRAGON, RemoveDragonHandler>();
		emplace<SPAWN_NPC, SpawnNpcHandler>();
		emplace<REMOVE_NPC, RemoveNpcHandler>();
		emplace<SPAWN_NPC_C, SpawnNpcControllerHandler>();
		emplace<SPAWN_MOB, SpawnMobHandler>();
		emplace<SPAWN_MOB_C, SpawnMobControllerHandler>();
		emplace<MOB_MOVED, MobMovedHandler>();
		emplace<SHOW_MOB_HP, ShowMobHpHandler>();
		emplace<MOVE_MONSTER_RESPONSE, MoveMonsterResponseHandler>();
		emplace<DAMAGE_MONSTER, DamageMonsterHandler>();
		emplace<APPLY_MONSTER_STATUS, ApplyMonsterStatusHandler>();
		emplace<CANCEL_MONSTER_STATUS, CancelMonsterStatusHandler>();
		emplace<KILL_MOB, KillMobHandler>();
		emplace<DROP_LOOT, DropLootHandler>();
		emplace<REMOVE_LOOT, RemoveLootHandler>();
		emplace<SPAWN_MIST, SpawnMistHandler>();
		emplace<REMOVE_MIST, RemoveMistHandler>();
		emplace<SPAWN_DOOR, SpawnDoorHandler>();
		emplace<REMOVE_DOOR, RemoveDoorHandler>();
		emplace<HIT_REACTOR, HitReactorHandler>();
		emplace<SPAWN_REACTOR, SpawnReactorHandler>();
		emplace<REMOVE_REACTOR, RemoveReactorHandler>();

		// Summon handlers
		emplace<SPAWN_SUMMON, SpawnSummonHandler>();
		emplace<REMOVE_SUMMON, RemoveSummonHandler>();
		emplace<MOVE_SUMMON, MoveSummonHandler>();
		emplace<SUMMON_ATTACK, SummonAttackHandler>();
		emplace<DAMAGE_SUMMON, DamageSummonHandler>();

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
		emplace<SKILL_LEARN_ITEM_RESULT, SkillLearnItemResultHandler>();
		emplace<MONSTER_BOOK_SET_CARD, MonsterBookCardHandler>();
		emplace<MONSTER_BOOK_SET_COVER, MonsterBookCoverHandler>();

		// Messaging handlers
		emplace<SHOW_STATUS_INFO, ShowStatusInfoHandler>();
		emplace<CHAT_RECEIVED, ChatReceivedHandler>();
		emplace<SCROLL_RESULT, ScrollResultHandler>();
		emplace<SERVER_MESSAGE, ServerMessageHandler>();
		emplace<FAKE_GM_NOTICE, FakeGMNoticeHandler>();
		emplace<WEEK_EVENT_MESSAGE, WeekEventMessageHandler>();
		emplace<SHOW_ITEM_GAIN_INCHAT, ShowItemGainInChatHandler>();
		emplace<WHISPER, WhisperHandler>();
		emplace<MULTICHAT, MultichatHandler>();

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
		emplace<QUEST_CLEAR, QuestClearHandler>();
		emplace<FAME_RESPONSE, FameResponseHandler>();
		emplace<BUDDY_LIST, BuddyListHandler>();
		emplace<FAMILY, FamilyHandler>();
		emplace<PARTY_OPERATION, PartyOperationHandler>();
		emplace<SCRIPT_PROGRESS_MESSAGE, ScriptProgressMessageHandler>();
		emplace<CLOCK, ClockHandler>();
		emplace<FORCED_STAT_SET, ForcedStatSetHandler>();
		emplace<NPC_ACTION, NpcActionHandler>();
		emplace<PLAYER_HINT, YellowTipHandler>();
		emplace<BROADCAST_MSG, BroadcastMsgHandler>();
		emplace<CATCH_MONSTER, CatchMonsterHandler>();
		emplace<QUICKSLOT_INIT, QuickSlotInitHandler>();
		emplace<LAST_CONNECTED_WORLD, LastConnectedWorldHandler>();
		emplace<CLAIM_STATUS_CHANGED, ClaimStatusChangedHandler>();
		emplace<SUE_CHARACTER_RESULT, SueCharacterResultHandler>();
		emplace<SET_GENDER, SetGenderHandler>();
		emplace<FAMILY_PRIVILEGE_LIST, FamilyPrivilegeListHandler>();
		emplace<ADMIN_RESULT, AdminResultHandler>();
		emplace<SKILL_EFFECT, SkillEffectHandler>();
		emplace<SET_NPC_SCRIPTABLE, SetNpcScriptableHandler>();
		emplace<AUTO_HP_POT, AutoHpPotHandler>();
		emplace<AUTO_MP_POT, AutoMpPotHandler>();
		emplace<FIELD_OBSTACLE_ONOFF, FieldObstacleOnOffHandler>();
		emplace<SET_TRACTION, SetTractionHandler>();
		emplace<OPEN_UI, OpenUIHandler>();
		emplace<LOCK_UI, LockUIHandler>();
		emplace<DISABLE_UI, DisableUIHandler>();
		emplace<SPAWN_GUIDE, SpawnGuideHandler>();
		emplace<TALK_GUIDE, TalkGuideHandler>();
		emplace<SHOW_COMBO, ShowComboHandler>();
		emplace<FORCED_MAP_EQUIP, ForcedMapEquipHandler>();
		emplace<SPOUSE_CHAT, SpouseChatHandler>();
		emplace<CONTI_MOVE, ContiMoveHandler>();
		emplace<CONTI_STATE, ContiStateHandler>();
		emplace<STOP_CLOCK, StopClockHandler>();
		emplace<MAKER_RESULT, MakerResultHandler>();
		emplace<ALLIANCE_OPERATION, AllianceOperationHandler>();
		emplace<MARRIAGE_REQUEST, MarriageRequestHandler>();
		emplace<MARRIAGE_RESULT, MarriageResultHandler>();
		emplace<WEDDING_GIFT_RESULT, WeddingGiftResultHandler>();
		emplace<NOTIFY_MARRIED_PARTNER, NotifyMarriedPartnerHandler>();
		emplace<FAMILY_CHART_RESULT, FamilyChartResultHandler>();
		emplace<FAMILY_RESULT, FamilyResultHandler>();
		emplace<FAMILY_JOIN_REQUEST, FamilyJoinRequestHandler>();
		emplace<FAMILY_JOIN_REQUEST_RESULT, FamilyJoinRequestResultHandler>();
		emplace<FAMILY_JOIN_ACCEPTED, FamilyJoinAcceptedHandler>();
		emplace<FAMILY_REP_GAIN, FamilyRepGainHandler>();
		emplace<FAMILY_NOTIFY_LOGIN, FamilyLoginLogoutHandler>();
		emplace<FAMILY_SUMMON_REQUEST, FamilySummonRequestHandler>();
		emplace<NOTIFY_LEVELUP, NotifyLevelUpHandler>();
		emplace<NOTIFY_JOB_CHANGE, NotifyJobChangeHandler>();
		emplace<SET_AVATAR_MEGAPHONE, SetAvatarMegaphoneHandler>();
		emplace<CLEAR_AVATAR_MEGAPHONE, ClearAvatarMegaphoneHandler>();
		emplace<NEW_YEAR_CARD, NewYearCardHandler>();
		emplace<SET_EXTRA_PENDANT_SLOT, SetExtraPendantSlotHandler>();
		emplace<SET_BACK_EFFECT, SetBackEffectHandler>();
		emplace<BLOCKED_MAP, BlockedMapHandler>();
		emplace<BLOCKED_SERVER, BlockedServerHandler>();
		emplace<OX_QUIZ, OXQuizHandler>();
		emplace<SPAWN_HIRED_MERCHANT, SpawnHiredMerchantHandler>();
		emplace<DESTROY_HIRED_MERCHANT, DestroyHiredMerchantHandler>();
		emplace<UPDATE_HIRED_MERCHANT, UpdateHiredMerchantHandler>();
		emplace<FREDRICK_MESSAGE, FredrickMessageHandler>();
		emplace<FREDRICK, FredrickHandler>();
		emplace<RPS_GAME, RPSGameHandler>();
		emplace<MESSENGER_OP, MessengerHandler>();
		emplace<PET_NAMECHANGE, PetNameChangeHandler>();
		emplace<PET_EXCEPTION_LIST, PetExceptionListHandler>();
		emplace<CATCH_MONSTER_WITH_ITEM, CatchMonsterWithItemHandler>();
		emplace<CANNOT_SPAWN_KITE, CannotSpawnKiteHandler>();
		emplace<SPAWN_KITE, SpawnKiteHandler>();
		emplace<REMOVE_KITE, RemoveKiteHandler>();
		emplace<THROW_GRENADE, ThrowGrenadeHandler>();
		emplace<DOJO_WARP_UP, DojoWarpUpHandler>();
		emplace<ARIANT_ARENA_SHOW_RESULT, AriantShowResultHandler>();
		emplace<PYRAMID_GAUGE, PyramidGaugeHandler>();
		emplace<PYRAMID_SCORE, PyramidScoreHandler>();

		// Event handlers
		emplace<SNOWBALL_STATE, SnowballStateHandler>();
		emplace<HIT_SNOWBALL, HitSnowballHandler>();
		emplace<SNOWBALL_MESSAGE, SnowballMessageHandler>();
		emplace<COCONUT_HIT, CoconutHitHandler>();
		emplace<COCONUT_SCORE, CoconutScoreHandler>();
		emplace<ARIANT_ARENA_USER_SCORE, AriantScoreHandler>();

		// Tournament
		emplace<TOURNAMENT, TournamentHandler>();
		emplace<TOURNAMENT_MATCH_TABLE, TournamentMatchTableHandler>();
		emplace<TOURNAMENT_SET_PRIZE, TournamentSetPrizeHandler>();
		emplace<TOURNAMENT_UEW, TournamentUEWHandler>();

		// Wedding
		emplace<WEDDING_PROGRESS, WeddingProgressHandler>();
		emplace<WEDDING_CEREMONY_END, WeddingCeremonyEndHandler>();

		// MapleTV
		emplace<SEND_TV, SendTVHandler>();
		emplace<REMOVE_TV, RemoveTVHandler>();
		emplace<ENABLE_TV, EnableTVHandler>();

		// Item Enhancement
		emplace<VICIOUS_HAMMER, ViciousHammerHandler>();
		emplace<VEGA_SCROLL, VegaScrollHandler>();

		// Cash Shop extensions
		emplace<CS_CHECK_NAME_CHANGE, CashShopNameChangeHandler>();
		emplace<CS_NAME_CHANGE_POSSIBLE, CashShopNameChangePossibleHandler>();
		emplace<CS_TRANSFER_WORLD, CashShopTransferWorldHandler>();
		emplace<CS_CASH_GACHAPON_RESULT, CashGachaponResultHandler>();

		// Monster Carnival
		emplace<MONSTER_CARNIVAL_START, MonsterCarnivalStartHandler>();
		emplace<MONSTER_CARNIVAL_CP, MonsterCarnivalCPHandler>();
		emplace<MONSTER_CARNIVAL_PARTY_CP, MonsterCarnivalPartyCPHandler>();
		emplace<MONSTER_CARNIVAL_SUMMON, MonsterCarnivalSummonHandler>();
		emplace<MONSTER_CARNIVAL_MESSAGE, MonsterCarnivalMessageHandler>();
		emplace<MONSTER_CARNIVAL_DIED, MonsterCarnivalDiedHandler>();

		// Foreign character effect handlers
		emplace<DAMAGE_PLAYER, DamagePlayerHandler>();
		emplace<FACIAL_EXPRESSION, FacialExpressionHandler>();
		emplace<GIVE_FOREIGN_BUFF, GiveForeignBuffHandler>();
		emplace<CANCEL_FOREIGN_BUFF, CancelForeignBuffHandler>();
		emplace<UPDATE_PARTYMEMBER_HP, UpdatePartyMemberHPHandler>();
		emplace<GUILD_NAME_CHANGED, GuildNameChangedHandler>();
		emplace<GUILD_MARK_CHANGED, GuildMarkChangedHandler>();
		emplace<GUILD_OPERATION, GuildOperationHandler>();
		emplace<GUILD_BBS, GuildBBSHandler>();
		emplace<CANCEL_CHAIR, CancelChairHandler>();
		emplace<SHOW_CHAIR, ShowChairHandler>();
		emplace<SHOW_ITEM_EFFECT, ShowItemEffectHandler>();
		emplace<STORAGE, StorageHandler>();
		emplace<CONFIRM_SHOP_TRANSACTION, ConfirmShopTransactionHandler>();
		emplace<PARCEL, ParcelHandler>();
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