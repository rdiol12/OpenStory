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

#include "../../IO/UI.h"
#include "../../IO/UITypes/UICharInfo.h"

namespace ms
{
	static std::vector<BotItem> parse_bot_items(InPacket& recv)
	{
		std::vector<BotItem> items;
		int16_t count = recv.read_short();

		for (int16_t i = 0; i < count; i++)
		{
			BotItem bi;
			bi.slot = recv.read_short();
			bi.item_id = recv.read_int();
			bi.count = recv.read_short();

			int8_t item_type = recv.read_byte();

			if (item_type == 1)
			{
				// Equip: 14 stat shorts + 2 bytes
				recv.skip(14 * 2); // str,dex,int,luk,hp,mp,watk,matk,wdef,mdef,acc,avoid,speed,jump
				recv.read_byte();  // upgradeSlots
				recv.read_byte();  // level
			}

			items.push_back(bi);
		}

		return items;
	}

	void BotInventoryHandler::handle(InPacket& recv) const
	{
		BotInventoryData data;

		data.char_id = recv.read_int();
		data.name = recv.read_string();
		data.level = recv.read_int();
		data.meso = recv.read_int();

		// 5 groups: EQUIP, USE, SETUP, ETC, EQUIPPED
		data.equip = parse_bot_items(recv);
		data.use = parse_bot_items(recv);
		data.setup = parse_bot_items(recv);
		data.etc = parse_bot_items(recv);
		data.equipped = parse_bot_items(recv);

		// Debug log
		static FILE* dbg = fopen("bot_inventory_debug.txt", "a");
		if (dbg)
		{
			fprintf(dbg, "BOT_INV: id=%d name=%s lv=%d meso=%d equip=%zu use=%zu setup=%zu etc=%zu equipped=%zu\n",
				data.char_id, data.name.c_str(), data.level, data.meso,
				data.equip.size(), data.use.size(), data.setup.size(), data.etc.size(), data.equipped.size());

			auto log_items = [&](const char* name, const std::vector<BotItem>& items) {
				for (size_t i = 0; i < items.size(); i++)
					fprintf(dbg, "  %s[%zu]: slot=%d itemid=%d count=%d\n", name, i, items[i].slot, items[i].item_id, items[i].count);
			};
			log_items("equip", data.equip);
			log_items("use", data.use);
			log_items("setup", data.setup);
			log_items("etc", data.etc);
			log_items("equipped", data.equipped);
			fflush(dbg);
		}

		// Create charinfo window if it doesn't exist
		if (!UI::get().get_element<UICharInfo>())
			UI::get().emplace<UICharInfo>(data.char_id);

		if (auto charinfo = UI::get().get_element<UICharInfo>())
		{
			if (!charinfo->is_active())
				charinfo->toggle_active();

			charinfo->set_bot_inventory(std::move(data));

			if (dbg)
			{
				fprintf(dbg, "  -> set_bot_inventory called, is_valid=%d\n", charinfo->get_char_id());
				fflush(dbg);
			}
		}
		else if (dbg)
		{
			fprintf(dbg, "  -> UICharInfo NOT FOUND\n");
			fflush(dbg);
		}
	}
}
