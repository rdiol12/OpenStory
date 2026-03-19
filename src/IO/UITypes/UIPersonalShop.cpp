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
#include "UIPersonalShop.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPersonalShop::UIPersonalShop() : UIDragElement<PosPERSONALSHOP>(),
		item_count(0), selected_slot(-1), is_owner(false)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["PersonalShop"]["main"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = src["backgrnd3"];

		if (backgrnd3.size() > 0)
			sprites.emplace_back(backgrnd3);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"]);
		buttons[Buttons::BT_BUY] = std::make_unique<MapleButton>(src["BtBuy"]);
		buttons[Buttons::BT_START] = std::make_unique<MapleButton>(src["BtStart"]);
		buttons[Buttons::BT_ENTER] = std::make_unique<MapleButton>(src["BtEnter"]);
		buttons[Buttons::BT_EXIT] = std::make_unique<MapleButton>(src["BtExit"]);
		buttons[Buttons::BT_ITEM] = std::make_unique<MapleButton>(src["BtItem"]);
		buttons[Buttons::BT_INFO] = std::make_unique<MapleButton>(src["BtInfo"]);
		buttons[Buttons::BT_COIN] = std::make_unique<MapleButton>(src["BtCoin"]);
		buttons[Buttons::BT_BAN] = std::make_unique<MapleButton>(src["BtBan"]);
		buttons[Buttons::BT_BLACKLIST] = std::make_unique<MapleButton>(src["BtBlackList"]);

		select_tex = src["select"];
		soldout_tex = src["soldout"];

		// Visitor buttons initially hidden; owner buttons initially hidden
		buttons[Buttons::BT_BUY]->set_active(false);
		buttons[Buttons::BT_START]->set_active(false);
		buttons[Buttons::BT_BAN]->set_active(false);
		buttons[Buttons::BT_BLACKLIST]->set_active(false);

		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0, false };

		owner_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "");
		item_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		price_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::BLUE);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIPersonalShop::draw(float inter) const
	{
		UIElement::draw(inter);

		owner_label.draw(position + Point<int16_t>(dimension.x() / 2, 22));

		for (int8_t i = 0; i < item_count; i++)
		{
			int16_t row_y = 50 + i * 36;

			if (i == selected_slot)
				select_tex.draw(position + Point<int16_t>(10, row_y));

			if (items[i].itemid > 0)
			{
				if (items[i].sold_out)
				{
					soldout_tex.draw(position + Point<int16_t>(10, row_y));
				}
				else
				{
					item_label.change_text("Item #" + std::to_string(items[i].itemid) + " x" + std::to_string(items[i].quantity));
					item_label.draw(position + Point<int16_t>(45, row_y + 8));

					price_label.change_text(std::to_string(items[i].price) + " meso");
					price_label.draw(position + Point<int16_t>(dimension.x() - 20, row_y + 8));
				}
			}
		}
	}

	void UIPersonalShop::update()
	{
		UIElement::update();
	}

	void UIPersonalShop::set_owner(const std::string& name)
	{
		owner_name = name;
		owner_label.change_text(name + "'s Shop");
	}

	void UIPersonalShop::add_item(int8_t slot, int32_t itemid, int16_t quantity, int32_t price)
	{
		if (slot >= 0 && slot < MAX_ITEMS)
		{
			items[slot] = { itemid, quantity, price, false };

			if (slot >= item_count)
				item_count = slot + 1;
		}
	}

	void UIPersonalShop::set_sold_out(int8_t slot)
	{
		if (slot >= 0 && slot < MAX_ITEMS)
			items[slot].sold_out = true;
	}

	void UIPersonalShop::clear_items()
	{
		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0, false };

		item_count = 0;
		selected_slot = -1;
	}

	void UIPersonalShop::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			TradeExitPacket().dispatch();
			deactivate();
		}
	}

	UIElement::Type UIPersonalShop::get_type() const
	{
		return TYPE;
	}

	Button::State UIPersonalShop::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
		case Buttons::BT_EXIT:
			TradeExitPacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_BUY:
			if (selected_slot >= 0 && !items[selected_slot].sold_out)
				ShopBuyPacket(selected_slot, items[selected_slot].quantity).dispatch();
			break;
		case Buttons::BT_START:
			ShopOpenPacket().dispatch();
			break;
		case Buttons::BT_ENTER:
			break;
		case Buttons::BT_ITEM:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line("Use the inventory to add items.", UIChatBar::YELLOW);
			break;
		case Buttons::BT_INFO:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line(owner_name + "'s Shop - " + std::to_string(item_count) + " item(s)", UIChatBar::YELLOW);
			break;
		case Buttons::BT_COIN:
			ShopTakeMesoPacket().dispatch();
			break;
		case Buttons::BT_BAN:
			ShopBanVisitorPacket().dispatch();
			break;
		case Buttons::BT_BLACKLIST:
			ShopBlacklistPacket().dispatch();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
