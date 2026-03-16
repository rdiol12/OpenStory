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
	// Opcode: BUY_CS_ITEM(215)
	class BuyCashItemPacket : public OutPacket
	{
	public:
		// Request the server to purchase a cash shop item
		BuyCashItemPacket(int8_t currency, int32_t sn_item_id) : OutPacket(OutPacket::Opcode::BUY_CS_ITEM)
		{
			write_byte(currency);
			write_int(sn_item_id);
		}
	};

	// Opcode: COUPON_CODE(216)
	class CouponCodePacket : public OutPacket
	{
	public:
		// Redeem a coupon code in the cash shop
		CouponCodePacket(const std::string& code) : OutPacket(OutPacket::Opcode::COUPON_CODE)
		{
			write_string(code);
		}
	};
}
