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

#include "../PacketHandler.h"

namespace ms
{
	// Handles the server response for secondary password (PIC) verification
	class CheckSpwResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Handles map field effects such as screen animations and environmental effects
	class FieldEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Stub handlers for unhandled v83 packets — all log their data for debugging
	class RelogResponseHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class UpdateQuestInfoHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Quest completion visual notification (quest log marker)
	class QuestClearHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class FameResponseHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BuddyListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class FamilyHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class PartyOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Script progress message shown during NPC scripts/events
	class ScriptProgressMessageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class ClockHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class ForcedStatSetHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SetTractionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class NpcActionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class YellowTipHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class CatchMonsterHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BlowWeatherHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Damage shown on another player (mob hit, environment, etc.)
	class DamagePlayerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Facial expression / emote shown on another player
	class FacialExpressionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Foreign buff applied to another player on the map
	class GiveForeignBuffHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Foreign buff removed from another player on the map
	class CancelForeignBuffHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Party member HP update
	class UpdatePartyMemberHPHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild name changed for a player on the map
	class GuildNameChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild mark/emblem changed for a player on the map
	class GuildMarkChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Chair visual cancelled for a player
	class CancelChairHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Item effect shown on a player
	class ShowItemEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Quick slot key configuration
	class QuickSlotInitHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop cash query result
	class QueryCashResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Last connected world notification
	class LastConnectedWorldHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Claim/event status changed
	class ClaimStatusChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Set gender packet
	class SetGenderHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family privilege list
	class FamilyPrivilegeListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Admin/GM command result
	class AdminResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Skill effect shown on another player
	class SkillEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Mark NPCs as scriptable (quest NPCs etc)
	class SetNpcScriptableHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Auto HP/MP potion setting
	class AutoHpPotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class AutoMpPotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// NPC Storage/Warehouse UI
	class StorageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Field obstacle on/off list (gates, platforms in PQs)
	class FieldObstacleOnOffHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Player interaction: trade, miniroom, player shop
	class PlayerInteractionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild operation response (create, invite, expel, rank change, etc.)
	class GuildOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild BBS (bulletin board system) response
	class GuildBBSHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Show chair visual on a foreign character (opcode 0xC4)
	class ShowChairHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// NPC shop transaction confirmation (opcode 0x132)
	class ConfirmShopTransactionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Parcel/Duey delivery system (opcode 0x142)
	class ParcelHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Server requests client to open a specific UI window
	// Opcode: OPEN_UI(220)
	class OpenUIHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Lock the UI (boss fights, cutscenes)
	// Opcode: LOCK_UI(221)
	class LockUIHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Disable the UI (boss fights, cutscenes)
	// Opcode: DISABLE_UI(222)
	class DisableUIHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn the beginner guide NPC
	// Opcode: SPAWN_GUIDE(223)
	class SpawnGuideHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guide NPC dialogue
	// Opcode: TALK_GUIDE(224)
	class TalkGuideHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Show combo counter
	// Opcode: SHOW_COMBO(225)
	class ShowComboHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Force equip on map entry
	// Opcode: FORCED_MAP_EQUIP(133)
	class ForcedMapEquipHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spouse/partner chat message
	// Opcode: SPOUSE_CHAT(136)
	class SpouseChatHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Continental boat/ship movement
	// Opcode: CONTI_MOVE(148)
	class ContiMoveHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Continental boat/ship state
	// Opcode: CONTI_STATE(149)
	class ContiStateHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Item Maker crafting result
	// Opcode: MAKER_RESULT(217)
	class MakerResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival start
	// Opcode: MONSTER_CARNIVAL_START(289)
	class MonsterCarnivalStartHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival individual CP update
	// Opcode: MONSTER_CARNIVAL_OBTAINED_CP(290)
	class MonsterCarnivalCPHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival party CP update
	// Opcode: MONSTER_CARNIVAL_PARTY_CP(291)
	class MonsterCarnivalPartyCPHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival summon notification
	// Opcode: MONSTER_CARNIVAL_SUMMON(292)
	class MonsterCarnivalSummonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival message
	// Opcode: MONSTER_CARNIVAL_MESSAGE(293)
	class MonsterCarnivalMessageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Monster Carnival death notification
	// Opcode: MONSTER_CARNIVAL_DIED(294)
	class MonsterCarnivalDiedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a hired merchant box on the map
	// Opcode: SPAWN_HIRED_MERCHANT(265)
	class SpawnHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a hired merchant box from the map
	// Opcode: DESTROY_HIRED_MERCHANT(266)
	class DestroyHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Update a hired merchant box info
	// Opcode: UPDATE_HIRED_MERCHANT(267)
	class UpdateHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Fredrick message (merchant storage NPC)
	// Opcode: FREDRICK_MESSAGE(310)
	class FredrickMessageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Fredrick merchant storage retrieval
	// Opcode: FREDRICK(311)
	class FredrickHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Stop the on-screen clock/timer
	// Opcode: STOP_CLOCK(154)
	class StopClockHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Rock-Paper-Scissors game
	// Opcode: RPS_GAME(312)
	class RPSGameHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Maple Messenger chat window
	// Opcode: MESSENGER(313)
	class MessengerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Level up notification (guild/family broadcast)
	// Opcode: NOTIFY_LEVELUP(105)
	class NotifyLevelUpHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Job change notification (guild/family broadcast)
	// Opcode: NOTIFY_JOB_CHANGE(107)
	class NotifyJobChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild alliance operations
	// Opcode: ALLIANCE_OPERATION(66)
	class AllianceOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// OX Quiz event question
	// Opcode: OX_QUIZ(145)
	class OXQuizHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Snowball event state
	// Opcode: SNOWBALL_STATE(281)
	class SnowballStateHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Snowball hit
	// Opcode: HIT_SNOWBALL(282)
	class HitSnowballHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Snowball message
	// Opcode: SNOWBALL_MESSAGE(283)
	class SnowballMessageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Coconut event hit
	// Opcode: COCONUT_HIT(285)
	class CoconutHitHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Coconut event score
	// Opcode: COCONUT_SCORE(286)
	class CoconutScoreHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet name change notification
	// Opcode: PET_NAMECHANGE(172)
	class PetNameChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet auto-feed exception list
	// Opcode: PET_EXCEPTION_LIST(173)
	class PetExceptionListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding progress step
	class WeddingProgressHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding ceremony end
	class WeddingCeremonyEndHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Marriage request / wishlist
	class MarriageRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Marriage result
	class MarriageResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding gift result
	class WeddingGiftResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Partner map transfer notification
	class NotifyMarriedPartnerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family chart/pedigree
	class FamilyChartResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family info
	class FamilyInfoResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family generic result
	class FamilyResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join request
	class FamilyJoinRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join request result
	class FamilyJoinRequestResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join accepted
	class FamilyJoinAcceptedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family reputation gain
	class FamilyRepGainHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family member login/logout
	class FamilyLoginLogoutHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family summon request
	class FamilySummonRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// MapleTV broadcast
	class SendTVHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove MapleTV
	class RemoveTVHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Enable MapleTV
	class EnableTVHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Avatar megaphone display
	class SetAvatarMegaphoneHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Clear avatar megaphone
	class ClearAvatarMegaphoneHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn kite on map
	class SpawnKiteHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove kite from map
	class RemoveKiteHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cannot spawn kite
	class CannotSpawnKiteHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Ariant arena user scores
	class AriantScoreHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Ariant arena show result
	class AriantShowResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pyramid PQ gauge
	class PyramidGaugeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pyramid PQ score/rank
	class PyramidScoreHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Tournament info
	class TournamentHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Tournament match table
	class TournamentMatchTableHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Tournament prize
	class TournamentSetPrizeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Tournament UEW
	class TournamentUEWHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Dojo warp up
	class DojoWarpUpHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Throw grenade visual
	class ThrowGrenadeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Skill book learning result
	class SkillLearnItemResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Catch monster with item result
	class CatchMonsterWithItemHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Set background effect
	class SetBackEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Blocked map notification
	class BlockedMapHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Blocked server notification
	class BlockedServerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Extra pendant slot toggle
	class SetExtraPendantSlotHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Vicious hammer result
	class ViciousHammerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Vega scroll result
	class VegaScrollHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// New Year card response
	class NewYearCardHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop name change check
	class CashShopNameChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop name change possible result
	class CashShopNameChangePossibleHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop world transfer possible result
	class CashShopTransferWorldHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop gachapon item result
	class CashGachaponResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}