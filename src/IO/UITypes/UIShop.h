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

#include "../../Character/Look/CharLook.h"
#include "../../Graphics/Animation.h"

namespace ms
{
	class UIShop : public UIDragElement<PosSHOP>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SHOP;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIShop(const CharLook& charlook, const Inventory& inventory);

		void draw(float alpha) const override;
		void update() override;

		void remove_cursor() override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> position) override;
		void send_scroll(double yoffset) override;
		void rightclick(Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void reset(int32_t npcid);
		// Build the buy-side category tabs — call after the server's
		// full item list has been added so only categories present in
		// this shop get a tab.
		void finalize_buy_tabs();
		void add_item(int32_t id, int32_t price, int32_t pitch, int32_t time, int16_t buyable);
		void add_rechargable(int32_t id, int32_t price, int32_t pitch, int32_t time, int16_t chargeprice, int16_t buyable);

		void modify(InventoryType::Id type);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void clear_tooltip();
		void show_item(int16_t slot, bool sale);
		void changeselltab(InventoryType::Id tab);
		int16_t slot_by_position(int16_t y);
		uint16_t tabbyinventory(InventoryType::Id type);
		uint16_t buy_tab_by_inventory(InventoryType::Id type);
		void changebuytab(InventoryType::Id type);
		void exit_shop();

		enum Buttons : int16_t
		{
			BUY_ITEM,
			SELL_ITEM,
			EXIT,
			CHECKBOX,
			OVERALL,
			EQUIP,
			USE,
			ETC,
			SETUP,
			CASH,
			BUY_TAB_EQUIP,
			BUY_TAB_USE,
			BUY_TAB_ETC,
			BUY_TAB_SETUP,
			BUY_TAB_CASH,
			BUY0,
			BUY1,
			BUY2,
			BUY3,
			BUY4,
			BUY5,
			BUY6,
			BUY7,
			BUY8,
			SELL0,
			SELL1,
			SELL2,
			SELL3,
			SELL4,
			SELL5,
			SELL6,
			SELL7,
			SELL8,
			NUM_BUTTONS
		};

		const CharLook& charlook;
		const Inventory& inventory;

		Animation npc;
		Texture buy_selection;
		Texture sell_selection;
		Texture meso;
		// Single shared meso-coin animation overlaid on every item's
		// price row, so all items blink in sync.
		Animation meso_anim;
		// Local frame counter for the char portrait so it animates
		// independently of whatever the player's world char is doing.
		mutable uint8_t charframe = 0;
		mutable uint16_t charframe_delay = 0;
		Text mesolabel;

		Slider buyslider;
		Slider sellslider;

		int16_t buy_x;
		int16_t buy_width;
		int16_t sell_x;
		int16_t sell_width;

		BoolPair<Texture> checkBox;

		bool rightclicksell;

		Point<int16_t> lastcursorpos;

		class BuyItem
		{
		public:
			BuyItem(Texture cur, int32_t i, int32_t p, int32_t pt, int32_t t, int16_t cp, int16_t b);

			void draw(Point<int16_t> position) const;

			int32_t get_id() const;
			int16_t get_buyable() const;

		private:
			Texture icon;
			Texture currency;
			int32_t id;
			int32_t price;
			int32_t pitch;
			int32_t time;
			int16_t chargeprice;
			int16_t buyable;
			Text namelabel;
			Text pricelabel;
		};

		class SellItem
		{
		public:
			SellItem(int32_t item_id, int16_t count, int16_t slot, bool showcount, Texture cur);

			void draw(Point<int16_t> position) const;

			int32_t get_id() const;
			int16_t get_slot() const;
			int16_t get_sellable() const;

		private:
			Texture icon;
			int32_t id;
			int16_t slot;
			int16_t sellable;
			bool showcount;
			Text namelabel;
			Text pricelabel;
		};

		struct BuyState
		{
			// Full list from the server, plus the filtered view that
			// the UI actually displays. `item_slots` maps the filtered
			// index back to the server's buy-list slot, which is what
			// NpcShopActionPacket expects.
			std::vector<BuyItem> all_items;
			std::vector<BuyItem> items;
			std::vector<int16_t> item_slots;
			// NONE = no filter (show all).
			InventoryType::Id filter = InventoryType::Id::NONE;
			int16_t offset;
			int16_t lastslot;
			int16_t selection;

			void reset();
			void draw(Point<int16_t> position, const Texture& selected) const;
			void show_item(int16_t slot);
			void add(BuyItem item);
			void set_filter(InventoryType::Id f);
			void buy() const;
			void select(int16_t selected);
		};

		BuyState buystate;

		struct SellState
		{
			std::vector<SellItem> items;
			int16_t offset;
			InventoryType::Id tab;
			int16_t lastslot;
			int16_t selection;

			void reset();
			void change_tab(const Inventory& inventory, InventoryType::Id type, Texture meso);
			void draw(Point<int16_t> position, const Texture& selected) const;
			void show_item(int16_t slot);
			void sell(bool skip_confirmation) const;
			void select(int16_t selected);
		};

		SellState sellstate;
	};
}