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
	// NPC Storage/Warehouse UI
	class StorageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Player interaction: trade, miniroom, player shop
	class PlayerInteractionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// NPC shop transaction confirmation (opcode 0x132)
	class ConfirmShopTransactionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Parcel/Duey delivery system (opcode 0x142)
	class ParcelHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Spawn a hired merchant box on the map
	// Opcode: SPAWN_HIRED_MERCHANT(265)
	class SpawnHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Remove a hired merchant box from the map
	// Opcode: DESTROY_HIRED_MERCHANT(266)
	class DestroyHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Update a hired merchant box info
	// Opcode: UPDATE_HIRED_MERCHANT(267)
	class UpdateHiredMerchantHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Fredrick message (merchant storage NPC)
	// Opcode: FREDRICK_MESSAGE(310)
	class FredrickMessageHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Fredrick merchant storage retrieval
	// Opcode: FREDRICK(311)
	class FredrickHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}
