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
#include "../Components/Icon.h"

#include "../../Character/Inventory/Inventory.h"
#include "../../Character/Inventory/InventoryType.h"
#include "../../Graphics/Animation.h"

#include <set>
#include <vector>

namespace ms
{
	class UIStorage : public UIDragElement<PosSTORAGE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::STORAGE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		struct ItemEntry
		{
			int32_t itemid = 0;
			int16_t count = 0;
		};

		UIStorage(const Inventory& inventory);

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> position) override;
		bool is_in_range(Point<int16_t> cursorpos) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void send_scroll(double yoffset) override;
		void rightclick(Point<int16_t> cursorpos) override;
		void doubleclick(Point<int16_t> cursorpos) override;
		bool send_icon(const Icon& icon, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		// Called by handler
		void open(int32_t npcid, uint8_t slots, int32_t meso);
		void close();
		void set_items_for_all(InventoryType::Id types[], size_t ntypes,
			std::vector<ItemEntry> items_by_type[]);
		void set_items_for_tab(InventoryType::Id type, const std::vector<ItemEntry>& items);
		void set_slots(uint8_t value);
		void set_meso(int32_t meso);
		void modify(InventoryType::Id type);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		// Icon type for storage items (drag from storage = take out)
		class StorageItemIcon : public Icon::Type
		{
		public:
			StorageItemIcon(int16_t index, InventoryType::Id tab);

			void drop_on_stage() const override {}
			void drop_on_equips(EquipSlot::Id) const override {}
			bool drop_on_items(InventoryType::Id tab, EquipSlot::Id eqslot, int16_t slot, bool equip) const override;
			void drop_on_bindings(Point<int16_t>, bool) const override {}
			void set_count(int16_t) override {}
			Icon::IconType get_type() override { return Icon::IconType::ITEM; }

		private:
			int16_t index;
			InventoryType::Id tab;
		};

		// Icon type for player inventory items (drag from inventory = store)
		class PlayerItemIcon : public Icon::Type
		{
		public:
			PlayerItemIcon(int16_t slot, int32_t itemid, int16_t count);

			void drop_on_stage() const override {}
			void drop_on_equips(EquipSlot::Id) const override {}
			bool drop_on_items(InventoryType::Id tab, EquipSlot::Id eqslot, int16_t slot, bool equip) const override;
			void drop_on_bindings(Point<int16_t>, bool) const override {}
			void set_count(int16_t) override {}
			Icon::IconType get_type() override { return Icon::IconType::ITEM; }

		private:
			int16_t inv_slot;
			int32_t itemid;
			int16_t count;
		};

		struct InventoryEntry
		{
			int32_t itemid = 0;
			int16_t count = 0;
			int16_t slot = 0;
		};

		void clear_tooltip();
		void refresh_storage_icons();
		void refresh_inventory_icons();
		void refresh_inventory_tab();
		void change_tab(InventoryType::Id type);

		void request_take_out(int16_t index);
		// Partial take-out: take the whole stack, then immediately
		// store `full_count - keep` back. The v83 protocol has no
		// quantity field on take-out so this is the only way to let
		// the player keep just some of a stack.
		void request_take_out_partial(int16_t index, int16_t keep);
		void request_store(int16_t inv_slot, int32_t itemid, int16_t count);
		void request_sort();
		void request_meso_change(bool store_to_storage);
		void close_with_packet();

		uint16_t button_by_tab(InventoryType::Id type) const;

		// Grid position helpers
		int16_t storage_slot_by_cursor(Point<int16_t> cursoroffset) const;
		int16_t inventory_slot_by_cursor(Point<int16_t> cursoroffset) const;
		Point<int16_t> storage_icon_pos(int16_t index) const;
		Point<int16_t> inventory_icon_pos(int16_t index) const;
		bool in_left_panel(Point<int16_t> cursoroffset) const;
		bool in_right_panel(Point<int16_t> cursoroffset) const;

		const Inventory& inventory;

		// Storage items organized by tab
		std::vector<ItemEntry> storage_equip;
		std::vector<ItemEntry> storage_use;
		std::vector<ItemEntry> storage_setup;
		std::vector<ItemEntry> storage_etc;
		std::vector<ItemEntry> storage_cash;

		std::vector<ItemEntry>& get_storage_tab(InventoryType::Id type);
		const std::vector<ItemEntry>& get_storage_tab(InventoryType::Id type) const;

		// Player inventory items for current tab
		std::vector<InventoryEntry> inventory_items;

		// Icon objects for drag-and-drop (keyed by item index in the
		// tab's item vector, not by visual cell).
		std::map<int16_t, std::unique_ptr<Icon>> storage_icons;
		std::map<int16_t, std::unique_ptr<Icon>> inv_icons;

		// Visual layout tables: slot_map[visual_cell] = item index (or -1).
		// Rebuilt on refresh; mutated on same-panel drag/drop so the user
		// can place items in any cube instead of being forced into the
		// server's dense 0..N-1 order.
		std::vector<int16_t> storage_slot_map;
		std::vector<int16_t> inventory_slot_map;

		// Persist per-itemid visual slot across refreshes so the player's
		// layout survives take-out/store operations that reshuffle the
		// server list. Keyed by itemid (storage) or server slot (inv).
		std::map<int32_t, int16_t> storage_preferred_slot;
		std::map<int16_t, int16_t> inventory_preferred_slot;

		int16_t drag_source_slot;

		Texture disabled_slot;
		Texture meso_icon;
		Animation npc_anim;
		Animation newitemslot;
		Text storage_mesolabel;
		Text player_mesolabel;

		InventoryType::Id current_tab;
		// Multi-select — click toggles each item in/out of the set.
		// Retrieve / Store buttons act on every selected item.
		std::set<int16_t> storage_selection;
		std::set<int16_t> inventory_selection;

		// Pending partial take-outs — waiting for the take-out response
		// so we can re-store the excess.
		struct PendingRestore
		{
			int32_t itemid;
			int16_t keep;
			InventoryType::Id tab;
		};
		std::vector<PendingRestore> pending_restores;
		Point<int16_t> lastcursorpos;
		enum DragSource : int8_t { DRAG_NONE, DRAG_STORAGE, DRAG_INVENTORY };
		DragSource drag_source;

		int32_t storage_meso;
		uint8_t storage_slots;
		int32_t npc_id;

		// Grid layout params
		int16_t storage_grid_x, storage_grid_y;
		int16_t inventory_grid_x, inventory_grid_y;
		int16_t cell_size;
		int16_t storage_cols, inventory_cols;
		int16_t storage_rows, inventory_rows;
		int16_t panel_divider_x;

		enum Buttons : int16_t
		{
			BT_GET,
			BT_PUT,
			BT_SORT,
			BT_IN_COIN,
			BT_OUT_COIN,
			BT_EXIT,
			BT_TAB_EQUIP,
			BT_TAB_USE,
			BT_TAB_ETC,
			BT_TAB_SETUP,
			BT_TAB_CASH,
			NUM_BUTTONS
		};
	};
}
