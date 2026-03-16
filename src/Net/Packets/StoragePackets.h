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
	// Take out an item from storage
	// mode 4: byte invtype, byte slot
	class StorageTakeOutPacket : public OutPacket
	{
	public:
		StorageTakeOutPacket(int8_t invtype, int8_t slot) : OutPacket(OutPacket::Opcode::STORAGE_ACTION)
		{
			write_byte(4);
			write_byte(invtype);
			write_byte(slot);
		}
	};

	// Store an item into storage
	// mode 5: short slot, int itemid, short quantity
	class StorageStorePacket : public OutPacket
	{
	public:
		StorageStorePacket(int16_t slot, int32_t itemid, int16_t quantity) : OutPacket(OutPacket::Opcode::STORAGE_ACTION)
		{
			write_byte(5);
			write_short(slot);
			write_int(itemid);
			write_short(quantity);
		}
	};

	// Arrange items in storage
	// mode 6: (no extra data)
	class StorageArrangePacket : public OutPacket
	{
	public:
		StorageArrangePacket() : OutPacket(OutPacket::Opcode::STORAGE_ACTION)
		{
			write_byte(6);
		}
	};

	// Deposit/withdraw mesos from storage
	// mode 7: int meso (positive = take out, negative = deposit)
	class StorageMesoPacket : public OutPacket
	{
	public:
		StorageMesoPacket(int32_t meso) : OutPacket(OutPacket::Opcode::STORAGE_ACTION)
		{
			write_byte(7);
			write_int(meso);
		}
	};

	// Close storage
	// mode 8: (no extra data)
	class StorageClosePacket : public OutPacket
	{
	public:
		StorageClosePacket() : OutPacket(OutPacket::Opcode::STORAGE_ACTION)
		{
			write_byte(8);
		}
	};
}
