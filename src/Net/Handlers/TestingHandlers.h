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
}