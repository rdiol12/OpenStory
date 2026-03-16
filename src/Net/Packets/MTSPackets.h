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
	// Enter the Maple Trading System
	class EnterMTSPacket : public OutPacket
	{
	public:
		EnterMTSPacket() : OutPacket(OutPacket::Opcode::ENTER_MTS)
		{
		}
	};

	// MTS operation: sell item (action 2)
	class MTSSellPacket : public OutPacket
	{
	public:
		MTSSellPacket(int8_t itemtype, int32_t itemid, int16_t stars,
			const std::string& owner, int16_t slot, int16_t quantity, int32_t price)
			: OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(2); // sell
			write_byte(itemtype);
			write_int(itemid);
			write_short(0);
			skip(7);

			if (itemtype == 1)
			{
				// Equip: skip 32 bytes padding
				skip(32);
			}
			else
			{
				write_short(stars);
			}

			write_string(owner);

			if (itemtype == 1)
				skip(32);
			else
				write_short(0);

			// Rechargeable padding
			if (itemtype != 1 && (itemid / 10000 == 207 || itemid / 10000 == 233))
				skip(8);

			write_int(slot);

			if (itemtype != 1)
			{
				if (itemid / 10000 == 207 || itemid / 10000 == 233)
					skip(4);
				else
					write_int(quantity);
			}
			else
			{
				write_int(quantity);
			}

			write_int(price);
		}
	};

	// MTS operation: change page (action 5)
	class MTSChangePagePacket : public OutPacket
	{
	public:
		MTSChangePagePacket(int32_t tab, int32_t type, int32_t page)
			: OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(5);
			write_int(tab);
			write_int(type);
			write_int(page);
		}
	};

	// MTS operation: search (action 6)
	class MTSSearchPacket : public OutPacket
	{
	public:
		MTSSearchPacket(int32_t tab, int32_t type, int32_t ci, const std::string& search)
			: OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(6);
			write_int(tab);
			write_int(type);
			write_int(0);
			write_int(ci);
			write_string(search);
		}
	};

	// MTS operation: cancel sale (action 7)
	class MTSCancelSalePacket : public OutPacket
	{
	public:
		MTSCancelSalePacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(7);
			write_int(id);
		}
	};

	// MTS operation: transfer item from transfer inv (action 8)
	class MTSTransferPacket : public OutPacket
	{
	public:
		MTSTransferPacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(8);
			write_int(id);
		}
	};

	// MTS operation: add to cart (action 9)
	class MTSAddCartPacket : public OutPacket
	{
	public:
		MTSAddCartPacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(9);
			write_int(id);
		}
	};

	// MTS operation: remove from cart (action 10)
	class MTSRemoveCartPacket : public OutPacket
	{
	public:
		MTSRemoveCartPacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(10);
			write_int(id);
		}
	};

	// MTS operation: buy item (action 16)
	class MTSBuyPacket : public OutPacket
	{
	public:
		MTSBuyPacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(16);
			write_int(id);
		}
	};

	// MTS operation: buy from cart (action 17)
	class MTSBuyFromCartPacket : public OutPacket
	{
	public:
		MTSBuyFromCartPacket(int32_t id) : OutPacket(OutPacket::Opcode::MTS_OPERATION)
		{
			write_byte(17);
			write_int(id);
		}
	};
}
