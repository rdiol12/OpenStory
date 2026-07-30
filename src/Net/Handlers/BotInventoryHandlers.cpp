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
#include "BotInventoryHandlers.h"

#include "Helpers/ItemParser.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UICharInfo.h"

namespace ms
{
	// One inventory block of Cosmic's botEquipSnapshot: byte count, then per item
	// a short slot followed by the STANDARD item serialization (addItemInfo).
	//
	static std::vector<BotItem> parse_bot_items(InPacket& recv, int32_t count)
	{
		std::vector<BotItem> items;
		items.reserve(count > 0 ? count : 0);

		for (int32_t i = 0; i < count && recv.available(); i++)
		{
			BotItem bi;
			bi.slot = recv.read_short();

			ItemParser::SkimmedItem it = ItemParser::skim_item(recv);
			bi.item_id = it.itemid;
			bi.count = it.count;

			items.push_back(bi);
		}

		return items;
	}

	void BotInventoryHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t resp = recv.read_byte();

		if (resp != 1)
			return;

		BotInventoryData data;

		int8_t bot_index = recv.read_byte();

		data.char_id = bot_index;
		data.name = "Bot " + std::to_string(static_cast<int>(bot_index));

		data.equipped = parse_bot_items(recv, recv.read_byte());

		int8_t tab_count = recv.available() ? recv.read_byte() : 0;

		for (int8_t t = 0; t < tab_count && recv.available(); t++)
		{
			int8_t tab_id = recv.read_byte();
			std::vector<BotItem> items = parse_bot_items(recv, recv.read_byte());

			switch (tab_id)
			{
			case 1: data.equip = std::move(items); break;
			case 2: data.use = std::move(items); break;
			case 3: data.setup = std::move(items); break;
			case 4: data.etc = std::move(items); break;
			default: break;   // 5 = CASH, which the window has no tab for
			}
		}

		if (recv.length() >= 4)
			data.meso = recv.read_int();

		// Trailing stack counts: short n, then (byte tab, short slot, short qty).
		if (recv.length() >= 2)
		{
			int16_t qty_count = recv.read_short();

			for (int16_t i = 0; i < qty_count && recv.length() >= 5; i++)
			{
				int8_t tab = recv.read_byte();
				int16_t slot = recv.read_short();
				int16_t qty = recv.read_short();

				std::vector<BotItem>* list = nullptr;

				switch (tab)
				{
				case 1: list = &data.equip; break;
				case 2: list = &data.use; break;
				case 3: list = &data.setup; break;
				case 4: list = &data.etc; break;
				default: break;
				}

				if (list == nullptr)
					continue;

				for (BotItem& bi : *list)
					if (bi.slot == slot)
						bi.count = qty;
			}
		}

		// Create charinfo window if it doesn't exist
		if (!UI::get().get_element<UICharInfo>())
			UI::get().emplace<UICharInfo>(data.char_id);

		if (auto charinfo = UI::get().get_element<UICharInfo>())
		{
			if (!charinfo->is_active())
				charinfo->toggle_active();

			charinfo->set_bot_inventory(std::move(data));
		}
	}
}
