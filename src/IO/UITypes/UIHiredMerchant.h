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
#include "../Components/Slider.h"
#include "../Components/Textfield.h"

#include "../../Character/Look/CharLook.h"
#include "../../Graphics/Animation.h"
#include "../../Net/Login.h"

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

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
		bool send_icon(const Icon& icon, Point<int16_t> cursorpos) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_owner(const std::string& name);
		void set_mode(bool owner, bool first_time = false);
		void set_meso(int64_t meso);
		void set_merchant(int32_t item_id);
		void doubleclick(Point<int16_t> cursorpos) override;
		void set_slot_look(int8_t slot, const LookEntry& entry, const std::string& name);
		void remove_visitor(int8_t slot);
		void add_chat(const std::string& line, int8_t speaker = 0);
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
			BT_ARRANGE,
			BT_ENTER,
			BT_EXIT,
			BT_COIN,
			BT_OPEN
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
		CharLook slot_looks[4];
		bool slot_used[4] = { false, false, false, false };
		std::string slot_names[4];
		std::vector<Text> chat_lines;
		Textfield chat_field;
		Slider chat_slider;
		int16_t chat_offset = 0;
		void rebuild_chat_slider();
		Slider item_slider;
		int16_t item_offset = 0;
		void rebuild_item_slider();
		mutable Text slot_name_text;
		mutable Text meso_label;
		Animation employee;
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
