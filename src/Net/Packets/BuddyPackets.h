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
	// Opcode: BUDDYLIST_MODIFY(130)
	// Mode 1: Add buddy
	class AddBuddyPacket : public OutPacket
	{
	public:
		AddBuddyPacket(const std::string& name, const std::string& group = "Default Group") : OutPacket(OutPacket::Opcode::BUDDYLIST_MODIFY)
		{
			write_byte(1);
			write_string(name);
			write_string(group);
		}
	};

	// Mode 2: Accept buddy request
	class AcceptBuddyPacket : public OutPacket
	{
	public:
		AcceptBuddyPacket(int32_t cid) : OutPacket(OutPacket::Opcode::BUDDYLIST_MODIFY)
		{
			write_byte(2);
			write_int(cid);
		}
	};

	// Mode 3: Delete buddy
	class DeleteBuddyPacket : public OutPacket
	{
	public:
		DeleteBuddyPacket(int32_t cid) : OutPacket(OutPacket::Opcode::BUDDYLIST_MODIFY)
		{
			write_byte(3);
			write_int(cid);
		}
	};

	// Opcode: WHISPER(120)
	// Whisper message to a player
	class WhisperPacket : public OutPacket
	{
	public:
		WhisperPacket(const std::string& name, const std::string& message) : OutPacket(OutPacket::Opcode::WHISPER)
		{
			write_byte(0x06);
			write_string(name);
			write_string(message);
		}
	};

	// Find player location
	class FindPlayerPacket : public OutPacket
	{
	public:
		FindPlayerPacket(const std::string& name) : OutPacket(OutPacket::Opcode::WHISPER)
		{
			write_byte(0x05);
			write_string(name);
		}
	};
}
