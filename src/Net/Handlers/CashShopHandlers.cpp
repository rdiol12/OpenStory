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
#include "CashShopHandlers.h"

#include "Helpers/CashShopParser.h"

#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIMTS.h"
#include "../../IO/Window.h"

namespace ms
{
	void SetCashShopHandler::handle(InPacket& recv) const
	{
		CashShopParser::parseCharacterInfo(recv);

		recv.skip_byte();	// Not MTS
		recv.skip_string();	// account_name
		recv.skip_int();

		int16_t specialcashitem_size = recv.read_short();

		for (size_t i = 0; i < specialcashitem_size; i++)
		{
			recv.skip_int();	// sn
			recv.skip_int();	// mod
			recv.skip_byte();	// info
		}

		recv.skip(121);

		for (size_t cat = 1; cat <= 8; cat++)
		{
			for (size_t gender = 0; gender < 2; gender++)
			{
				for (size_t in = 0; in < 5; in++)
				{
					recv.skip_int(); // category
					recv.skip_int(); // gender
					recv.skip_int(); // commoditysn
				}
			}
		}

		recv.skip_int();
		recv.skip_short();
		recv.skip_byte();
		recv.skip_int();

		transition();

		UI::get().change_state(UI::State::CASHSHOP);
	}

	void CashShopOperationHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		switch (operation)
		{
		case 0x4A:
		{
			// Buy item success
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Cash Shop] Item purchased successfully!", UIChatBar::LineType::YELLOW);
			break;
		}
		case 0x4C:
		{
			int8_t reason = recv.read_byte();
			std::string msg;
			switch (reason)
			{
			case 0: msg = "Unknown error."; break;
			case 1: msg = "You don't have enough NX."; break;
			case 2: msg = "Item is out of stock."; break;
			case 3: msg = "Your cash inventory is full."; break;
			case 4: msg = "This item is not available for purchase."; break;
			case 5: msg = "You have exceeded the purchase limit."; break;
			case 6: msg = "You are under the required level."; break;
			case 7: msg = "You already have this item."; break;
			default: msg = "Purchase failed (code: " + std::to_string(reason) + ")"; break;
			}
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Cash Shop] " + msg, UIChatBar::LineType::RED);
			break;
		}
		case 0x59:
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Cash Shop] Coupon redeemed successfully!", UIChatBar::LineType::YELLOW);
			break;
		}
		case 0x5C:
		{
			int8_t reason = recv.read_byte();
			std::string msg;
			switch (reason)
			{
			case 0: msg = "Invalid coupon code."; break;
			case 1: msg = "This coupon has expired."; break;
			case 2: msg = "This coupon has already been used."; break;
			case 3: msg = "This coupon is for a different server."; break;
			default: msg = "Coupon error (code: " + std::to_string(reason) + ")"; break;
			}
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Cash Shop] " + msg, UIChatBar::LineType::RED);
			break;
		}
		default:
			break;
		}
	}

	void SetITCHandler::handle(InPacket& recv) const
	{
		// SET_ITC — MTS transition
		// Same as SET_CASH_SHOP but with MTS trailing data instead of CS items
		CashShopParser::parseCharacterInfo(recv);

		recv.skip_string();	// account_name

		// MTS config bytes (hardcoded by Cosmic)
		recv.skip(28);

		// Open the MTS UI
		UI::get().emplace<UIMTS>();
	}

	void SetCashShopHandler::transition() const
	{
		Constants::Constants::get().set_viewwidth(1024);
		Constants::Constants::get().set_viewheight(768);

		float fadestep = 0.025f;

		Window::get().fadeout(
			fadestep,
			[]()
			{
				GraphicsGL::get().clear();

				Stage::get().load(-1, 0);

				UI::get().enable();
				Timer::get().start();
				GraphicsGL::get().unlock();
			}
		);

		GraphicsGL::get().lock();
		Stage::get().clear();
		Timer::get().start();
	}
}