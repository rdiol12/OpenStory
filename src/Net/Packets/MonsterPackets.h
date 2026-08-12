//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
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

#include "../OutPacket.h"

#include <cstdint>
#include <vector>

namespace ms
{
	// Custom protocol, no canonical v83 layout — see docs/PROTOCOL_MONSTER.md.
	class MonsterLifeEnterPacket : public OutPacket
	{
	public:
		MonsterLifeEnterPacket(int32_t owner_charid) : OutPacket(OutPacket::Opcode::MLIFE_OP)
		{
			write_byte(0x00);
			write_int(owner_charid);
		}
	};

	class MonsterLifeLeavePacket : public OutPacket
	{
	public:
		MonsterLifeLeavePacket() : OutPacket(OutPacket::Opcode::MLIFE_OP)
		{
			write_byte(0x01);
		}
	};

	class MonsterLifePlacePacket : public OutPacket
	{
	public:
		MonsterLifePlacePacket(int32_t cardid, int16_t x, int16_t y)
			: OutPacket(OutPacket::Opcode::MLIFE_OP)
		{
			write_byte(0x02);
			write_int(cardid);
			write_short(x);
			write_short(y);
		}
	};

	class MonsterLifeMovePacket : public OutPacket
	{
	public:
		MonsterLifeMovePacket(int32_t slotid, int16_t x, int16_t y)
			: OutPacket(OutPacket::Opcode::MLIFE_OP)
		{
			write_byte(0x03);
			write_int(slotid);
			write_short(x);
			write_short(y);
		}
	};

	class MonsterLifeRemovePacket : public OutPacket
	{
	public:
		MonsterLifeRemovePacket(int32_t slotid) : OutPacket(OutPacket::Opcode::MLIFE_OP)
		{
			write_byte(0x04);
			write_int(slotid);
		}
	};

	class MonsterBattleOpenPacket : public OutPacket
	{
	public:
		MonsterBattleOpenPacket() : OutPacket(OutPacket::Opcode::MBATTLE_OP)
		{
			write_byte(0x00);
		}
	};

	class MonsterBattleSetTeamPacket : public OutPacket
	{
	public:
		MonsterBattleSetTeamPacket(const std::vector<int32_t>& cardids)
			: OutPacket(OutPacket::Opcode::MBATTLE_OP)
		{
			write_byte(0x01);
			write_byte(static_cast<int8_t>(cardids.size()));

			for (int32_t id : cardids)
				write_int(id);
		}
	};
}
