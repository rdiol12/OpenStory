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
#include "MTSHandlers.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UIMTS.h"

namespace ms
{
	// Skip an item's serialized data from the packet (addItemInfo with zeroPosition=true)
	// Returns: itemid, quantity, and whether it's an equip
	static void skip_item_info(InPacket& recv, int32_t& out_itemid, int16_t& out_quantity, bool& out_is_equip)
	{
		int8_t item_type = recv.read_byte(); // 1=equip, 2=use, etc
		out_itemid = recv.read_int();
		out_is_equip = (item_type == 1);

		bool is_cash = recv.read_bool();
		if (is_cash)
			recv.skip(8); // cash id / pet id / ring id

		recv.skip(8); // expiration time

		if (item_type == 1)
		{
			// Equip
			out_quantity = 1;
			recv.skip_byte();  // upgrade slots
			recv.skip_byte();  // level
			recv.skip(15 * 2); // 15 stats (str,dex,int,luk,hp,mp,watk,matk,wdef,mdef,acc,avoid,hands,speed,jump)
			recv.skip_string(); // owner
			recv.skip_short();  // flag

			if (is_cash)
			{
				recv.skip(10); // cash equip padding
			}
			else
			{
				recv.skip_byte();  // 0
				recv.skip_byte();  // item level
				recv.skip_int();   // exp nibble
				recv.skip_int();   // vicious
				recv.skip(8);      // padding
			}

			recv.skip(8); // time
			recv.skip_int(); // -1
		}
		else
		{
			// Regular item
			out_quantity = recv.read_short();
			recv.skip_string(); // owner
			recv.skip_short();  // flag

			// Check rechargeable (stars, bullets)
			if ((out_itemid / 10000) == 207 || (out_itemid / 10000) == 233)
			{
				recv.skip(8); // rechargeable data
			}
		}
	}

	// Parse an MTS listing entry (item info + listing metadata)
	static void parse_mts_listing(InPacket& recv,
		int32_t& id, int32_t& itemid, int16_t& quantity,
		int32_t& taxes, int32_t& price, std::string& seller, bool& is_equip)
	{
		skip_item_info(recv, itemid, quantity, is_equip);

		id = recv.read_int();       // listing id
		taxes = recv.read_int();    // taxes
		price = recv.read_int();    // base price
		recv.skip_int();            // 0
		recv.skip(8);               // ending date (long)
		seller = recv.read_string();// account name
		recv.skip_string();         // char name (same as seller usually)
		recv.skip(28);              // padding
	}

	void MTSOperationHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();

		switch (operation)
		{
		case 0x15: // sendMTS — item listing page
		{
			int32_t total_items = recv.read_int();
			int32_t num_items = recv.read_int();
			int32_t tab = recv.read_int();
			int32_t type = recv.read_int();
			int32_t page = recv.read_int();
			recv.skip_byte(); // 1
			recv.skip_byte(); // 1

			// Create or get MTS UI
			UI::get().emplace<UIMTS>();
			auto mts = UI::get().get_element<UIMTS>();

			if (mts)
			{
				int32_t pages = (total_items > 0) ? ((total_items + 15) / 16) : 1;
				mts->set_items(total_items, tab, type, page, pages);

				for (int32_t i = 0; i < num_items; i++)
				{
					int32_t id, itemid, taxes, price;
					int16_t quantity;
					std::string seller;
					bool is_equip;

					parse_mts_listing(recv, id, itemid, quantity, taxes, price, seller, is_equip);
					mts->add_listing(id, itemid, quantity, taxes, price, seller, is_equip);
				}

				mts->finish_listings();
			}

			recv.skip_byte(); // trailing 1
			break;
		}

		case 0x1D: // MTSConfirmSell
		{
			if (auto mts = UI::get().get_element<UIMTS>())
				mts->confirm_sell();
			break;
		}

		case 0x21: // transferInventory
		{
			int32_t count = recv.read_int();

			if (auto mts = UI::get().get_element<UIMTS>())
			{
				mts->set_transfer_inv(count);

				for (int32_t i = 0; i < count; i++)
				{
					int32_t id, itemid, taxes, price;
					int16_t quantity;
					std::string seller;
					bool is_equip;

					parse_mts_listing(recv, id, itemid, quantity, taxes, price, seller, is_equip);
					mts->add_transfer_item(id, itemid, quantity, price, seller, is_equip);
				}
			}

			// Skip any trailing bytes
			while (recv.available())
				recv.skip_byte();

			break;
		}

		case 0x23: // notYetSoldInv
		{
			int32_t count = recv.read_int();

			if (auto mts = UI::get().get_element<UIMTS>())
			{
				mts->set_not_yet_sold(count);

				if (count > 0)
				{
					for (int32_t i = 0; i < count; i++)
					{
						int32_t id, itemid, taxes, price;
						int16_t quantity;
						std::string seller;
						bool is_equip;

						parse_mts_listing(recv, id, itemid, quantity, taxes, price, seller, is_equip);
						mts->add_not_yet_sold_item(id, itemid, quantity, price, seller, is_equip);
					}
				}
				else
				{
					recv.skip_int(); // 0
				}
			}
			break;
		}

		case 0x27: // MTSConfirmTransfer
		{
			int32_t quantity = recv.read_int();
			int32_t pos = recv.read_int();

			if (auto mts = UI::get().get_element<UIMTS>())
				mts->confirm_transfer(static_cast<int16_t>(quantity), static_cast<int16_t>(pos));
			break;
		}

		case 0x33: // MTSConfirmBuy
		{
			if (auto mts = UI::get().get_element<UIMTS>())
				mts->confirm_buy();
			break;
		}

		case 0x34: // MTSFailBuy
		{
			recv.skip_byte(); // 0x42
			if (auto mts = UI::get().get_element<UIMTS>())
				mts->fail_buy();
			break;
		}

		case 0x3D: // MTSWantedListingOver
		{
			recv.skip_int(); // nx
			recv.skip_int(); // items
			break;
		}

		default:
			break;
		}
	}

	void MTSCashHandler::handle(InPacket& recv) const
	{
		// MTS_OPERATION2 — cash balance
		int32_t nx_prepaid = recv.read_int();
		int32_t maple_point = recv.read_int();

		if (auto mts = UI::get().get_element<UIMTS>())
			mts->set_cash(nx_prepaid, maple_point);
	}
}
