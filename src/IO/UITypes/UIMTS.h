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
#pragma once

#include "../UIDragElement.h"
#include "../../Graphics/Text.h"
#include "../../Data/ItemData.h"

namespace ms
{
	class UIMTS : public UIDragElement<PosTRADE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MTS;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		UIMTS();

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> position) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called by handlers
		void set_items(int32_t total_items, int32_t tab, int32_t type, int32_t page, int32_t pages);
		void add_listing(int32_t id, int32_t itemid, int16_t quantity,
			int32_t taxes, int32_t price, const std::string& seller, bool is_equip);
		void finish_listings();
		void set_cash(int32_t nx_prepaid, int32_t maple_point);
		void confirm_sell();
		void confirm_buy();
		void fail_buy();
		void confirm_transfer(int16_t quantity, int16_t pos);
		void set_not_yet_sold(int32_t count);
		void add_not_yet_sold_item(int32_t id, int32_t itemid, int16_t quantity,
			int32_t price, const std::string& seller, bool is_equip);
		void set_transfer_inv(int32_t count);
		void add_transfer_item(int32_t id, int32_t itemid, int16_t quantity,
			int32_t price, const std::string& seller, bool is_equip);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_tab(int32_t new_tab);
		void change_page(int32_t new_page);

		enum Buttons : uint16_t
		{
			BT_BUY,
			BT_SELL,
			BT_SEARCH,
			BT_CART_ADD,
			BT_CART_REMOVE,
			BT_CANCEL_SALE,
			BT_TRANSFER,
			BT_PREV_PAGE,
			BT_NEXT_PAGE,
			BT_TAB_BUY,
			BT_TAB_SELL,
			BT_TAB_CART,
			BT_CLOSE,
			NUM_BUTTONS
		};

		enum Tab : int32_t
		{
			TAB_BUY = 1,
			TAB_SELL = 2,
			TAB_CART = 4
		};

		struct MTSItem
		{
			int32_t id = 0;
			int32_t itemid = 0;
			int16_t quantity = 0;
			int32_t taxes = 0;
			int32_t price = 0;
			std::string seller;
			bool is_equip = false;
			Texture icon;
			Text name_label;
			Text price_label;
			Text seller_label;

			bool empty() const { return id == 0; }
		};

		static constexpr int ITEMS_PER_PAGE = 16;

		std::vector<MTSItem> listings;
		std::vector<MTSItem> my_listings;
		std::vector<MTSItem> transfer_items;

		int32_t current_tab;
		int32_t current_type;
		int32_t current_page;
		int32_t total_pages;

		int32_t nx_prepaid;
		int32_t maple_point;

		int8_t selected_index; // -1 = none

		Text title_label;
		Text page_label;
		Text cash_label;
		Text status_label;
	};
}
