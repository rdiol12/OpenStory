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
#include "EventHandlers.h"
#include "SocialHandlers.h"

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

	// Report player result (SUE_CHARACTER_RESULT)
	class SueCharacterResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Set gender packet
	class SetGenderHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Admin/GM command result
	class AdminResultHandler : public PacketHandler
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

	// Field obstacle on/off list (gates, platforms in PQs)
	class FieldObstacleOnOffHandler : public PacketHandler
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

	// Stop the on-screen clock/timer
	// Opcode: STOP_CLOCK(154)
	class StopClockHandler : public PacketHandler
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