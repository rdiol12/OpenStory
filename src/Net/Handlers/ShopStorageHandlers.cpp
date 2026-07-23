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

#include "../../Gameplay/HiredMerchants.h"
#include "../../Gameplay/MiniRooms.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIStorage.h"
#include "../../IO/UITypes/UITrade.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIStatusBar.h"
#include "../../IO/UITypes/UIHiredMerchant.h"
#include "../../IO/UITypes/UIOwl.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIPersonalShop.h"
#include "../../IO/UITypes/UIMinigame.h"
#include "../../Net/Packets/TradePackets.h"
#include "Helpers/ItemParser.h"
#include "Helpers/LoginParser.h"

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

		case 4:  // VISIT — someone joined the room
		{
			{
				auto shop = UI::get().get_element<UIPersonalShop>();
				auto hm = UI::get().get_element<UIHiredMerchant>();
				bool shop_open = shop && shop->is_active();
				bool hm_open = hm && hm->is_active();

				if (shop_open || hm_open)
				{
					int8_t vslot = recv.read_byte();
					LookEntry vlook = LoginParser::parse_look(recv);
					std::string vname = recv.read_string();

					if (shop_open)
					{
						chat::log("[dbg] VISIT slot=" + std::to_string(vslot) + " name=" + vname, chat::LineType::YELLOW);
						shop->set_slot_look(vslot, vlook, vname);
						shop->add_chat(vname + " entered the shop.", 0);
					}
					else
					{
						hm->set_slot_look(vslot, vlook, vname);
						hm->add_chat(vname + " entered the shop.", 0);
					}

					break;
				}
			}

			uint8_t slot = recv.read_byte();

			// addCharLook — identical layout to the login/char-select
			// look block (gender, skin, face, mega flag, hair, equips,
			// masked equips, weapon sticker, pets), so reuse the login
			// parser and hand the result to the trade UI so it can draw
			// the partner's real avatar.
			LookEntry look = LoginParser::parse_look(recv);

			std::string partner_name = recv.read_string();

			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_partner(slot, partner_name, look);

			break;
		}

		case 5:  // ROOM — trade room setup
		{
			int8_t room_type = recv.read_byte();

			if (room_type == 0)
			{
				int8_t code = recv.read_byte();
				std::string msg;

				switch (code)
				{
				case 1: msg = "That room is already closed."; break;
				case 2: msg = "The room is full."; break;
				case 4: msg = "You can't do that while dead."; break;
				case 6: msg = "You need a store permit to do that."; break;
				case 10: msg = "You can't open a store this close to a portal."; break;
				case 13: msg = "You can't set up a store right here."; break;
				case 15: msg = "Stores can only be opened in the Free Market."; break;
				case 17: msg = "You may not enter this store."; break;
				case 18: msg = "The owner is doing store maintenance."; break;
				case 22: msg = "Incorrect password."; break;
				default: msg = "Unable to do that right now. (" + std::to_string(code) + ")"; break;
				}

				if (auto messenger = UI::get().get_element<UIStatusMessenger>())
					messenger->show_status(Color::Name::RED, msg);

				break;
			}

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

					// Partner addCharLook — parse and keep for the avatar
					LookEntry look = LoginParser::parse_look(recv);

					std::string partner_name = recv.read_string();

					if (auto trade = UI::get().get_element<UITrade>())
						trade->set_partner(1, partner_name, look);
				}

				// Self data
				uint8_t self_num = recv.read_byte();

				// Self addCharLook — parse to advance the packet, but
				// discard it: we already have our own look locally.
				LoginParser::parse_look(recv);

				recv.skip_string(); // self name

				// End marker
				if (recv.available())
					recv.skip_byte(); // 0xFF
			}
			else if (room_type == 5) // Hired merchant
			{
				recv.read_byte();
				int16_t myslot = recv.read_short();
				int32_t hm_itemid = recv.read_int();
				recv.read_string();

				std::vector<std::pair<int8_t, std::pair<LookEntry, std::string>>> hm_visitors;

				while (recv.available())
				{
					int8_t vslot = recv.read_byte();

					if (vslot == -1 || static_cast<uint8_t>(vslot) == 0xFF)
						break;

					LookEntry vlook = LoginParser::parse_look(recv);
					std::string vname = recv.read_string();
					hm_visitors.push_back({ vslot, { vlook, vname } });
				}

				bool owner = (myslot == 0);
				int16_t msg_count = recv.read_short();
				std::vector<std::pair<std::string, int8_t>> hm_msgs;

				for (int16_t i = 0; i < msg_count; i++)
				{
					std::string mtext = recv.read_string();
					int8_t mslot = recv.read_byte();
					hm_msgs.push_back({ mtext, mslot });
				}

				std::string owner_name = recv.read_string();
				bool hm_first = false;

				if (owner)
				{
					recv.read_short();
					recv.read_short();
					hm_first = recv.read_byte() != 0;
					int8_t sold_n = recv.read_byte();

					for (int8_t i = 0; i < sold_n; i++)
					{
						recv.read_int();
						recv.read_short();
						recv.read_int();
						recv.read_string();
					}

					recv.read_int();
				}

				std::string desc = recv.read_string();
				recv.read_byte();
				int64_t meso = recv.read_int();
				int8_t item_count = recv.read_byte();

				if (item_count == 0)
					recv.read_byte();

				UI::get().remove(UIElement::Type::HIREDMERCHANT);
				UI::get().emplace<UIHiredMerchant>();

				if (auto hm = UI::get().get_element<UIHiredMerchant>())
				{
					hm->set_owner(owner_name + " : " + desc);
					hm->set_mode(owner, hm_first);
					hm->set_merchant(hm_itemid);
					hm->set_slot_look(0, LookEntry(), owner_name);
					hm->set_meso(meso);

					for (const auto& v : hm_visitors)
						hm->set_slot_look(v.first, v.second.first, v.second.second);

					for (const auto& m : hm_msgs)
						hm->add_chat(m.first, m.second);

					hm->clear_items();

					for (int8_t i = 0; i < item_count; i++)
					{
						int16_t bundles = recv.read_short();
						recv.read_short();
						int32_t price = recv.read_int();
						int32_t itemid = ItemParser::skip_item(recv);
						hm->add_item(i, itemid, bundles, price);

						if (bundles <= 0)
							hm->set_sold_out(i);
					}
				}
			}
			else if (room_type == 4) // Personal shop
			{
				recv.read_byte();
				int8_t myslot = recv.read_byte();

				int8_t sold_count = recv.read_byte();

				for (int8_t i = 0; i < sold_count; i++)
				{
					recv.read_int();
					recv.read_short();
					recv.read_int();
					recv.read_string();
				}

				LookEntry owner_look = LoginParser::parse_look(recv);
				std::string owner_name = recv.read_string();

				std::vector<std::pair<int8_t, std::pair<LookEntry, std::string>>> visitors;

				while (recv.available())
				{
					int8_t vslot = recv.read_byte();

					if (vslot == -1 || vslot == 0x7F || static_cast<uint8_t>(vslot) == 0xFF)
						break;

					LookEntry vlook = LoginParser::parse_look(recv);
					std::string vname = recv.read_string();
					visitors.push_back({ vslot, { vlook, vname } });
				}

				std::string desc = recv.read_string();
				recv.read_byte();
				int8_t item_count = recv.read_byte();

				UI::get().remove(UIElement::Type::PERSONALSHOP);
				UI::get().emplace<UIPersonalShop>();

				if (auto shop = UI::get().get_element<UIPersonalShop>())
				{
					shop->set_owner(owner_name + " : " + desc);
					shop->set_mode(myslot == 0);
					shop->set_slot_look(0, owner_look, owner_name);

					for (const auto& v : visitors)
						shop->set_slot_look(v.first, v.second.first, v.second.second);

					shop->clear_items();

					for (int8_t i = 0; i < item_count; i++)
					{
						int16_t bundles = recv.read_short();
						recv.read_short();
						int32_t price = recv.read_int();
						int32_t itemid = ItemParser::skip_item(recv);
						shop->add_item(i, itemid, bundles, price);

						if (bundles <= 0)
							shop->set_sold_out(i);
					}
				}
			}
			break;
		}

		case 0x19: // UPDATE_MERCHANT — item list refresh; merchant variant leads with int meso
		{
			auto hm = UI::get().get_element<UIHiredMerchant>();

			if (hm && hm->is_active())
			{
				int32_t meso = recv.read_int();
				int8_t item_count = recv.read_byte();

				hm->set_meso(meso);
				hm->clear_items();

				for (int8_t i = 0; i < item_count; i++)
				{
					int16_t bundles = recv.read_short();
					recv.read_short();
					int32_t price = recv.read_int();
					int32_t itemid = ItemParser::skip_item(recv);
					hm->add_item(i, itemid, bundles, price);

					if (bundles <= 0)
						hm->set_sold_out(i);
				}

				break;
			}

			int8_t item_count = recv.read_byte();

			if (auto shop = UI::get().get_element<UIPersonalShop>())
			{
				shop->clear_items();

				for (int8_t i = 0; i < item_count; i++)
				{
					int16_t bundles = recv.read_short();
					recv.read_short();
					int32_t price = recv.read_int();
					int32_t itemid = ItemParser::skip_item(recv);
					shop->add_item(i, itemid, bundles, price);

					if (bundles <= 0)
						shop->set_sold_out(i);
				}
			}
			break;
		}

		case 0x1A: // UPDATE_PLAYERSHOP — an item sold
		{
			int8_t pos = recv.read_byte();
			recv.read_short();
			std::string buyer = recv.read_string();

			if (auto shop = UI::get().get_element<UIPersonalShop>())
				shop->set_sold_out(pos);

			chat::log("[Shop] " + buyer + " bought an item!", chat::LineType::YELLOW);
			break;
		}

		case 10: // EXIT — room left/closed
		{
			{
				auto shop = UI::get().get_element<UIPersonalShop>();
				auto hm = UI::get().get_element<UIHiredMerchant>();
				bool shop_open = shop && shop->is_active();
				bool hm_open = hm && hm->is_active();

				if (shop_open || hm_open)
				{
					if (recv.available())
					{
						int8_t vslot = recv.read_byte();

						if (vslot >= 1 && vslot <= 3)
						{
							if (shop_open)
								shop->remove_visitor(vslot);
							else
								hm->remove_visitor(vslot);

							break;
						}
					}

					if (shop_open)
						shop->deactivate();
					else
						hm->deactivate();

					break;
				}
			}

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

				bool routed = false;

				if (auto shop = UI::get().get_element<UIPersonalShop>())
				{
					if (shop->is_active())
					{
						shop->add_chat(message, static_cast<int8_t>(speaker));
						routed = true;
					}
				}

				if (!routed)
				{
					if (auto hm = UI::get().get_element<UIHiredMerchant>())
					{
						if (hm->is_active())
						{
							hm->add_chat(message, static_cast<int8_t>(speaker));
							routed = true;
						}
					}
				}

				if (!routed)
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
	// Server confirms a shop buy/sell transaction result. Cosmic's
	// Shop.buy() / Shop.sell() pick the code; surface the exact reason
	// instead of a generic "failed" so missing-meso, full-inventory and
	// slot/itemid-mismatch failures can be told apart in-game.
	void ConfirmShopTransactionHandler::handle(InPacket& recv) const
	{
		int8_t code = recv.read_byte();

		auto messenger = UI::get().get_element<UIStatusMessenger>();
		if (!messenger) return;

		switch (code)
		{
		case 0:  // buy success — silent (inventory packet handles the gain)
		case 8:  // sell / recharge success — silent
			break;
		case 2:
			messenger->show_status(Color::Name::RED, "You don't have enough mesos.");
			break;
		case 3:
			messenger->show_status(Color::Name::RED, "Your inventory is full.");
			break;
		case 5:
			messenger->show_status(Color::Name::RED, "You can't sell that item.");
			break;
		case 9:
			messenger->show_status(Color::Name::RED, "You don't have enough mesos to recharge.");
			break;
		default:
			messenger->show_status(Color::Name::RED,
				"Transaction failed (code " + std::to_string(static_cast<int>(code)) + ").");
			break;
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

				chat::log("[Duey] Package received from " + from + "!", chat::LineType::YELLOW);

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


	void EntrustedShopCheckHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x07:
		{
			int32_t permit = MiniRooms::get().get_pending_permit();

			if (permit == 0)
				break;

			UI::get().emplace<UIEnterText>(
				"Shop name:",
				[permit](const std::string& desc)
				{
					if (!desc.empty())
						CreateShopPacket(desc, permit, 5).dispatch();
				},
				30
			);
			break;
		}
		case 0x09:
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED,
					"Retrieve your stored items from Fredrick before opening a new store.");
			break;
		default:
			break;
		}
	}

	void OwlResultHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 6)
		{
			recv.read_int();
			int32_t itemid = recv.read_int();

			if (auto owl = UI::get().get_element<UIOwl>())
				owl->show_results(itemid, recv);
		}
		else if (mode == 7)
		{
			int8_t count = recv.read_byte();
			std::vector<int32_t> ids;

			for (int8_t i = 0; i < count; i++)
				ids.push_back(recv.read_int());

			if (auto owl = UI::get().get_element<UIOwl>())
				owl->set_top10(ids);
		}
	}

	void OwlMessageHandler::handle(InPacket& recv) const
	{
		int8_t msg = recv.read_byte();
		std::string text;

		switch (msg)
		{
		case 0: return;
		case 1: text = "That shop was closed."; break;
		case 2: text = "The shop is at full capacity."; break;
		case 3: text = "You cannot go to that shop right now."; break;
		case 17: text = "You may not enter that store."; break;
		case 18: text = "That store is closed or under maintenance."; break;
		default: text = "Unable to visit that shop right now. (" + std::to_string(msg) + ")"; break;
		}

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, text);
	}

	void InventoryGrowHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();
		uint8_t limit = static_cast<uint8_t>(recv.read_byte());

		InventoryType::Id id = InventoryType::by_value(type);

		if (id != InventoryType::Id::NONE)
		{
			Stage::get().get_player().get_inventory().set_slotmax(id, limit);
			chat::log("Your inventory has been expanded to " + std::to_string(limit) + " slots.", chat::LineType::YELLOW);
		}
	}

	void PoliceHandler::handle(InPacket& recv) const
	{
		std::string text;

		if (recv.available())
			text = recv.read_string();

		if (text.empty())
			text = "You have been disconnected by the server police.";

		chat::log("[Police] " + text, chat::LineType::RED);

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, text);
	}

	void MarriageNoticeHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();
		std::string name = recv.read_string();

		std::string group = type == 0 ? "[Guild]" : "[Family]";
		chat::log(group + " Congratulations on the marriage of " + name + "!", chat::LineType::PINK);
	}

	void MobCatchFailHandler::handle(InPacket& recv) const
	{
		int8_t msg = recv.read_byte();
		recv.read_int();
		recv.read_int();

		std::string text = msg == 2
			? "The monster resisted with an Elemental Rock!"
			: "The monster is too strong to be caught right now.";

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, text);
	}

	void TamingMobHandler::handle(InPacket& recv) const
	{
		recv.read_int();
		recv.read_int();
		recv.read_int();
		recv.read_int();
		recv.read_byte();
	}

	void TrockResultHandler::handle(InPacket& recv) const
	{
	}

	void LeftKnockBackHandler::handle(InPacket& recv) const
	{
		Stage::get().get_player().get_phobj().hspeed = -4.0;
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
		int8_t skin = recv.read_byte();
		recv.read_byte(); // 1
		recv.read_byte(); // 4

		HiredMerchants::get().add(owner_id, object_id, item_id, position, owner_name, description, skin);
	}

	void DestroyHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();

		HiredMerchants::get().remove(owner_id);
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

		chat::log("[HiredMerchant] " + description + " (" + std::to_string(cur_visitors) + "/" + std::to_string(max_visitors) + " visitors)", chat::LineType::YELLOW);
	}

	void FredrickMessageHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		chat::log("[Fredrick] Message (op=" + std::to_string(operation) + ")", chat::LineType::YELLOW);
	}

	void FredrickHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		// 0x23 = retrieve items, 0x24 = error
		// Full parsing would require ItemInfo deserialization
		chat::log("[Fredrick] Response (op=" + std::to_string(op) + ")", chat::LineType::YELLOW);
	}
}
