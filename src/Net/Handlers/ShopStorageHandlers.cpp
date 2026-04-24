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
#include "ShopStorageHandlers.h"

#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIStorage.h"
#include "../../IO/UITypes/UITrade.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIStatusBar.h"
#include "../../IO/UITypes/UIHiredMerchant.h"
#include "../../IO/UITypes/UIPersonalShop.h"
#include "../../IO/UITypes/UIMinigame.h"
#include "../../Net/Packets/TradePackets.h"
#include "Helpers/ItemParser.h"

namespace ms
{
	namespace
	{
		InventoryType::Id type_by_storage_bitfield(int16_t bitfield)
		{
			switch (bitfield)
			{
			case 4: return InventoryType::Id::EQUIP;
			case 8: return InventoryType::Id::USE;
			case 16: return InventoryType::Id::SETUP;
			case 32: return InventoryType::Id::ETC;
			case 64: return InventoryType::Id::CASH;
			default: return InventoryType::Id::NONE;
			}
		}

		UIStorage::ItemEntry parse_storage_item(InPacket& recv, InventoryType::Id forced_type, InventoryType::Id& parsed_type)
		{
			int8_t item_type = recv.read_byte();
			int32_t itemid = recv.read_int();

			parsed_type = forced_type;
			if (parsed_type == InventoryType::Id::NONE)
				parsed_type = InventoryType::by_item_id(itemid);

			bool cash = recv.read_bool();
			if (cash)
				recv.skip(8); // unique ID

			recv.skip(8); // expiration

			UIStorage::ItemEntry entry = { itemid, 1 };

			bool is_equip = (parsed_type == InventoryType::Id::EQUIP) || (item_type == 1);
			bool is_pet = (itemid >= 5000000 && itemid <= 5000102);

			if (is_equip)
			{
				recv.read_byte();  // upgrade slots
				recv.read_byte();  // level

				// 15 equip stats (Cosmic v83): STR, DEX, INT, LUK, HP, MP, WATK, MATK, WDEF, MDEF, ACC, AVOID, HANDS, SPEED, JUMP
				for (int i = 0; i < 15; i++)
					recv.read_short();

				recv.read_string(); // owner
				recv.read_short();  // flag

				if (cash)
				{
					recv.skip(10);
				}
				else
				{
					recv.read_byte();   // unk
					recv.read_byte();   // item level
					recv.read_short();  // unk
					recv.read_short();  // item exp
					recv.read_int();    // vicious
					recv.read_long();   // unk
				}

				recv.skip(12); // trailing data
			}
			else if (is_pet)
			{
				recv.read_padded_string(13);
				recv.read_byte();  // pet level
				recv.read_short(); // closeness
				recv.read_byte();  // fullness
				recv.skip(18);
			}
			else
			{
				entry.count = recv.read_short();
				recv.read_string(); // owner
				recv.read_short();  // flag

				if ((itemid / 10000 == 233) || (itemid / 10000 == 207))
					recv.skip(8);
			}

			return entry;
		}

		struct ParsedStorageItems
		{
			std::vector<UIStorage::ItemEntry> equip;
			std::vector<UIStorage::ItemEntry> use;
			std::vector<UIStorage::ItemEntry> setup;
			std::vector<UIStorage::ItemEntry> etc_items;
			std::vector<UIStorage::ItemEntry> cash;

			std::vector<UIStorage::ItemEntry>& get(InventoryType::Id type)
			{
				switch (type)
				{
				case InventoryType::Id::EQUIP: return equip;
				case InventoryType::Id::USE: return use;
				case InventoryType::Id::SETUP: return setup;
				case InventoryType::Id::ETC: return etc_items;
				case InventoryType::Id::CASH: return cash;
				default: return etc_items;
				}
			}
		};

		ParsedStorageItems parse_storage_items(InPacket& recv, uint8_t count, InventoryType::Id forced_type)
		{
			ParsedStorageItems result;

			for (size_t i = 0; i < count && recv.available(); ++i)
			{
				InventoryType::Id parsed_type = InventoryType::Id::NONE;
				UIStorage::ItemEntry entry = parse_storage_item(recv, forced_type, parsed_type);
				if (parsed_type != InventoryType::Id::NONE)
					result.get(parsed_type).push_back(entry);
			}

			return result;
		}
	}

	void StorageHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();
		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (mode)
		{
		case 0x16:
		{
			// Open storage
			if (recv.length() < 4)
				break;

			int32_t npcid = recv.read_int();
			uint8_t slots = static_cast<uint8_t>(recv.read_byte());

			recv.skip(2); // inventory mask
			recv.skip(2); // zero
			recv.skip(4); // zero
			int32_t meso = recv.read_int();
			recv.skip(2); // zero

			uint8_t item_count = static_cast<uint8_t>(recv.read_byte());
			ParsedStorageItems items = parse_storage_items(recv, item_count, InventoryType::Id::NONE);

			if (recv.length() >= 2)
				recv.skip(2);
			if (recv.length() >= 1)
				recv.skip(1);

			const Inventory& inv = Stage::get().get_player().get_inventory();
			UI::get().emplace<UIStorage>(inv);

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->open(npcid, slots, meso);

				InventoryType::Id types[] = {
					InventoryType::Id::EQUIP, InventoryType::Id::USE,
					InventoryType::Id::SETUP, InventoryType::Id::ETC,
					InventoryType::Id::CASH
				};
				std::vector<UIStorage::ItemEntry> tab_items[] = {
					items.equip, items.use, items.setup, items.etc_items, items.cash
				};
				storage_ui->set_items_for_all(types, 5, tab_items);
			}

			break;
		}
		case 0x09:
		case 0x0D:
		{
			// Take out (0x09) or Store (0x0D) result — updates one tab
			if (recv.length() < 2)
				break;

			uint8_t slots = static_cast<uint8_t>(recv.read_byte());
			int16_t type_bitfield = recv.read_short();
			InventoryType::Id type = type_by_storage_bitfield(type_bitfield);

			recv.skip(2); // zero
			recv.skip(4); // zero

			uint8_t count = static_cast<uint8_t>(recv.read_byte());
			ParsedStorageItems parsed = parse_storage_items(recv, count, type);

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_slots(slots);
				if (type != InventoryType::Id::NONE)
					storage_ui->set_items_for_tab(type, parsed.get(type));
			}

			break;
		}
		case 0x0F:
		{
			// Arrange result — updates all tabs
			if (recv.length() < 2)
				break;

			uint8_t slots = static_cast<uint8_t>(recv.read_byte());

			recv.skip(1);  // mask byte (writeByte(124))
			recv.skip(10); // zeroes

			uint8_t count = static_cast<uint8_t>(recv.read_byte());
			ParsedStorageItems items = parse_storage_items(recv, count, InventoryType::Id::NONE);

			if (recv.length() >= 1)
				recv.skip(1);

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_slots(slots);

				InventoryType::Id types[] = {
					InventoryType::Id::EQUIP, InventoryType::Id::USE,
					InventoryType::Id::SETUP, InventoryType::Id::ETC,
					InventoryType::Id::CASH
				};
				std::vector<UIStorage::ItemEntry> tab_items[] = {
					items.equip, items.use, items.setup, items.etc_items, items.cash
				};
				storage_ui->set_items_for_all(types, 5, tab_items);
			}

			break;
		}
		case 0x13:
		{
			// Meso update
			if (recv.length() < 2)
				break;

			uint8_t slots = static_cast<uint8_t>(recv.read_byte());

			recv.skip(2); // type mask
			recv.skip(2); // zero
			recv.skip(4); // zero

			int32_t meso = recv.read_int();

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_slots(slots);
				storage_ui->set_meso(meso);
			}

			break;
		}
		case 0x0A:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "Your inventory is full.");
			break;
		}
		case 0x0B:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "You do not have enough mesos.");
			break;
		}
		case 0x0C:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "You already have that one-of-a-kind item.");
			break;
		}
		case 0x11:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "Storage is full.");
			break;
		}
		default:
			break;
		}

		UI::get().enable();
	}

	void PlayerInteractionHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t action = recv.read_byte();

		switch (action)
		{
		case 2:  // INVITE
		{
			int8_t type = recv.read_byte();
			std::string inviter = recv.read_string();

			// The invite carries the room id the inviter already
			// created. On accept we VISIT that room rather than
			// CREATEing a new one (the old code skipped this int).
			int32_t room_id = recv.available() >= 4 ? recv.read_int() : 0;

			if (type == 3) // Trade invite
			{
				// Show accept/decline dialog
				UI::get().emplace<UIAlarmInvite>(
					"Trade request from " + inviter + ". Accept?",
					[room_id](bool yes)
					{
						if (yes)
							TradeVisitPacket(room_id).dispatch();
						else
							TradeDeclinePacket().dispatch();
					}
				);

				if (auto statusbar = UI::get().get_element<UIStatusBar>())
					statusbar->notify();
			}
			break;
		}

		case 4:  // VISIT — partner joined the trade
		{
			uint8_t slot = recv.read_byte();

			// Skip partner appearance data (addCharLook)
			// This is complex; skip what we can
			// For now, just read the partner name at the end
			// We'll skip the look data by reading until we find the name

			// In v83: addCharLook writes gender(byte), skin(byte), face(int),
			// hair flag(byte), hair(int), then equipped items, then cash items,
			// then end marker -1, then pets
			// This is variable length, so we need to parse it

			int8_t gender = recv.read_byte();
			int8_t skin = recv.read_byte();
			int32_t face = recv.read_int();

			// mega flag
			recv.skip_byte();

			int32_t hair = recv.read_int();

			// Equipped items (slot, itemid pairs until slot == -1)
			while (recv.available())
			{
				int8_t equipped_slot = recv.read_byte();
				if (equipped_slot == -1)
					break;
				recv.skip_int(); // item id
			}

			// Cash items (slot, itemid pairs until slot == -1)
			while (recv.available())
			{
				int8_t cash_slot = recv.read_byte();
				if (cash_slot == -1)
					break;
				recv.skip_int(); // item id
			}

			// Skip remaining look data (weapon sticker, pets)
			recv.skip_int(); // weapon sticker

			for (int i = 0; i < 3; i++)
				recv.skip_int(); // pet ids

			std::string partner_name = recv.read_string();

			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_partner(slot, partner_name);

			break;
		}

		case 5:  // ROOM — trade room setup
		{
			int8_t room_type = recv.read_byte();

			if (room_type == 3) // Trade
			{
				int8_t room_mode = recv.read_byte();
				uint8_t player_num = recv.read_byte();

				// Create the trade UI
				UI::get().emplace<UITrade>();

				if (player_num == 1)
				{
					// Partner data follows
					recv.skip_byte(); // slot

					// Skip partner addCharLook
					recv.skip_byte(); // gender
					recv.skip_byte(); // skin
					recv.skip_int();  // face
					recv.skip_byte(); // mega flag
					recv.skip_int();  // hair

					// Equipped items
					while (recv.available())
					{
						int8_t eslot = recv.read_byte();
						if (eslot == -1) break;
						recv.skip_int();
					}

					// Cash items
					while (recv.available())
					{
						int8_t cslot = recv.read_byte();
						if (cslot == -1) break;
						recv.skip_int();
					}

					recv.skip_int(); // weapon sticker
					for (int i = 0; i < 3; i++)
						recv.skip_int(); // pet ids

					std::string partner_name = recv.read_string();

					if (auto trade = UI::get().get_element<UITrade>())
						trade->set_partner(1, partner_name);
				}

				// Self data
				uint8_t self_num = recv.read_byte();

				// Skip self addCharLook
				recv.skip_byte(); // gender
				recv.skip_byte(); // skin
				recv.skip_int();  // face
				recv.skip_byte(); // mega flag
				recv.skip_int();  // hair

				// Equipped items
				while (recv.available())
				{
					int8_t eslot = recv.read_byte();
					if (eslot == -1) break;
					recv.skip_int();
				}

				// Cash items
				while (recv.available())
				{
					int8_t cslot = recv.read_byte();
					if (cslot == -1) break;
					recv.skip_int();
				}

				recv.skip_int(); // weapon sticker
				for (int i = 0; i < 3; i++)
					recv.skip_int(); // pet ids

				recv.skip_string(); // self name

				// End marker
				if (recv.available())
					recv.skip_byte(); // 0xFF
			}
			break;
		}

		case 10: // EXIT — trade cancelled/completed
		{
			uint8_t player_num = recv.read_byte();
			uint8_t result = recv.read_byte();

			if (auto trade = UI::get().get_element<UITrade>())
			{
				trade->trade_result(result);

				// Close after showing result
				if (result == 7) // SUCCESS
				{
					// Trade completed successfully
				}
			}

			// Deactivate trade window
			if (auto trade = UI::get().get_element<UITrade>())
				trade->deactivate();

			break;
		}

		case 15: // SET_ITEMS — item added to trade
		{
			uint8_t player_num = recv.read_byte();
			uint8_t trade_slot = recv.read_byte();

			// Parse item info — simplified version
			// Full item info includes type detection, but for trade we read basics
			int8_t item_type = recv.read_byte(); // 1=equip, 2=use/etc

			if (item_type == 1)
			{
				// Equip item
				int32_t itemid = recv.read_int();

				// Skip equip stats (variable, but typically ~40+ bytes)
				// Flags
				bool has_uid = recv.read_bool();
				if (has_uid)
					recv.skip_long(); // unique id

				recv.skip_long(); // expiration

				// Equip stats: 15 shorts + owner string + flags
				for (int i = 0; i < 15; i++)
					recv.skip_short();

				recv.skip_string(); // owner
				recv.skip_byte();   // flag

				// Additional bytes depend on type
				if (recv.available())
				{
					recv.skip_byte(); // level up type
					recv.skip_byte(); // level
					recv.skip_int();  // exp
					recv.skip_int();  // vicious
					recv.skip_int();  // flag2
				}

				if (auto trade = UI::get().get_element<UITrade>())
					trade->set_item(player_num, trade_slot, itemid, 1);
			}
			else
			{
				// Regular item
				int32_t itemid = recv.read_int();

				bool has_uid = recv.read_bool();
				if (has_uid)
					recv.skip_long();

				recv.skip_long(); // expiration

				int16_t quantity = recv.read_short();
				recv.skip_string(); // owner
				recv.skip_short();  // flag

				// Rechargeable check
				if ((itemid / 10000) == 207 || (itemid / 10000) == 233)
					recv.skip_long(); // recharge bytes

				if (auto trade = UI::get().get_element<UITrade>())
					trade->set_item(player_num, trade_slot, itemid, quantity);
			}
			break;
		}

		case 16: // SET_MESO
		{
			uint8_t player_num = recv.read_byte();
			int32_t meso = recv.read_int();

			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_meso(player_num, meso);

			break;
		}

		case 17: // CONFIRM
		{
			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_partner_confirmed();

			break;
		}

		case 6:  // CHAT
		{
			if (recv.available())
			{
				recv.skip_byte(); // CHAT_THING (0x08)
				uint8_t speaker = recv.read_byte();
				std::string message = recv.read_string();

				if (auto trade = UI::get().get_element<UITrade>())
					trade->add_chat(message);
			}
			break;
		}

		default:
			break;
		}
	}

	// Opcode 306 (0x132) — CONFIRM_SHOP_TRANSACTION
	// Server confirms a shop buy/sell transaction result
	void ConfirmShopTransactionHandler::handle(InPacket& recv) const
	{
		int8_t code = recv.read_byte();

		// code 0 = buy success, code 8 = sell/recharge success
		// Other codes = errors (not enough mesos, inventory full, etc.)
		if (code != 0 && code != 8)
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "Transaction failed.");
		}
	}

	// Opcode 322 (0x142) — PARCEL (Duey delivery system)
	// Handles parcel/delivery notifications and operations
	void ParcelHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		switch (operation)
		{
			case 0x08:
			{
				// Open Duey window with package list
				recv.read_byte(); // unused
				int8_t count = recv.read_byte();

				for (int8_t i = 0; i < count; i++)
				{
					recv.read_int(); // package id
					recv.read_padded_string(13); // sender name
					recv.read_int(); // mesos
					recv.read_long(); // sent timestamp

					recv.read_int(); // has_message flag
					recv.skip(200); // message buffer (fixed 200 bytes)

					recv.read_byte(); // unused

					int8_t has_item = recv.read_byte();

					if (has_item == 1)
						ItemParser::skip_item(recv);
				}

				recv.read_byte(); // terminal byte

				if (auto chatbar = UI::get().get_element<UIChatBar>())
				{
					std::string msg = "[Duey] You have " + std::to_string(count) + " package(s).";
					chatbar->send_chatline(msg, UIChatBar::LineType::YELLOW);
				}

				break;
			}
			case 0x09:
			case 0x14:
			{
				// Enable actions (success)
				break;
			}
			case 0x0A:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "Not enough mesos for delivery.");

				break;
			}
			case 0x0B:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "Incorrect delivery request.");

				break;
			}
			case 0x0C:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "That character does not exist.");

				break;
			}
			case 0x0D:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "Cannot send to yourself.");

				break;
			}
			case 0x0E:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "Receiver's storage is full.");

				break;
			}
			case 0x12:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::WHITE, "Package sent successfully!");

				break;
			}
			case 0x15:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, "Not enough inventory space.");

				break;
			}
			case 0x17:
			{
				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::WHITE, "Package received!");

				break;
			}
			case 0x19:
			{
				// Parcel received notification
				std::string from = recv.read_string();
				recv.read_bool(); // quick delivery

				if (auto chatbar = UI::get().get_element<UIChatBar>())
					chatbar->send_chatline("[Duey] Package received from " + from + "!", UIChatBar::LineType::YELLOW);

				break;
			}
			case 0x1B:
			{
				// Package notification
				bool quick = recv.read_bool();

				if (auto chatbar = UI::get().get_element<UIChatBar>())
				{
					std::string msg = quick ? "[Duey] Quick delivery package has arrived!" : "[Duey] You have a package waiting!";
					chatbar->send_chatline(msg, UIChatBar::LineType::YELLOW);
				}

				break;
			}
		}
	}

	void SpawnHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t item_id = recv.read_int();
		Point<int16_t> position = recv.read_point();
		recv.read_short(); // 0
		std::string owner_name = recv.read_string();
		recv.read_byte(); // 0x05
		int32_t object_id = recv.read_int();
		std::string description = recv.read_string();
		recv.read_byte(); // sprite index
		recv.read_byte(); // 1
		recv.read_byte(); // 4

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] " + owner_name + "'s shop: " + description, UIChatBar::LineType::YELLOW);
	}

	void DestroyHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] A hired merchant has been removed.", UIChatBar::LineType::YELLOW);
	}

	void UpdateHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		recv.read_byte(); // 5 = hired merchant type
		int32_t object_id = recv.read_int();
		std::string description = recv.read_string();
		recv.read_byte(); // sprite index
		int8_t cur_visitors = recv.read_byte();
		int8_t max_visitors = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] " + description + " (" + std::to_string(cur_visitors) + "/" + std::to_string(max_visitors) + " visitors)", UIChatBar::LineType::YELLOW);
	}

	void FredrickMessageHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Fredrick] Message (op=" + std::to_string(operation) + ")", UIChatBar::LineType::YELLOW);
	}

	void FredrickHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		// 0x23 = retrieve items, 0x24 = error
		// Full parsing would require ItemInfo deserialization
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Fredrick] Response (op=" + std::to_string(op) + ")", UIChatBar::LineType::YELLOW);
	}
}
