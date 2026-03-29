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

#include "../OutPacket.h"

namespace ms
{
	// TAKE: GM takes item from bot inventory to their own
	// Opcode: BOT_INV_ACTION(0x168)
	class BotInvTakePacket : public OutPacket
	{
	public:
		BotInvTakePacket(int32_t bot_id, int8_t inv_type, int16_t slot)
			: OutPacket(OutPacket::Opcode::BOT_INV_ACTION)
		{
			write_byte(0); // action = TAKE
			write_int(bot_id);
			write_byte(inv_type);
			write_short(slot);
		}
	};

	// GIVE: GM gives item from their inventory to bot
	// Opcode: BOT_INV_ACTION(0x168)
	class BotInvGivePacket : public OutPacket
	{
	public:
		BotInvGivePacket(int32_t bot_id, int8_t src_inv_type, int16_t src_slot)
			: OutPacket(OutPacket::Opcode::BOT_INV_ACTION)
		{
			write_byte(1); // action = GIVE
			write_int(bot_id);
			write_byte(src_inv_type);
			write_short(src_slot);
		}
	};

	// EQUIP: Move equip between bot's equip inv and equipped slots
	// Opcode: BOT_INV_ACTION(0x168)
	class BotInvEquipPacket : public OutPacket
	{
	public:
		BotInvEquipPacket(int32_t bot_id, int16_t src_slot, int16_t dst_slot)
			: OutPacket(OutPacket::Opcode::BOT_INV_ACTION)
		{
			write_byte(2); // action = EQUIP
			write_int(bot_id);
			write_short(src_slot);
			write_short(dst_slot);
		}
	};
}
