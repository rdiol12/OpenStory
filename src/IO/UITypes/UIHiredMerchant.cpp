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
#include "UIHiredMerchant.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIHiredMerchant::UIHiredMerchant() : UIDragElement<PosHIREDMERCHANT>(),
		stored_meso(0), item_count(0), selected_slot(-1)
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
		buttons[Buttons::BT_ARRANGE] = std::make_unique<MapleButton>(src["BtArrange"]);
		buttons[Buttons::BT_VISIT] = std::make_unique<MapleButton>(src["BtVisit"]);
		buttons[Buttons::BT_COIN] = std::make_unique<MapleButton>(src["BtCoin"]);
		buttons[Buttons::BT_EXIT] = std::make_unique<MapleButton>(src["BtExit"]);

		select_tex = src["select"];

		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0 };

		owner_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "");
		meso_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Meso: 0");
		item_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		price_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::BLUE);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIHiredMerchant::draw(float inter) const
	{
		UIElement::draw(inter);

		owner_label.draw(position + Point<int16_t>(dimension.x() / 2, 22));
		meso_label.draw(position + Point<int16_t>(15, dimension.y() - 30));

		// Draw item list
		for (int8_t i = 0; i < item_count; i++)
		{
			int16_t row_y = 50 + i * 36;

			if (i == selected_slot)
				select_tex.draw(position + Point<int16_t>(10, row_y));

			if (items[i].itemid > 0)
			{
				item_label.change_text("Item #" + std::to_string(items[i].itemid) + " x" + std::to_string(items[i].quantity));
				item_label.draw(position + Point<int16_t>(45, row_y + 8));

				price_label.change_text(std::to_string(items[i].price) + " meso");
				price_label.draw(position + Point<int16_t>(dimension.x() - 20, row_y + 8));
			}
		}
	}

	void UIHiredMerchant::update()
	{
		UIElement::update();
	}

	void UIHiredMerchant::set_owner(const std::string& name)
	{
		owner_name = name;
		owner_label.change_text(name + "'s Shop");
	}

	void UIHiredMerchant::set_meso(int64_t meso)
	{
		stored_meso = meso;
		meso_label.change_text("Meso: " + std::to_string(meso));
	}

	void UIHiredMerchant::add_item(int8_t slot, int32_t itemid, int16_t quantity, int32_t price)
	{
		if (slot >= 0 && slot < MAX_ITEMS)
		{
			items[slot] = { itemid, quantity, price };

			if (slot >= item_count)
				item_count = slot + 1;
		}
	}

	void UIHiredMerchant::clear_items()
	{
		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0 };

		item_count = 0;
		selected_slot = -1;
	}

	void UIHiredMerchant::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			TradeExitPacket().dispatch();
			deactivate();
		}
	}

	UIElement::Type UIHiredMerchant::get_type() const
	{
		return TYPE;
	}

	Button::State UIHiredMerchant::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
		case Buttons::BT_EXIT:
			TradeExitPacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_BUY:
			if (selected_slot >= 0)
				ShopBuyPacket(selected_slot, items[selected_slot].quantity).dispatch();
			break;
		case Buttons::BT_ARRANGE:
			ShopArrangePacket().dispatch();
			break;
		case Buttons::BT_VISIT:
			break;
		case Buttons::BT_COIN:
			ShopTakeMesoPacket().dispatch();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
