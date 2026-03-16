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
	// Create a trade room
	// action 0: byte type (3 = trade)
	class TradeCreatePacket : public OutPacket
	{
	public:
		TradeCreatePacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0);   // CREATE
			write_byte(3);   // type = trade
		}
	};

	// Invite a player to trade
	// action 2: int target_cid
	class TradeInvitePacket : public OutPacket
	{
	public:
		TradeInvitePacket(int32_t target_cid) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(2);   // INVITE
			write_int(target_cid);
		}
	};

	// Decline a trade invitation
	// action 3: (no extra data)
	class TradeDeclinePacket : public OutPacket
	{
	public:
		TradeDeclinePacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(3);   // DECLINE
		}
	};

	// Accept trade invitation and enter room
	// action 4: int room_id
	class TradeVisitPacket : public OutPacket
	{
	public:
		TradeVisitPacket(int32_t room_id) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(4);   // VISIT
			write_int(room_id);
		}
	};

	// Add item to trade
	// action 15: byte invtype, short slot, short qty, byte trade_slot
	class TradeSetItemPacket : public OutPacket
	{
	public:
		TradeSetItemPacket(int8_t invtype, int16_t slot, int16_t quantity, int8_t trade_slot) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(15);  // SET_ITEMS
			write_byte(invtype);
			write_short(slot);
			write_short(quantity);
			write_byte(trade_slot);
		}
	};

	// Set meso amount in trade
	// action 16: int meso
	class TradeSetMesoPacket : public OutPacket
	{
	public:
		TradeSetMesoPacket(int32_t meso) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(16);  // SET_MESO
			write_int(meso);
		}
	};

	// Confirm trade
	// action 17: (no extra data)
	class TradeConfirmPacket : public OutPacket
	{
	public:
		TradeConfirmPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(17);  // CONFIRM
		}
	};

	// Cancel/exit trade
	// action 10: (no extra data)
	class TradeExitPacket : public OutPacket
	{
	public:
		TradeExitPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(10);  // EXIT
		}
	};

	// Send chat in trade
	// action 6: string message
	class TradeChatPacket : public OutPacket
	{
	public:
		TradeChatPacket(const std::string& message) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(6);   // CHAT
			write_string(message);
		}
	};
}
