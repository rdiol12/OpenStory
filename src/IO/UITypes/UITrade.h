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
#include "../../Data/ItemData.h"
#include "../../Character/Inventory/Inventory.h"

namespace ms
{
	class UITrade : public UIDragElement<PosTRADE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::TRADE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		UITrade();

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> position) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called by handler to set up trade room
		void set_partner(uint8_t slot, const std::string& name);
		// Add item to a trade slot
		void set_item(uint8_t player_num, uint8_t slot, int32_t itemid, int16_t count);
		// Set meso for a player
		void set_meso(uint8_t player_num, int32_t meso);
		// Partner confirmed
		void set_partner_confirmed();
		// Trade completed or cancelled
		void trade_result(uint8_t result);
		// Chat message
		void add_chat(const std::string& message);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void cancel_trade();
		void confirm_trade();
		int8_t slot_by_position(Point<int16_t> cursoroff, bool& is_my_side) const;

		enum Buttons : uint16_t
		{
			BT_CONFIRM,
			BT_CANCEL,
			NUM_BUTTONS
		};

		struct TradeItem
		{
			int32_t itemid = 0;
			int16_t count = 0;
			Texture icon;
			Text namelabel;
			Text countlabel;

			bool empty() const { return itemid == 0; }
		};

		// 9 slots per side
		static constexpr int MAX_SLOTS = 9;

		TradeItem my_items[MAX_SLOTS];
		TradeItem partner_items[MAX_SLOTS];

		int32_t my_meso;
		int32_t partner_meso;

		std::string partner_name;
		Text my_name_label;
		Text partner_name_label;
		Text my_meso_label;
		Text partner_meso_label;
		Text status_label;

		bool my_confirmed;
		bool partner_confirmed;

		int8_t selected_slot;  // -1 = none

		Texture background;

		// Pre-allocated draw objects
		Text confirmed_text;
		Text confirm_btn_text;
		Text cancel_btn_text;
	};
}
