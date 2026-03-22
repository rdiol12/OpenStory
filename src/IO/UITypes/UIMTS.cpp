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
#include "UIMTS.h"

#include "UINotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"

#include "../../Data/ItemData.h"
#include "../../Graphics/Geometry.h"
#include "../../Net/Packets/MTSPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMTS::UIMTS() : UIDragElement<PosTRADE>(Point<int16_t>(400, 20))
	{
		// MTS uses a fixed-size window
		dimension = Point<int16_t>(480, 460);
		dragarea = Point<int16_t>(480, 20);

		current_tab = TAB_BUY;
		current_type = 0;
		current_page = 0;
		total_pages = 0;
		nx_prepaid = 0;
		maple_point = 0;
		selected_index = -1;

		// Tab buttons at the top
		int16_t tab_y = 25;
		buttons[BT_TAB_BUY]  = std::make_unique<AreaButton>(Point<int16_t>(10, tab_y), Point<int16_t>(80, 22));
		buttons[BT_TAB_SELL] = std::make_unique<AreaButton>(Point<int16_t>(95, tab_y), Point<int16_t>(80, 22));
		buttons[BT_TAB_CART] = std::make_unique<AreaButton>(Point<int16_t>(180, tab_y), Point<int16_t>(80, 22));

		// Action buttons at the bottom
		int16_t btn_y = dimension.y() - 35;
		buttons[BT_BUY]         = std::make_unique<AreaButton>(Point<int16_t>(10, btn_y), Point<int16_t>(60, 22));
		buttons[BT_SELL]        = std::make_unique<AreaButton>(Point<int16_t>(10, btn_y), Point<int16_t>(60, 22));
		buttons[BT_SEARCH]      = std::make_unique<AreaButton>(Point<int16_t>(270, tab_y), Point<int16_t>(60, 22));
		buttons[BT_CART_ADD]    = std::make_unique<AreaButton>(Point<int16_t>(75, btn_y), Point<int16_t>(60, 22));
		buttons[BT_CART_REMOVE] = std::make_unique<AreaButton>(Point<int16_t>(75, btn_y), Point<int16_t>(60, 22));
		buttons[BT_CANCEL_SALE] = std::make_unique<AreaButton>(Point<int16_t>(75, btn_y), Point<int16_t>(80, 22));
		buttons[BT_TRANSFER]    = std::make_unique<AreaButton>(Point<int16_t>(160, btn_y), Point<int16_t>(70, 22));
		buttons[BT_PREV_PAGE]   = std::make_unique<AreaButton>(Point<int16_t>(340, btn_y), Point<int16_t>(40, 22));
		buttons[BT_NEXT_PAGE]   = std::make_unique<AreaButton>(Point<int16_t>(385, btn_y), Point<int16_t>(40, 22));
		buttons[BT_CLOSE]       = std::make_unique<AreaButton>(Point<int16_t>(430, btn_y), Point<int16_t>(40, 22));

		// Initial visibility
		buttons[BT_SELL]->set_active(false);
		buttons[BT_CART_REMOVE]->set_active(false);
		buttons[BT_CANCEL_SALE]->set_active(false);
		buttons[BT_TRANSFER]->set_active(false);

		// Labels
		title_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Maple Trading System");
		page_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Page 1 / 1");
		cash_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::YELLOW, "NX: 0");
		status_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, "Loading...");

		// Pre-allocate draw objects
		bg = ColorBox(dimension.x(), dimension.y(), Color::Name::BLACK, 0.85f);
		titlebar = ColorBox(dimension.x(), 20, Color::Name::ENDEAVOUR, 1.0f);
		sel_highlight = ColorBox(dimension.x() - 20, 40, Color::Name::ENDEAVOUR, 0.5f);
		tab_buy = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE, "Browse");
		tab_sell = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE, "My Sales");
		tab_cart = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE, "Cart");
		tab_search = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Search");

		active = true;
	}

	void UIMTS::draw(float alpha) const
	{
		// Draw background (dark panel)
		auto pos = position;

		// Background fill
		bg.draw(DrawArgument(pos));

		// Title bar
		titlebar.draw(DrawArgument(pos));
		title_label.draw(pos + Point<int16_t>(dimension.x() / 2, 3));

		// Tab labels
		int16_t tab_y = 28;
		Color::Name buy_color = (current_tab == TAB_BUY) ? Color::Name::YELLOW : Color::Name::WHITE;
		Color::Name sell_color = (current_tab == TAB_SELL) ? Color::Name::YELLOW : Color::Name::WHITE;
		Color::Name cart_color = (current_tab == TAB_CART) ? Color::Name::YELLOW : Color::Name::WHITE;

		tab_buy.change_color(buy_color);
		tab_sell.change_color(sell_color);
		tab_cart.change_color(cart_color);
		tab_buy.draw(pos + Point<int16_t>(50, tab_y));
		tab_sell.draw(pos + Point<int16_t>(135, tab_y));
		tab_cart.draw(pos + Point<int16_t>(220, tab_y));
		tab_search.draw(pos + Point<int16_t>(300, tab_y));

		// Cash display
		cash_label.draw(pos + Point<int16_t>(dimension.x() - 10, 28));

		// Draw separator
		int16_t list_y = 52;

		// Item listings
		const std::vector<MTSItem>* items = &listings;
		if (current_tab == TAB_SELL)
			items = &my_listings;
		else if (current_tab == TAB_CART)
			items = &listings; // cart items come through listings

		int16_t item_h = 42;
		for (size_t i = 0; i < items->size() && i < ITEMS_PER_PAGE; i++)
		{
			const MTSItem& item = (*items)[i];
			if (item.empty())
				continue;

			int16_t y = list_y + static_cast<int16_t>(i) * item_h;
			Point<int16_t> item_pos = pos + Point<int16_t>(10, y);

			// Highlight selected
			if (static_cast<int8_t>(i) == selected_index)
				sel_highlight.draw(DrawArgument(item_pos));

			// Item icon
			if (item.icon.is_valid())
				item.icon.draw(DrawArgument(item_pos + Point<int16_t>(4, item_h - 6)));

			// Item name
			item.name_label.draw(item_pos + Point<int16_t>(40, 4));

			// Price
			item.price_label.draw(item_pos + Point<int16_t>(40, 18));

			// Seller
			item.seller_label.draw(item_pos + Point<int16_t>(dimension.x() - 120, 4));
		}

		if (items->empty())
		{
			status_label.draw(pos + Point<int16_t>(dimension.x() / 2, list_y + 60));
		}

		// Page info
		page_label.draw(pos + Point<int16_t>(dimension.x() / 2, dimension.y() - 52));

		// Button labels
		int16_t btn_y = dimension.y() - 32;
		if (current_tab == TAB_BUY)
		{
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Buy").draw(pos + Point<int16_t>(40, btn_y));
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Add Cart").draw(pos + Point<int16_t>(105, btn_y));
		}
		else if (current_tab == TAB_SELL)
		{
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Cancel Sale").draw(pos + Point<int16_t>(55, btn_y));
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Transfer").draw(pos + Point<int16_t>(195, btn_y));
		}
		else if (current_tab == TAB_CART)
		{
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Buy").draw(pos + Point<int16_t>(40, btn_y));
			Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Remove").draw(pos + Point<int16_t>(105, btn_y));
		}

		Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "<").draw(pos + Point<int16_t>(360, btn_y));
		Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, ">").draw(pos + Point<int16_t>(405, btn_y));
		Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Close").draw(pos + Point<int16_t>(450, btn_y));

		UIElement::draw(alpha);
	}

	void UIMTS::update()
	{
		UIElement::update();

		std::string cash_str = "NX: " + std::to_string(nx_prepaid);
		cash_label.change_text(cash_str);

		std::string page_str = "Page " + std::to_string(current_page + 1) + " / " + std::to_string(std::max(1, total_pages));
		page_label.change_text(page_str);
	}

	Button::State UIMTS::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_TAB_BUY:
			change_tab(TAB_BUY);
			return Button::State::NORMAL;

		case BT_TAB_SELL:
			change_tab(TAB_SELL);
			return Button::State::NORMAL;

		case BT_TAB_CART:
			change_tab(TAB_CART);
			return Button::State::NORMAL;

		case BT_BUY:
			if (selected_index >= 0 && selected_index < static_cast<int8_t>(listings.size()))
			{
				int32_t id = listings[selected_index].id;
				if (current_tab == TAB_CART)
					MTSBuyFromCartPacket(id).dispatch();
				else
					MTSBuyPacket(id).dispatch();
			}
			return Button::State::NORMAL;

		case BT_CART_ADD:
			if (selected_index >= 0 && selected_index < static_cast<int8_t>(listings.size()))
			{
				MTSAddCartPacket(listings[selected_index].id).dispatch();
			}
			return Button::State::NORMAL;

		case BT_CART_REMOVE:
			if (selected_index >= 0 && selected_index < static_cast<int8_t>(listings.size()))
			{
				MTSRemoveCartPacket(listings[selected_index].id).dispatch();
			}
			return Button::State::NORMAL;

		case BT_CANCEL_SALE:
			if (selected_index >= 0 && selected_index < static_cast<int8_t>(my_listings.size()))
			{
				MTSCancelSalePacket(my_listings[selected_index].id).dispatch();
			}
			return Button::State::NORMAL;

		case BT_TRANSFER:
			if (selected_index >= 0 && selected_index < static_cast<int8_t>(transfer_items.size()))
			{
				MTSTransferPacket(transfer_items[selected_index].id).dispatch();
			}
			return Button::State::NORMAL;

		case BT_PREV_PAGE:
			if (current_page > 0)
				change_page(current_page - 1);
			return Button::State::NORMAL;

		case BT_NEXT_PAGE:
			if (current_page < total_pages - 1)
				change_page(current_page + 1);
			return Button::State::NORMAL;

		case BT_SEARCH:
			UI::get().emplace<UIEnterText>(
				"Enter search keyword:",
				[this](const std::string& search)
				{
					MTSSearchPacket(current_tab, current_type, 0, search).dispatch();
				}
			);
			return Button::State::NORMAL;

		case BT_CLOSE:
			deactivate();
			return Button::State::NORMAL;

		case BT_SELL:
			UI::get().emplace<UIOk>(
				"Drag an item from your inventory to the MTS window to sell it.",
				[](bool) {}
			);
			return Button::State::NORMAL;

		default:
			return Button::State::NORMAL;
		}
	}

	Cursor::State UIMTS::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked)
		{
			Point<int16_t> cursoroff = cursorpos - position;
			int16_t list_y = 52;
			int16_t item_h = 42;

			// Check if clicking on an item in the list
			if (cursoroff.x() >= 10 && cursoroff.x() <= dimension.x() - 10)
			{
				int16_t rel_y = cursoroff.y() - list_y;
				if (rel_y >= 0)
				{
					int8_t index = static_cast<int8_t>(rel_y / item_h);
					if (index >= 0 && index < ITEMS_PER_PAGE)
					{
						const auto* items = &listings;
						if (current_tab == TAB_SELL)
							items = &my_listings;

						if (index < static_cast<int8_t>(items->size()))
						{
							selected_index = index;
							return Cursor::State::CLICKING;
						}
					}
				}
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIMTS::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIMTS::get_type() const
	{
		return TYPE;
	}

	void UIMTS::change_tab(int32_t new_tab)
	{
		current_tab = new_tab;
		current_page = 0;
		selected_index = -1;

		// Show/hide appropriate buttons
		buttons[BT_BUY]->set_active(new_tab == TAB_BUY || new_tab == TAB_CART);
		buttons[BT_SELL]->set_active(false); // sell is initiated from inventory
		buttons[BT_CART_ADD]->set_active(new_tab == TAB_BUY);
		buttons[BT_CART_REMOVE]->set_active(new_tab == TAB_CART);
		buttons[BT_CANCEL_SALE]->set_active(new_tab == TAB_SELL);
		buttons[BT_TRANSFER]->set_active(new_tab == TAB_SELL);

		// Request page from server
		MTSChangePagePacket(new_tab, current_type, 0).dispatch();
	}

	void UIMTS::change_page(int32_t new_page)
	{
		current_page = new_page;
		selected_index = -1;
		MTSChangePagePacket(current_tab, current_type, new_page).dispatch();
	}

	void UIMTS::set_items(int32_t total_items, int32_t tab, int32_t type, int32_t page, int32_t pages)
	{
		current_tab = tab;
		current_type = type;
		current_page = page;
		total_pages = pages;
		listings.clear();
		selected_index = -1;

		status_label.change_text(total_items > 0 ? "" : "No items found.");
	}

	void UIMTS::add_listing(int32_t id, int32_t itemid, int16_t quantity,
		int32_t taxes, int32_t price, const std::string& seller, bool is_equip)
	{
		MTSItem item;
		item.id = id;
		item.itemid = itemid;
		item.quantity = quantity;
		item.taxes = taxes;
		item.price = price;
		item.seller = seller;
		item.is_equip = is_equip;

		const ItemData& idata = ItemData::get(itemid);
		item.icon = idata.get_icon(false);

		std::string name = idata.get_name();
		if (name.empty())
			name = "Item #" + std::to_string(itemid);
		if (quantity > 1)
			name += " x" + std::to_string(quantity);

		item.name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);

		std::string price_str = std::to_string(price) + " NX";
		item.price_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, price_str);
		item.seller_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, seller);

		listings.push_back(std::move(item));
	}

	void UIMTS::finish_listings()
	{
		if (listings.empty())
			status_label.change_text("No items found.");
		else
			status_label.change_text("");
	}

	void UIMTS::set_cash(int32_t nx, int32_t mp)
	{
		nx_prepaid = nx;
		maple_point = mp;
	}

	void UIMTS::confirm_sell()
	{
		status_label.change_text("Item listed successfully!");
	}

	void UIMTS::confirm_buy()
	{
		status_label.change_text("Item purchased!");
		selected_index = -1;
	}

	void UIMTS::fail_buy()
	{
		status_label.change_text("Purchase failed. Not enough NX?");
	}

	void UIMTS::confirm_transfer(int16_t quantity, int16_t pos)
	{
		status_label.change_text("Item transferred to inventory!");
	}

	void UIMTS::set_not_yet_sold(int32_t count)
	{
		my_listings.clear();
	}

	void UIMTS::add_not_yet_sold_item(int32_t id, int32_t itemid, int16_t quantity,
		int32_t price, const std::string& seller, bool is_equip)
	{
		MTSItem item;
		item.id = id;
		item.itemid = itemid;
		item.quantity = quantity;
		item.price = price;
		item.seller = seller;
		item.is_equip = is_equip;

		const ItemData& idata = ItemData::get(itemid);
		item.icon = idata.get_icon(false);

		std::string name = idata.get_name();
		if (name.empty())
			name = "Item #" + std::to_string(itemid);
		if (quantity > 1)
			name += " x" + std::to_string(quantity);

		item.name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);

		std::string price_str = std::to_string(price) + " NX";
		item.price_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, price_str);
		item.seller_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, seller);

		my_listings.push_back(std::move(item));
	}

	void UIMTS::set_transfer_inv(int32_t count)
	{
		transfer_items.clear();
	}

	void UIMTS::add_transfer_item(int32_t id, int32_t itemid, int16_t quantity,
		int32_t price, const std::string& seller, bool is_equip)
	{
		MTSItem item;
		item.id = id;
		item.itemid = itemid;
		item.quantity = quantity;
		item.price = price;
		item.seller = seller;
		item.is_equip = is_equip;

		const ItemData& idata = ItemData::get(itemid);
		item.icon = idata.get_icon(false);

		std::string name = idata.get_name();
		if (name.empty())
			name = "Item #" + std::to_string(itemid);

		item.name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);
		item.price_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Transfer");
		item.seller_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "");

		transfer_items.push_back(std::move(item));
	}
}
