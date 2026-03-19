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
	// Spawn a character on the stage
	// Opcode: SPAWN_CHAR(160)
	class SpawnCharHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a character from the stage
	// Opcode: REMOVE_CHAR(161)
	class RemoveCharHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a pet on the stage
	// Opcode: SPAWN_PET(168)
	class SpawnPetHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Move a character
	// Opcode: CHAR_MOVED(185)
	class CharMovedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Update the look of a character
	// Opcode: UPDATE_CHARLOOK(197)
	class UpdateCharLookHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Display an effect on a character
	// Opcode: SHOW_FOREIGN_EFFECT(198)
	class ShowForeignEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a mob on the stage
	// Opcode: SPAWN_MOB(236)
	class SpawnMobHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a map from the stage, either by killing it or making it invisible.
	// Opcode: KILL_MOB(237)
	class KillMobHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a mob on the stage and take control of it
	// Opcode: SPAWN_MOB_C(238)
	class SpawnMobControllerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Update mob state and position with the client
	// Opcode: MOB_MOVED(239)
	class MobMovedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Updates a mob's hp with the client
	// Opcode: SHOW_MOB_HP(250)
	class ShowMobHpHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Applies a status effect to a mob
	// Opcode: APPLY_MONSTER_STATUS(242)
	class ApplyMonsterStatusHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cancels a status effect on a mob
	// Opcode: CANCEL_MONSTER_STATUS(243)
	class CancelMonsterStatusHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawns a summon on the map
	// Opcode: SPAWN_SPECIAL_MAPOBJECT(175)
	class SpawnSummonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Removes a summon from the map
	// Opcode: REMOVE_SPECIAL_MAPOBJECT(176)
	class RemoveSummonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Updates a summon's position
	// Opcode: MOVE_SUMMON(177)
	class MoveSummonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Summon attacks a mob
	// Opcode: SUMMON_ATTACK(178)
	class SummonAttackHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// A summon takes damage
	// Opcode: DAMAGE_SUMMON(179)
	class DamageSummonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove an NPC from the map
	// Opcode: REMOVE_NPC(258)
	class RemoveNpcHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a mystic door on the map
	// Opcode: SPAWN_DOOR(275)
	class SpawnDoorHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a mystic door from the map
	// Opcode: REMOVE_DOOR(276)
	class RemoveDoorHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a mist effect on the map
	// Opcode: SPAWN_MIST(273)
	class SpawnMistHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a mist effect from the map
	// Opcode: REMOVE_MIST(274)
	class RemoveMistHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Update pet position
	// Opcode: MOVE_PET(170)
	class MovePetHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet chat message
	// Opcode: PET_CHAT(171)
	class PetChatHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet command response
	// Opcode: PET_COMMAND(174)
	class PetCommandHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Server acknowledges mob movement
	// Opcode: MOVE_MONSTER_RESPONSE(240)
	class MoveMonsterResponseHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Damage numbers on a mob (from other players)
	// Opcode: DAMAGE_MONSTER(246)
	class DamageMonsterHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cancel a skill effect on a character
	// Opcode: CANCEL_SKILL_EFFECT(191)
	class CancelSkillEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Display or hide a chalkboard above a character
	// Opcode: CHALKBOARD(164)
	class ChalkboardHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn an Evan dragon on the map
	// Opcode: SPAWN_DRAGON(181)
	class SpawnDragonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Move an Evan dragon
	// Opcode: MOVE_DRAGON(182)
	class MoveDragonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove an Evan dragon from the map
	// Opcode: REMOVE_DRAGON(183)
	class RemoveDragonHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Summon uses a skill
	// Opcode: SUMMON_SKILL(180)
	class SummonSkillHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn an NPC on the current stage
	// Opcode: SPAWN_NPC(257)
	class SpawnNpcHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn an NPC on the current stage and take control of it
	// Opcode: SPAWN_NPC_C(259)
	class SpawnNpcControllerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Drop an item on the stage
	// Opcode: DROP_LOOT(268)
	class DropLootHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove an item from the stage
	// Opcode: REMOVE_LOOT(269)
	class RemoveLootHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Change state of reactor
	// Opcode: HIT_REACTOR(277)
	class HitReactorHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Parse a ReactorSpawn and send it to the Stage spawn queue
	// Opcode: SPAWN_REACTOR(279)
	class SpawnReactorHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a reactor from the stage
	// Opcode: REMOVE_REACTOR(280)
	class RemoveReactorHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}