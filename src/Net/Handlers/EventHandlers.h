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
}