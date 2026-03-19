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
#include "../../Graphics/Texture.h"

namespace ms
{
	class UIHiredMerchant : public UIDragElement<PosHIREDMERCHANT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::HIREDMERCHANT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIHiredMerchant();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_owner(const std::string& name);
		void set_meso(int64_t meso);
		void add_item(int8_t slot, int32_t itemid, int16_t quantity, int32_t price);
		void clear_items();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_BUY,
			BT_ARRANGE,
			BT_VISIT,
			BT_COIN,
			BT_EXIT
		};

		struct ShopItem
		{
			int32_t itemid;
			int16_t quantity;
			int32_t price;
		};

		static constexpr int8_t MAX_ITEMS = 16;

		std::string owner_name;
		int64_t stored_meso;
		ShopItem items[MAX_ITEMS];
		int8_t item_count;
		int8_t selected_slot;

		mutable Text owner_label;
		mutable Text meso_label;
		mutable Text item_label;
		mutable Text price_label;

		Texture select_tex;
	};
}
