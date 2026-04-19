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
	// Handler for entering the Cash Shop
	class SetCashShopHandler : public PacketHandler
	{
	public:
		void handle(InPacket& recv) const override;

	private:
		void transition() const;
	};

	// Returns the UI scale that was active immediately before the cash shop
	// transition. The cash shop layout is authored at 1:1 for a 1024x768
	// window, so the transition forces UI_SCALE to 1.0. Call this on exit
	// to restore the user's previous scale.
	float get_pre_cashshop_ui_scale();

	// Handler for Cash Shop operation responses (buy, coupon, etc.)
	class CashShopOperationHandler : public PacketHandler
	{
	public:
		void handle(InPacket& recv) const override;
	};

	// Handler for entering MTS (SET_ITC, opcode 126)
	class SetITCHandler : public PacketHandler
	{
	public:
		void handle(InPacket& recv) const override;
	};

	// Cash shop cash query result
	class QueryCashResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop name change check
	class CashShopNameChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop name change possible result
	class CashShopNameChangePossibleHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop world transfer possible result
	class CashShopTransferWorldHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Cash shop gachapon item result
	class CashGachaponResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}