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
#include "UIStorage.h"

#include "UINotice.h"

#include "../UI.h"

#include "../Components/AreaButton.h"
#include "../Components/Charset.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/StoragePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include <algorithm>

namespace ms
{
	// StorageItemIcon - drag from storage to take out
	UIStorage::StorageItemIcon::StorageItemIcon(int16_t idx, InventoryType::Id t)
		: index(idx), tab(t) {}

	bool UIStorage::StorageItemIcon::drop_on_items(InventoryType::Id, EquipSlot::Id, int16_t, bool) const
	{
		// Dropped on right panel (inventory) = take out from storage
		int8_t invtype_byte = 0;
		switch (tab)
		{
		case InventoryType::Id::EQUIP: invtype_byte = 1; break;
		case InventoryType::Id::USE: invtype_byte = 2; break;
		case InventoryType::Id::SETUP: invtype_byte = 3; break;
		case InventoryType::Id::ETC: invtype_byte = 4; break;
		case InventoryType::Id::CASH: invtype_byte = 5; break;
		default: break;
		}

		StorageTakeOutPacket(invtype_byte, static_cast<int8_t>(index)).dispatch();
		return true;
	}

	// PlayerItemIcon - drag from inventory to store
	UIStorage::PlayerItemIcon::PlayerItemIcon(int16_t s, int32_t id, int16_t c)
		: inv_slot(s), itemid(id), count(c) {}

	bool UIStorage::PlayerItemIcon::drop_on_items(InventoryType::Id, EquipSlot::Id, int16_t, bool) const
	{
		// Dropped on left panel (storage) = store item
		StorageStorePacket(inv_slot, itemid, count).dispatch();
		return true;
	}

	UIStorage::UIStorage(const Inventory& inv) : UIDragElement<PosSTORAGE>(Point<int16_t>(500, 20)), inventory(inv)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["Trunk"];

		// Use FullBackgrnd for two-panel layout
		nl::node full_bg_node = src["FullBackgrnd"];
		Texture full_bg_tex = full_bg_node;
		bool use_full = full_bg_tex.is_valid() && full_bg_tex.get_dimensions().x() > 0;

		if (use_full)
		{
			sprites.emplace_back(full_bg_node);
			nl::node fb2 = src["FullBackgrnd2"];
			nl::node fb3 = src["FullBackgrnd3"];
			if (Texture(fb2).is_valid()) sprites.emplace_back(fb2);
			if (Texture(fb3).is_valid()) sprites.emplace_back(fb3);
		}
		else
		{
			sprites.emplace_back(src["backgrnd"]);
			sprites.emplace_back(src["backgrnd2"]);
			sprites.emplace_back(src["backgrnd3"]);
		}

		Texture bg = use_full ? full_bg_tex : Texture(src["backgrnd"]);
		auto bg_dim = bg.get_dimensions();

		// Buttons from NX
		buttons[BT_GET] = std::make_unique<MapleButton>(src["BtGet"]);
		buttons[BT_PUT] = std::make_unique<MapleButton>(src["BtPut"]);
		buttons[BT_SORT] = std::make_unique<MapleButton>(src["BtSort"]);
		buttons[BT_EXIT] = std::make_unique<MapleButton>(src["BtExit"]);

		nl::node bt_incoin = src["BtInCoin"];
		if (bt_incoin.size() > 0)
			buttons[BT_IN_COIN] = std::make_unique<MapleButton>(bt_incoin);
		else
			buttons[BT_IN_COIN] = std::make_unique<AreaButton>(Point<int16_t>(184, 317), Point<int16_t>(70, 20));

		nl::node bt_outcoin = src["BtOutCoin"];
		if (bt_outcoin.size() > 0)
			buttons[BT_OUT_COIN] = std::make_unique<MapleButton>(bt_outcoin);
		else
			buttons[BT_OUT_COIN] = std::make_unique<AreaButton>(Point<int16_t>(11, 317), Point<int16_t>(70, 20));

		// Tab buttons
		nl::node tab_en = src["Tab"]["enabled"];
		nl::node tab_dis = src["Tab"]["disabled"];

		if (tab_en.size() > 0 && tab_dis.size() > 0)
		{
			buttons[BT_TAB_EQUIP] = std::make_unique<TwoSpriteButton>(tab_dis["0"], tab_en["0"]);
			buttons[BT_TAB_USE] = std::make_unique<TwoSpriteButton>(tab_dis["1"], tab_en["1"]);
			buttons[BT_TAB_ETC] = std::make_unique<TwoSpriteButton>(tab_dis["2"], tab_en["2"]);
			buttons[BT_TAB_SETUP] = std::make_unique<TwoSpriteButton>(tab_dis["3"], tab_en["3"]);
			buttons[BT_TAB_CASH] = std::make_unique<TwoSpriteButton>(tab_dis["4"], tab_en["4"]);
		}
		else
		{
			int16_t tab_w = 55;
			int16_t tab_x = use_full ? 352 : 7;
			for (int i = 0; i < 5; i++)
				buttons[BT_TAB_EQUIP + i] = std::make_unique<AreaButton>(Point<int16_t>(tab_x + tab_w * i, 75), Point<int16_t>(tab_w - 2, 20));
		}

		buttons[button_by_tab(InventoryType::Id::EQUIP)]->set_state(Button::State::PRESSED);

		// Sprites
		disabled_slot = src["disabled"];
		meso_icon = nl::nx::ui["UIWindow2.img"]["Shop2"]["meso"];

		// Labels
		storage_mesolabel = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::MINESHAFT);
		player_mesolabel = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::MINESHAFT);

		// Grid layout - 36px cells (matching UIItemInventory), 4 cols both panels
		cell_size = 36;
		storage_cols = 4;
		visible_rows = 4;

		// Left panel: storage items below NPC/buttons
		storage_grid_x = 11;
		storage_grid_y = 170;

		// Right panel: inventory items below tabs (matches left panel grid Y)
		panel_divider_x = 347;
		if (use_full)
		{
			inventory_grid_x = panel_divider_x + 18;
			inventory_grid_y = 170;
			inventory_cols = 4;
		}
		else
		{
			// No FullBackgrnd — still set up a right panel next to the left panel
			panel_divider_x = bg_dim.x();
			inventory_grid_x = panel_divider_x + 10;
			inventory_grid_y = storage_grid_y;
			inventory_cols = 4;
		}

		// State
		current_tab = InventoryType::Id::EQUIP;
		storage_selection = -1;
		inventory_selection = -1;
		drag_source = DRAG_NONE;
		storage_meso = 0;
		storage_slots = 0;
		npc_id = 0;

		// Ensure dimension covers both panels including the inventory grid
		int16_t min_width = bg_dim.x();
		if (inventory_cols > 0)
		{
			int16_t right_edge = inventory_grid_x + inventory_cols * cell_size + 10;
			min_width = std::max(min_width, right_edge);
		}

		dimension = Point<int16_t>(min_width, bg_dim.y());
		dragarea = Point<int16_t>(min_width, 20);
		active = false;
	}

	void UIStorage::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Draw storage items (left grid) using Icon objects
		for (auto& pair : storage_icons)
		{
			if (pair.second)
			{
				Point<int16_t> iconpos = position + storage_icon_pos(pair.first);
				pair.second->draw(iconpos);
			}
		}

		// Draw inventory items (right grid) using Icon objects
		for (auto& pair : inv_icons)
		{
			if (pair.second)
			{
				Point<int16_t> iconpos = position + inventory_icon_pos(pair.first);
				pair.second->draw(iconpos);
			}
		}

		// Draw meso labels
		if (meso_icon.is_valid())
		{
			meso_icon.draw(position + Point<int16_t>(100, 329));
			if (inventory_cols > 0)
				meso_icon.draw(position + Point<int16_t>(inventory_grid_x + 100, 329));
		}

		storage_mesolabel.draw(position + Point<int16_t>(170, 330));
		if (inventory_cols > 0)
			player_mesolabel.draw(position + Point<int16_t>(inventory_grid_x + 170, 330));
	}

	void UIStorage::update()
	{
		int64_t player_meso = inventory.get_meso();

		std::string storage_mesostr = std::to_string(storage_meso);
		std::string player_mesostr = std::to_string(player_meso);
		string_format::split_number(storage_mesostr);
		string_format::split_number(player_mesostr);

		storage_mesolabel.change_text(storage_mesostr);
		player_mesolabel.change_text(player_mesostr);
	}

	bool UIStorage::is_in_range(Point<int16_t> cursorpos) const
	{
		auto drawpos = get_draw_position();
		auto bounds = Rectangle<int16_t>(drawpos, drawpos + dimension);
		return bounds.contains(cursorpos);
	}

	Cursor::State UIStorage::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;
		lastcursorpos = cursoroffset;

		// Show tooltips
		int16_t sslot = storage_slot_by_cursor(cursoroffset);
		int16_t islot = inventory_slot_by_cursor(cursoroffset);

		if (sslot >= 0)
		{
			const auto& items = get_storage_tab(current_tab);
			if (sslot < static_cast<int16_t>(items.size()))
				UI::get().show_item(Tooltip::Parent::SHOP, items[sslot].itemid);
			else
				clear_tooltip();
		}
		else if (islot >= 0)
		{
			if (islot < static_cast<int16_t>(inventory_items.size()))
				UI::get().show_item(Tooltip::Parent::SHOP, inventory_items[islot].itemid);
			else
				clear_tooltip();
		}
		else
		{
			clear_tooltip();
		}

		// Handle clicks - initiate drag or select
		if (clicked)
		{
			// Check storage grid (left panel)
			if (sslot >= 0)
			{
				const auto& items = get_storage_tab(current_tab);
				if (sslot < static_cast<int16_t>(items.size()))
				{
					auto it = storage_icons.find(sslot);
					if (it != storage_icons.end() && it->second)
					{
						Point<int16_t> slotpos = storage_icon_pos(sslot);
						it->second->start_drag(cursoroffset - slotpos);
						UI::get().drag_icon(it->second.get());

						drag_source = DRAG_STORAGE;
						storage_selection = sslot;
						inventory_selection = -1;
						clear_tooltip();
						return Cursor::State::GRABBING;
					}
				}
				return Cursor::State::CLICKING;
			}

			// Check inventory grid (right panel)
			if (islot >= 0)
			{
				if (islot < static_cast<int16_t>(inventory_items.size()))
				{
					auto it = inv_icons.find(islot);
					if (it != inv_icons.end() && it->second)
					{
						Point<int16_t> slotpos = inventory_icon_pos(islot);
						it->second->start_drag(cursoroffset - slotpos);
						UI::get().drag_icon(it->second.get());

						drag_source = DRAG_INVENTORY;
						inventory_selection = islot;
						storage_selection = -1;
						clear_tooltip();
						return Cursor::State::GRABBING;
					}
				}
				return Cursor::State::CLICKING;
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	bool UIStorage::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		// Only allow cross-panel drops:
		// Storage item (left) dragged to right panel = take out
		if (drag_source == DRAG_STORAGE && in_right_panel(cursoroffset))
		{
			icon.drop_on_items(current_tab, EquipSlot::Id::NONE, 0, false);
			drag_source = DRAG_NONE;
			return true;
		}

		// Inventory item (right) dragged to left panel = store
		if (drag_source == DRAG_INVENTORY && in_left_panel(cursoroffset))
		{
			icon.drop_on_items(current_tab, EquipSlot::Id::NONE, 0, false);
			drag_source = DRAG_NONE;
			return true;
		}

		// Dropped on same panel or outside — cancel
		drag_source = DRAG_NONE;
		return true;
	}

	void UIStorage::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close_with_packet();
	}

	void UIStorage::send_scroll(double yoffset)
	{
	}

	void UIStorage::rightclick(Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		int16_t sslot = storage_slot_by_cursor(cursoroffset);
		if (sslot >= 0)
		{
			const auto& items = get_storage_tab(current_tab);
			if (sslot < static_cast<int16_t>(items.size()))
			{
				storage_selection = sslot;
				inventory_selection = -1;
				request_take_out(sslot);
			}
			return;
		}

		int16_t islot = inventory_slot_by_cursor(cursoroffset);
		if (islot >= 0)
		{
			if (islot < static_cast<int16_t>(inventory_items.size()))
			{
				inventory_selection = islot;
				storage_selection = -1;
				const auto& entry = inventory_items[islot];
				request_store(entry.slot, entry.itemid, entry.count);
			}
		}
	}

	UIElement::Type UIStorage::get_type() const
	{
		return TYPE;
	}

	Button::State UIStorage::button_pressed(uint16_t buttonid)
	{
		clear_tooltip();

		switch (buttonid)
		{
		case BT_GET:
			if (storage_selection >= 0)
				request_take_out(storage_selection);
			return Button::State::NORMAL;

		case BT_PUT:
			if (inventory_selection >= 0 && inventory_selection < static_cast<int16_t>(inventory_items.size()))
			{
				const auto& entry = inventory_items[inventory_selection];
				request_store(entry.slot, entry.itemid, entry.count);
			}
			return Button::State::NORMAL;

		case BT_SORT:
			request_sort();
			return Button::State::NORMAL;

		case BT_IN_COIN:
			request_meso_change(true);
			return Button::State::NORMAL;

		case BT_OUT_COIN:
			request_meso_change(false);
			return Button::State::NORMAL;

		case BT_EXIT:
			close_with_packet();
			return Button::State::PRESSED;

		case BT_TAB_EQUIP:
			change_tab(InventoryType::Id::EQUIP);
			return Button::State::IDENTITY;
		case BT_TAB_USE:
			change_tab(InventoryType::Id::USE);
			return Button::State::IDENTITY;
		case BT_TAB_ETC:
			change_tab(InventoryType::Id::ETC);
			return Button::State::IDENTITY;
		case BT_TAB_SETUP:
			change_tab(InventoryType::Id::SETUP);
			return Button::State::IDENTITY;
		case BT_TAB_CASH:
			change_tab(InventoryType::Id::CASH);
			return Button::State::IDENTITY;

		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIStorage::open(int32_t new_npcid, uint8_t slots, int32_t meso)
	{
		npc_id = new_npcid;
		storage_slots = slots;
		storage_meso = meso;

		storage_selection = -1;
		inventory_selection = -1;

		storage_equip.clear();
		storage_use.clear();
		storage_setup.clear();
		storage_etc.clear();
		storage_cash.clear();
		storage_icons.clear();
		inv_icons.clear();

		current_tab = InventoryType::Id::EQUIP;

		for (uint16_t i = BT_TAB_EQUIP; i <= BT_TAB_CASH; i++)
			buttons[i]->set_state(Button::State::NORMAL);

		buttons[button_by_tab(current_tab)]->set_state(Button::State::PRESSED);

		refresh_inventory_tab();
		makeactive();
	}

	void UIStorage::close()
	{
		clear_tooltip();
		active = false;
		storage_selection = -1;
		inventory_selection = -1;
		storage_icons.clear();
		inv_icons.clear();
	}

	void UIStorage::set_items_for_all(InventoryType::Id types[], size_t ntypes,
		std::vector<ItemEntry> items_by_type[])
	{
		for (size_t i = 0; i < ntypes; i++)
			get_storage_tab(types[i]) = items_by_type[i];

		refresh_storage_icons();
		refresh_inventory_icons();
	}

	void UIStorage::set_items_for_tab(InventoryType::Id type, const std::vector<ItemEntry>& items)
	{
		get_storage_tab(type) = items;

		if (type == current_tab)
		{
			refresh_storage_icons();
			refresh_inventory_tab();
		}
	}

	void UIStorage::set_slots(uint8_t value)
	{
		storage_slots = value;
	}

	void UIStorage::set_meso(int32_t meso)
	{
		storage_meso = meso;
	}

	void UIStorage::modify(InventoryType::Id type)
	{
		if (type == current_tab)
			refresh_inventory_tab();
	}

	void UIStorage::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::SHOP);
	}

	void UIStorage::refresh_storage_icons()
	{
		storage_icons.clear();

		const auto& items = get_storage_tab(current_tab);
		for (int16_t i = 0; i < static_cast<int16_t>(items.size()); i++)
		{
			int16_t count = (current_tab == InventoryType::Id::EQUIP) ? -1 : items[i].count;
			const Texture& tex = ItemData::get(items[i].itemid).get_icon(false);

			storage_icons[i] = std::make_unique<Icon>(
				std::make_unique<StorageItemIcon>(i, current_tab),
				tex, count
			);
		}
	}

	void UIStorage::refresh_inventory_icons()
	{
		inv_icons.clear();

		for (int16_t i = 0; i < static_cast<int16_t>(inventory_items.size()); i++)
		{
			const auto& entry = inventory_items[i];
			int16_t count = (current_tab == InventoryType::Id::EQUIP) ? -1 : entry.count;
			const Texture& tex = ItemData::get(entry.itemid).get_icon(false);

			inv_icons[i] = std::make_unique<Icon>(
				std::make_unique<PlayerItemIcon>(entry.slot, entry.itemid, entry.count),
				tex, count
			);
		}
	}

	void UIStorage::refresh_inventory_tab()
	{
		inventory_items.clear();

		int16_t slotmax = inventory.get_slotmax(current_tab);
		for (int16_t slot = 1; slot <= slotmax; ++slot)
		{
			int32_t itemid = inventory.get_item_id(current_tab, slot);
			if (itemid == 0)
				continue;

			int16_t count = (current_tab == InventoryType::Id::EQUIP)
				? 1
				: inventory.get_item_count(current_tab, slot);

			InventoryEntry entry;
			entry.itemid = itemid;
			entry.count = count;
			entry.slot = slot;
			inventory_items.push_back(entry);
		}

		refresh_inventory_icons();
	}

	void UIStorage::change_tab(InventoryType::Id type)
	{
		uint16_t oldtab = button_by_tab(current_tab);
		uint16_t newtab = button_by_tab(type);

		buttons[oldtab]->set_state(Button::State::NORMAL);
		buttons[newtab]->set_state(Button::State::PRESSED);

		current_tab = type;
		storage_selection = -1;
		inventory_selection = -1;

		refresh_storage_icons();
		refresh_inventory_tab();
	}

	void UIStorage::request_take_out(int16_t index)
	{
		const auto& items = get_storage_tab(current_tab);
		if (index < 0 || index >= static_cast<int16_t>(items.size()))
			return;

		int8_t invtype_byte = 0;
		switch (current_tab)
		{
		case InventoryType::Id::EQUIP: invtype_byte = 1; break;
		case InventoryType::Id::USE: invtype_byte = 2; break;
		case InventoryType::Id::SETUP: invtype_byte = 3; break;
		case InventoryType::Id::ETC: invtype_byte = 4; break;
		case InventoryType::Id::CASH: invtype_byte = 5; break;
		default: break;
		}

		StorageTakeOutPacket(invtype_byte, static_cast<int8_t>(index)).dispatch();
	}

	void UIStorage::request_store(int16_t inv_slot, int32_t itemid, int16_t count)
	{
		int16_t max_quantity = std::max<int16_t>(1, count);

		if (current_tab != InventoryType::Id::EQUIP && max_quantity > 1)
		{
			auto on_enter = [inv_slot, itemid](int32_t quantity)
			{
				auto shortqty = static_cast<int16_t>(quantity);
				StorageStorePacket(inv_slot, itemid, shortqty).dispatch();
			};

			UI::get().emplace<UIEnterNumber>("How many would you like to store?", on_enter, max_quantity, 1);
		}
		else
		{
			StorageStorePacket(inv_slot, itemid, 1).dispatch();
		}
	}

	void UIStorage::request_sort()
	{
		StorageArrangePacket().dispatch();
	}

	void UIStorage::request_meso_change(bool store_to_storage)
	{
		int32_t max_value = 0;

		if (store_to_storage)
		{
			int64_t player_meso = inventory.get_meso();
			max_value = static_cast<int32_t>(std::min<int64_t>(player_meso, 2147483647));
		}
		else
		{
			max_value = std::max<int32_t>(0, storage_meso);
		}

		if (max_value <= 0)
			return;

		auto on_enter = [store_to_storage](int32_t amount)
		{
			int32_t signed_amount = store_to_storage ? -amount : amount;
			StorageMesoPacket(signed_amount).dispatch();
		};

		const char* question = store_to_storage
			? "How many mesos would you like to store?"
			: "How many mesos would you like to take out?";

		UI::get().emplace<UIEnterNumber>(question, on_enter, max_value, 1);
	}

	void UIStorage::close_with_packet()
	{
		close();
		StorageClosePacket().dispatch();
	}

	uint16_t UIStorage::button_by_tab(InventoryType::Id type) const
	{
		switch (type)
		{
		case InventoryType::Id::EQUIP: return BT_TAB_EQUIP;
		case InventoryType::Id::USE: return BT_TAB_USE;
		case InventoryType::Id::ETC: return BT_TAB_ETC;
		case InventoryType::Id::SETUP: return BT_TAB_SETUP;
		case InventoryType::Id::CASH: return BT_TAB_CASH;
		default: return BT_TAB_EQUIP;
		}
	}

	bool UIStorage::in_left_panel(Point<int16_t> cursoroffset) const
	{
		return cursoroffset.x() >= 0 && cursoroffset.x() < panel_divider_x
			&& cursoroffset.y() >= 0 && cursoroffset.y() < dimension.y();
	}

	bool UIStorage::in_right_panel(Point<int16_t> cursoroffset) const
	{
		if (inventory_cols <= 0) return false;
		return cursoroffset.x() >= panel_divider_x
			&& cursoroffset.y() >= 0 && cursoroffset.y() < dimension.y();
	}

	Point<int16_t> UIStorage::storage_icon_pos(int16_t index) const
	{
		int16_t col = index % storage_cols;
		int16_t row = index / storage_cols;
		return Point<int16_t>(storage_grid_x + col * cell_size, storage_grid_y + row * cell_size);
	}

	Point<int16_t> UIStorage::inventory_icon_pos(int16_t index) const
	{
		if (inventory_cols <= 0) return Point<int16_t>(0, 0);
		int16_t col = index % inventory_cols;
		int16_t row = index / inventory_cols;
		return Point<int16_t>(inventory_grid_x + col * cell_size, inventory_grid_y + row * cell_size);
	}

	int16_t UIStorage::storage_slot_by_cursor(Point<int16_t> cursoroffset) const
	{
		int16_t x = cursoroffset.x() - storage_grid_x;
		int16_t y = cursoroffset.y() - storage_grid_y;

		if (x < 0 || y < 0) return -1;

		int16_t col = x / cell_size;
		int16_t row = y / cell_size;

		if (col >= storage_cols || row >= visible_rows) return -1;

		return row * storage_cols + col;
	}

	int16_t UIStorage::inventory_slot_by_cursor(Point<int16_t> cursoroffset) const
	{
		if (inventory_cols <= 0) return -1;

		int16_t x = cursoroffset.x() - inventory_grid_x;
		int16_t y = cursoroffset.y() - inventory_grid_y;

		if (x < 0 || y < 0) return -1;

		int16_t col = x / cell_size;
		int16_t row = y / cell_size;

		if (col >= inventory_cols || row >= visible_rows) return -1;

		return row * inventory_cols + col;
	}

	std::vector<UIStorage::ItemEntry>& UIStorage::get_storage_tab(InventoryType::Id type)
	{
		switch (type)
		{
		case InventoryType::Id::EQUIP: return storage_equip;
		case InventoryType::Id::USE: return storage_use;
		case InventoryType::Id::SETUP: return storage_setup;
		case InventoryType::Id::ETC: return storage_etc;
		case InventoryType::Id::CASH: return storage_cash;
		default: return storage_etc;
		}
	}

	const std::vector<UIStorage::ItemEntry>& UIStorage::get_storage_tab(InventoryType::Id type) const
	{
		switch (type)
		{
		case InventoryType::Id::EQUIP: return storage_equip;
		case InventoryType::Id::USE: return storage_use;
		case InventoryType::Id::SETUP: return storage_setup;
		case InventoryType::Id::ETC: return storage_etc;
		case InventoryType::Id::CASH: return storage_cash;
		default: return storage_etc;
		}
	}
}
