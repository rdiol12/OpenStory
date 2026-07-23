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

	// Stock an item into your own shop (before opening it)
	class AddShopItemPacket : public OutPacket
	{
	public:
		AddShopItemPacket(int8_t invtype, int16_t slot, int16_t bundles, int16_t perbundle, int32_t price) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x16);
			write_byte(invtype);
			write_short(slot);
			write_short(bundles);
			write_short(perbundle);
			write_int(price);
		}
	};

	// Visit a player shop / minigame room by object id
	class MiniRoomVisitPacket : public OutPacket
	{
	public:
		MiniRoomVisitPacket(int32_t oid) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(4);
			write_int(oid);
		}
	};

	// Open a personal shop with a store permit (cash item 514xxxx)
	class CreateShopPacket : public OutPacket
	{
	public:
		CreateShopPacket(const std::string& desc, int32_t permit_itemid, int8_t room_type = 4) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0);
			write_byte(room_type);
			write_string(desc);
			write_byte(0);
			write_byte(0);
			write_byte(0);
			write_int(permit_itemid);
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
			write_byte(0x0A);  // EXIT
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

	// --- Shop / Hired Merchant Packets ---

	// Buy item from personal shop or hired merchant
	// action 5: byte slot, short quantity
	class ShopBuyPacket : public OutPacket
	{
	public:
		ShopBuyPacket(int8_t slot, int16_t quantity) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x17);
			write_byte(slot);
			write_short(quantity);
		}
	};

	// Rearrange items in hired merchant (owner only)
	// action 42
	class ShopArrangePacket : public OutPacket
	{
	public:
		ShopArrangePacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(42);
		}
	};

	// Collect stored meso from shop
	// action 28
	class ShopTakeMesoPacket : public OutPacket
	{
	public:
		ShopTakeMesoPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(28);
		}
	};

	// Open/start personal shop (owner only)
	// action 13
	class ShopOpenPacket : public OutPacket
	{
	public:
		ShopOpenPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x0B);
			write_byte(1);
		}
	};

	// Leave merchant maintenance: closes if empty, reopens store if stocked
	class MerchantMaintOffPacket : public OutPacket
	{
	public:
		MerchantMaintOffPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x27);
		}
	};

	// Tidy Up: withdraw merchant mesos, prune sold-out listings (owner only)
	class MerchantOrganizePacket : public OutPacket
	{
	public:
		MerchantOrganizePacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x28);
		}
	};

	// Take a stocked item back (merchant owner, store closed)
	class TakeItemBackPacket : public OutPacket
	{
	public:
		TakeItemBackPacket(int16_t slot) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x26);
			write_short(slot);
		}
	};

	// Take a stocked item back (personal shop owner)
	class ShopRemoveItemPacket : public OutPacket
	{
	public:
		ShopRemoveItemPacket(int16_t slot) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x1B);
			write_short(slot);
		}
	};

	// Close the whole merchant store and reclaim contents (owner only)
	class CloseMerchantPacket : public OutPacket
	{
	public:
		CloseMerchantPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(0x29);
		}
	};

	// Ask the server to validate a merchant deploy spot (Fredrick check)
	class HiredMerchantRequestPacket : public OutPacket
	{
	public:
		HiredMerchantRequestPacket() : OutPacket(OutPacket::Opcode::HIRED_MERCHANT_REQUEST) {}
	};

	// Request the most-searched-items leaderboard for the Owl dialog
	class OwlActionPacket : public OutPacket
	{
	public:
		OwlActionPacket() : OutPacket(OutPacket::Opcode::OWL_ACTION) {}
	};

	// Warp to a shop found through the Owl
	class OwlWarpPacket : public OutPacket
	{
	public:
		OwlWarpPacket(int32_t owner_id, int32_t map_id) : OutPacket(OutPacket::Opcode::OWL_WARP)
		{
			write_int(owner_id);
			write_int(map_id);
		}
	};

	// Consume an Owl of Minerva to search for an item across shops
	class UseOwlSearchPacket : public OutPacket
	{
	public:
		UseOwlSearchPacket(int16_t slot, int32_t owl_itemid, int32_t search_itemid)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(owl_itemid);
			write_int(search_itemid);
		}
	};

	// Ban a visitor from shop (owner only)
	// action 14: byte slot (0 = first visitor)
	class ShopBanVisitorPacket : public OutPacket
	{
	public:
		ShopBanVisitorPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(14);
			write_byte(0);
		}
	};

	// Blacklist management (owner only)
	// action 15
	class ShopBlacklistPacket : public OutPacket
	{
	public:
		ShopBlacklistPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(15);
		}
	};
}
