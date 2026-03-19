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

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIPersonalShop : public UIDragElement<PosPERSONALSHOP>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PERSONALSHOP;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIPersonalShop();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_owner(const std::string& name);
		void add_item(int8_t slot, int32_t itemid, int16_t quantity, int32_t price);
		void set_sold_out(int8_t slot);
		void clear_items();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_BUY,
			BT_START,
			BT_ENTER,
			BT_EXIT,
			BT_ITEM,
			BT_INFO,
			BT_COIN,
			BT_BAN,
			BT_BLACKLIST
		};

		struct ShopItem
		{
			int32_t itemid;
			int16_t quantity;
			int32_t price;
			bool sold_out;
		};

		static constexpr int8_t MAX_ITEMS = 16;

		std::string owner_name;
		ShopItem items[MAX_ITEMS];
		int8_t item_count;
		int8_t selected_slot;
		bool is_owner;

		Texture select_tex;
		Texture soldout_tex;

		mutable Text owner_label;
		mutable Text item_label;
		mutable Text price_label;
	};
}
