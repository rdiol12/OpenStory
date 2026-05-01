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
#include "NpcInteractionHandlers.h"

#include "../../IO/UI.h"

#include "../../IO/UITypes/UINpcTalk.h"
#include "../../IO/UITypes/UIShop.h"

#include "../NpcResponseTracker.h"

#include <utility>
#include <vector>

namespace ms
{
	void NpcDialogueHandler::handle(InPacket& recv) const
	{
		recv.skip(1);

		int32_t npcid = recv.read_int();

		// Server responded — drop the stale-response timer for this NPC.
		NpcResponseTracker::get().clear_pending(npcid);

		int8_t msgtype = recv.read_byte();
		int8_t speaker = recv.read_byte();

		std::string text = recv.read_string();

		// `style` is unused by the new layout but kept on the
		// change_text signature for interface stability.
		int16_t style = 0;

		// Trailing payload: msgtype 3 (sendGetNumber) carries default,
		// min, max as three ints (12 bytes total).
		int32_t num_def = 0, num_min = 0, num_max = 0;

		if (msgtype == 3 && recv.length() >= 12)
		{
			num_def = recv.read_int();
			num_min = recv.read_int();
			num_max = recv.read_int();
		}

		// Force-remove any stale NpcTalk element so emplace always creates a
		// fresh one. UIStateGame::remove uses unique_ptr::release() which
		// leaves a null entry behind; that can occasionally wedge subsequent
		// dialog opens when the server sends a new dialogue before the old
		// one's state has fully cleared.
		UI::get().remove(UIElement::Type::NPCTALK);
		UI::get().emplace<UINpcTalk>();
		UI::get().enable();

		if (auto npctalk = UI::get().get_element<UINpcTalk>())
		{
			if (msgtype == 3)
				npctalk->set_number_bounds(num_def, num_min, num_max);

			npctalk->change_text(npcid, msgtype, style, speaker, text);
		}
	}

	void OpenNpcShopHandler::handle(InPacket& recv) const
	{
		int32_t npcid = recv.read_int();

		// Server responded (shop opened) — drop the stale-response timer.
		NpcResponseTracker::get().clear_pending(npcid);

		auto oshop = UI::get().get_element<UIShop>();

		if (!oshop)
			return;

		UIShop& shop = *oshop;

		shop.reset(npcid);

		int16_t size = recv.read_short();

		for (int16_t i = 0; i < size; i++)
		{
			int32_t itemid = recv.read_int();
			int32_t price = recv.read_int();
			int32_t pitch = recv.read_int();
			int32_t time = recv.read_int();

			recv.skip(4);

			bool norecharge = recv.read_short() == 1;

			if (norecharge)
			{
				int16_t buyable = recv.read_short();

				shop.add_item(itemid, price, pitch, time, buyable);
			}
			else
			{
				recv.skip(4);

				int16_t rechargeprice = recv.read_short();
				int16_t slotmax = recv.read_short();

				shop.add_rechargable(itemid, price, pitch, time, rechargeprice, slotmax);
			}
		}

		// All items loaded — now we know which categories this shop
		// carries, so build the buy-side tabs accordingly.
		shop.finalize_buy_tabs();
	}
}