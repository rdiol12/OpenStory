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
