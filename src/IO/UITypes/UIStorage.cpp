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
#include "../../Character/Look/CharLook.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Graphics/Geometry.h"

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
		// Reuse the inventory "new item" animated outline for the
		// selection highlight so it matches the rest of the game UI.
		newitemslot = nl::nx::ui["UIWindow.img"]["Item"]["New"]["inventory"];

		// Labels — left-aligned so they sit immediately after the coin icon.
		storage_mesolabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MINESHAFT);
		player_mesolabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MINESHAFT);

		// Grid layout — matches the cube strip on the backdrop.
		// Left panel (storage) is the narrower pane; right panel
		// (inventory) is wider so it gets more columns and starts
		// higher up to fit one extra row.
		cell_size = 36;

		storage_cols = 4;
		storage_rows = 6;
		storage_grid_x = 11;
		storage_grid_y = 102;

		panel_divider_x = 186;
		if (use_full)
		{
			inventory_cols = 16;
			inventory_rows = 6;
			inventory_grid_x = panel_divider_x;
			inventory_grid_y = 100;
		}
		else
		{
			panel_divider_x = bg_dim.x();
			inventory_grid_x = 0;
			inventory_grid_y = storage_grid_y;
			inventory_cols = 0;
			inventory_rows = 0;
		}

		// State
		current_tab = InventoryType::Id::EQUIP;
		storage_selection.clear();
		inventory_selection.clear();
		drag_source = DRAG_NONE;
		drag_source_slot = -1;
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
		// Wider drag-strip — the top 30 px (where the window has no
		// interactive buttons or slots) can be grabbed to move the UI.
		dragarea = Point<int16_t>(min_width, 30);
		active = false;
	}

	void UIStorage::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Animated storage-keeper NPC portrait pinned above the grid.
		// Tall sprites (Fredrick et al.) get a uniform scale to fit a
		// 75-px slot; small NPCs draw at native size.
		{
			Point<int16_t> portrait_pos = position + Point<int16_t>(55, 85);
			auto dim = npc_anim.get_dimensions();
			constexpr int16_t PORTRAIT_SLOT = 75;
			float s = 1.0f;
			if (dim.x() > 0 && dim.y() > 0)
			{
				float sx = static_cast<float>(PORTRAIT_SLOT) / dim.x();
				float sy = static_cast<float>(PORTRAIT_SLOT) / dim.y();
				s = std::min(sx, sy);
				if (s > 1.0f) s = 1.0f;
			}
			npc_anim.draw(DrawArgument(portrait_pos, s, s, 1.0f), alpha);
		}

		// Player's own character portrait above the inventory grid.
		if (inventory_cols > 0)
		{
			const CharLook& look = Stage::get().get_player().get_look();
			Point<int16_t> player_pos = position
				+ Point<int16_t>(panel_divider_x + 30, 60);
			const_cast<CharLook&>(look).draw(
				DrawArgument(player_pos, 0.75f, 0.75f, 1.0f), 1.0f);
		}

		// Selection highlight — only storage items can be highlighted,
		// so no pass for the inventory panel.
		for (int16_t s = 0; s < static_cast<int16_t>(storage_slot_map.size()); s++)
		{
			int16_t idx = storage_slot_map[s];
			if (idx >= 0 && storage_selection.count(idx))
				newitemslot.draw(position + storage_icon_pos(s) + Point<int16_t>(1, 1), alpha);
		}

		// Draw items at their visual-cell positions. storage_slot_map
		// and inventory_slot_map are the client-side arrangement —
		// rebuilt on refresh but mutated by in-panel drag-drop so the
		// player can put items in any cube they like.
		for (int16_t s = 0; s < static_cast<int16_t>(storage_slot_map.size()); s++)
		{
			int16_t item_idx = storage_slot_map[s];
			if (item_idx < 0) continue;
			auto it = storage_icons.find(item_idx);
			if (it == storage_icons.end() || !it->second) continue;
			Point<int16_t> iconpos = position + storage_icon_pos(s);
			it->second->draw(iconpos);
		}
		for (int16_t s = 0; s < static_cast<int16_t>(inventory_slot_map.size()); s++)
		{
			int16_t item_idx = inventory_slot_map[s];
			if (item_idx < 0) continue;
			auto it = inv_icons.find(item_idx);
			if (it == inv_icons.end() || !it->second) continue;
			Point<int16_t> iconpos = position + inventory_icon_pos(s);
			it->second->draw(iconpos);
		}

		// Meso row: left panel shows the player's current wallet, right
		// panel shows the storage amount as sent by the server. Labels
		// draw immediately to the right of each coin icon.
		const int16_t left_icon_x  = 52;
		const int16_t left_icon_y  = 318;
		const int16_t right_icon_x_off = 49;
		const int16_t right_icon_y = 320;

		// Text baseline — Text draws from the top so pull up to
		// visually land the digits on the coin's centre line.
		const int16_t left_label_y_adjust  = -4;
		const int16_t right_label_y_adjust = -7;

		if (meso_icon.is_valid())
			meso_icon.draw(position + Point<int16_t>(left_icon_x, left_icon_y));
		storage_mesolabel.draw(position + Point<int16_t>(left_icon_x + 20, left_icon_y + left_label_y_adjust));

		if (inventory_cols > 0)
		{
			if (meso_icon.is_valid())
				meso_icon.draw(position + Point<int16_t>(panel_divider_x + right_icon_x_off, right_icon_y));
			player_mesolabel.draw(position + Point<int16_t>(panel_divider_x + right_icon_x_off + 20, right_icon_y + right_label_y_adjust));
		}
	}

	void UIStorage::update()
	{
		// Advance the NPC portrait animation so the storage keeper
		// breathes / blinks instead of being a still frame.
		npc_anim.update();
		newitemslot.update(6);

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

		// Visual-cell → item-index lookup
		int16_t svisual = storage_slot_by_cursor(cursoroffset);
		int16_t ivisual = inventory_slot_by_cursor(cursoroffset);

		int16_t s_item = -1;
		int16_t i_item = -1;
		if (svisual >= 0 && svisual < static_cast<int16_t>(storage_slot_map.size()))
			s_item = storage_slot_map[svisual];
		if (ivisual >= 0 && ivisual < static_cast<int16_t>(inventory_slot_map.size()))
			i_item = inventory_slot_map[ivisual];

		if (s_item >= 0)
		{
			const auto& items = get_storage_tab(current_tab);
			if (s_item < static_cast<int16_t>(items.size()))
				UI::get().show_item(Tooltip::Parent::SHOP, items[s_item].itemid);
			else
				clear_tooltip();
		}
		else if (i_item >= 0)
		{
			if (i_item < static_cast<int16_t>(inventory_items.size()))
				UI::get().show_item(Tooltip::Parent::SHOP, inventory_items[i_item].itemid);
			else
				clear_tooltip();
		}
		else
		{
			clear_tooltip();
		}

		// Handle clicks — clicking an already-highlighted item removes
		// the highlight; clicking an un-highlighted item starts a drag.
		if (clicked)
		{
			if (svisual >= 0 && s_item >= 0)
			{
				auto it = storage_icons.find(s_item);
				if (it != storage_icons.end() && it->second)
				{
					Point<int16_t> slotpos = storage_icon_pos(svisual);
					it->second->start_drag(cursoroffset - slotpos);
					UI::get().drag_icon(it->second.get());

					drag_source = DRAG_STORAGE;
					drag_source_slot = svisual;
					clear_tooltip();
					return Cursor::State::GRABBING;
				}
			}
			if (svisual >= 0)
				return Cursor::State::CLICKING;

			if (ivisual >= 0 && i_item >= 0)
			{
				auto it = inv_icons.find(i_item);
				if (it != inv_icons.end() && it->second)
				{
					Point<int16_t> slotpos = inventory_icon_pos(ivisual);
					it->second->start_drag(cursoroffset - slotpos);
					UI::get().drag_icon(it->second.get());

					drag_source = DRAG_INVENTORY;
					drag_source_slot = ivisual;
					clear_tooltip();
					return Cursor::State::GRABBING;
				}
			}
			if (ivisual >= 0)
				return Cursor::State::CLICKING;
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	bool UIStorage::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		// Cross-panel: storage → inventory = take out
		if (drag_source == DRAG_STORAGE && in_right_panel(cursoroffset))
		{
			icon.drop_on_items(current_tab, EquipSlot::Id::NONE, 0, false);
			drag_source = DRAG_NONE;
			drag_source_slot = -1;
			return true;
		}

		// Cross-panel: inventory → storage = store
		if (drag_source == DRAG_INVENTORY && in_left_panel(cursoroffset))
		{
			icon.drop_on_items(current_tab, EquipSlot::Id::NONE, 0, false);
			drag_source = DRAG_NONE;
			drag_source_slot = -1;
			return true;
		}

		// Same-panel drop — rearrange the visual cell (client-side only).
		// The server keeps its own sequential order; we just remember
		// where the player wants each item displayed.
		if (drag_source == DRAG_STORAGE && in_left_panel(cursoroffset) && drag_source_slot >= 0)
		{
			int16_t drop = storage_slot_by_cursor(cursoroffset);
			if (drop >= 0 && drop < static_cast<int16_t>(storage_slot_map.size())
				&& drop != drag_source_slot)
			{
				int16_t src = drag_source_slot;
				std::swap(storage_slot_map[src], storage_slot_map[drop]);

				const auto& items = get_storage_tab(current_tab);
				auto persist = [&](int16_t slot)
				{
					int16_t idx = storage_slot_map[slot];
					if (idx >= 0 && idx < static_cast<int16_t>(items.size()))
						storage_preferred_slot[items[idx].itemid] = slot;
				};
				persist(src);
				persist(drop);
			}
			drag_source = DRAG_NONE;
			drag_source_slot = -1;
			return true;
		}

		if (drag_source == DRAG_INVENTORY && in_right_panel(cursoroffset) && drag_source_slot >= 0)
		{
			int16_t drop = inventory_slot_by_cursor(cursoroffset);
			if (drop >= 0 && drop < static_cast<int16_t>(inventory_slot_map.size())
				&& drop != drag_source_slot)
			{
				int16_t src = drag_source_slot;
				std::swap(inventory_slot_map[src], inventory_slot_map[drop]);

				auto persist = [&](int16_t slot)
				{
					int16_t idx = inventory_slot_map[slot];
					if (idx >= 0 && idx < static_cast<int16_t>(inventory_items.size()))
						inventory_preferred_slot[inventory_items[idx].slot] = slot;
				};
				persist(src);
				persist(drop);
			}
			drag_source = DRAG_NONE;
			drag_source_slot = -1;
			return true;
		}

		// Outside the window — cancel
		drag_source = DRAG_NONE;
		drag_source_slot = -1;
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

	void UIStorage::doubleclick(Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		// Double-click on a storage item:
		//  - stackable with count > 1 → open quantity prompt
		//  - otherwise → take it out (single retrieve)
		int16_t svisual = storage_slot_by_cursor(cursoroffset);
		if (svisual >= 0 && svisual < static_cast<int16_t>(storage_slot_map.size()))
		{
			int16_t s_item = storage_slot_map[svisual];
			if (s_item >= 0)
			{
				const auto& items = get_storage_tab(current_tab);
				if (s_item < static_cast<int16_t>(items.size())
					&& current_tab != InventoryType::Id::EQUIP
					&& items[s_item].count > 1)
				{
					int16_t max_qty = items[s_item].count;
					int16_t index = s_item;
					auto on_enter = [this, index](int32_t quantity)
					{
						request_take_out_partial(index, static_cast<int16_t>(quantity));
					};
					UI::get().emplace<UIEnterNumber>(
						"How many would you like to take out?",
						on_enter, max_qty, max_qty, -41);
					return;
				}

				if (s_item < static_cast<int16_t>(items.size()))
					request_take_out(s_item);
			}
			return;
		}

		// Double-click on an inventory item stores it immediately;
		// stackables route through the quantity prompt.
		int16_t ivisual = inventory_slot_by_cursor(cursoroffset);
		if (ivisual >= 0 && ivisual < static_cast<int16_t>(inventory_slot_map.size()))
		{
			int16_t i_item = inventory_slot_map[ivisual];
			if (i_item >= 0 && i_item < static_cast<int16_t>(inventory_items.size()))
			{
				const auto& entry = inventory_items[i_item];
				request_store(entry.slot, entry.itemid, entry.count);
			}
		}
	}

	void UIStorage::rightclick(Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		// Right-click on a storage item toggles the multi-retrieve
		// highlight; the Retrieve button then acts on the full set.
		int16_t svisual = storage_slot_by_cursor(cursoroffset);
		if (svisual >= 0 && svisual < static_cast<int16_t>(storage_slot_map.size()))
		{
			int16_t s_item = storage_slot_map[svisual];
			if (s_item >= 0)
			{
				const auto& items = get_storage_tab(current_tab);
				if (s_item < static_cast<int16_t>(items.size()))
				{
					if (storage_selection.count(s_item))
						storage_selection.erase(s_item);
					else
						storage_selection.insert(s_item);
				}
			}
			return;
		}

		// Right-click on an inventory item toggles the bulk-store
		// highlight; the Store (BT_PUT) button then stores the full set.
		int16_t ivisual = inventory_slot_by_cursor(cursoroffset);
		if (ivisual >= 0 && ivisual < static_cast<int16_t>(inventory_slot_map.size()))
		{
			int16_t i_item = inventory_slot_map[ivisual];
			if (i_item >= 0 && i_item < static_cast<int16_t>(inventory_items.size()))
			{
				if (inventory_selection.count(i_item))
					inventory_selection.erase(i_item);
				else
					inventory_selection.insert(i_item);
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
		{
			// Take out every highlighted storage item. Iterate from
			// highest index down so earlier take-outs don't shift the
			// indices we haven't processed yet.
			std::vector<int16_t> picks(storage_selection.rbegin(), storage_selection.rend());
			const auto& items = get_storage_tab(current_tab);

			// Fallback: if nothing is highlighted, take out whatever
			// the cursor is hovering over.
			if (picks.empty())
			{
				int16_t hover = storage_slot_by_cursor(lastcursorpos);
				if (hover >= 0 && hover < static_cast<int16_t>(storage_slot_map.size()))
				{
					int16_t idx = storage_slot_map[hover];
					if (idx >= 0) picks.push_back(idx);
				}
			}

			// Single stackable pick → prompt the player for a partial
			// quantity. Multi-pick or equip → take each stack in full.
			if (picks.size() == 1 && picks[0] < static_cast<int16_t>(items.size())
				&& current_tab != InventoryType::Id::EQUIP
				&& items[picks[0]].count > 1)
			{
				int16_t max_qty = items[picks[0]].count;
				int16_t index = picks[0];
				auto on_enter = [this, index](int32_t quantity)
				{
					request_take_out_partial(index, static_cast<int16_t>(quantity));
				};
				UI::get().emplace<UIEnterNumber>(
					"How many would you like to take out?",
					on_enter, max_qty, max_qty, -41);
				storage_selection.clear();
				return Button::State::NORMAL;
			}

			for (int16_t idx : picks)
				if (idx >= 0 && idx < static_cast<int16_t>(items.size()))
					request_take_out(idx);
			storage_selection.clear();
			return Button::State::NORMAL;
		}

		case BT_PUT:
		{
			std::vector<int16_t> picks(inventory_selection.rbegin(), inventory_selection.rend());
			for (int16_t idx : picks)
				if (idx >= 0 && idx < static_cast<int16_t>(inventory_items.size()))
				{
					const auto& entry = inventory_items[idx];
					request_store(entry.slot, entry.itemid, entry.count);
				}
			inventory_selection.clear();
			return Button::State::NORMAL;
		}

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

		// Load the storage-keeper NPC's animated stand so it breathes /
		// blinks in the portrait slot instead of staying a still frame.
		// v83 pads npc ids to 7 digits + ".img".
#ifdef USE_NX
		{
			std::string strid = std::to_string(npc_id);
			if (strid.size() < 7)
				strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");
			nl::node src = nl::nx::npc[strid];
			std::string link = (std::string)src["info"]["link"];
			if (!link.empty())
				src = nl::nx::npc[link + ".img"];

			// Pick the state with the MOST frames so the portrait actually
			// animates when the NPC has a multi-frame pose (breath / blink).
			// If every state has only one frame, fall back to `stand` (or
			// the first non-info state).
			nl::node chosen;
			size_t best_frames = 0;
			for (auto sub : src)
			{
				if (sub.name() == "info") continue;
				size_t n = 0;
				for (auto f : sub)
					if (f.data_type() == nl::node::type::bitmap) n++;
				if (n > best_frames)
				{
					best_frames = n;
					chosen = sub;
				}
			}
			if (!chosen || chosen.size() == 0)
				chosen = src["stand"];
			if (chosen.size() > 0)
				npc_anim = Animation(chosen);
		}
#endif

		storage_selection.clear();
		inventory_selection.clear();

		storage_equip.clear();
		storage_use.clear();
		storage_setup.clear();
		storage_etc.clear();
		storage_cash.clear();
		storage_icons.clear();
		inv_icons.clear();
		storage_slot_map.clear();
		inventory_slot_map.clear();
		storage_preferred_slot.clear();
		inventory_preferred_slot.clear();

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
		storage_selection.clear();
		inventory_selection.clear();
		storage_icons.clear();
		inv_icons.clear();
		storage_slot_map.clear();
		inventory_slot_map.clear();
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
		// Finalise any partial take-outs whose item just landed in the
		// player's inventory: find the new stack of that itemid in the
		// relevant tab, figure out how much arrived, and store back the
		// excess that the player didn't want to keep.
		for (auto it = pending_restores.begin(); it != pending_restores.end();)
		{
			if (it->tab != type) { ++it; continue; }

			int16_t slotmax = inventory.get_slotmax(type);
			int16_t found_slot = 0;
			int16_t found_count = 0;
			for (int16_t slot = 1; slot <= slotmax; ++slot)
			{
				if (inventory.get_item_id(type, slot) != it->itemid)
					continue;
				int16_t cnt = (type == InventoryType::Id::EQUIP)
					? 1 : inventory.get_item_count(type, slot);
				if (cnt > found_count)
				{
					found_slot = slot;
					found_count = cnt;
				}
			}

			if (found_slot > 0 && found_count > it->keep)
			{
				int16_t excess = found_count - it->keep;
				StorageStorePacket(found_slot, it->itemid, excess).dispatch();
				it = pending_restores.erase(it);
			}
			else
			{
				++it;
			}
		}

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
		int16_t total_cells = storage_cols * storage_rows;
		storage_slot_map.assign(total_cells, -1);

		for (int16_t i = 0; i < static_cast<int16_t>(items.size()); i++)
		{
			int16_t count = (current_tab == InventoryType::Id::EQUIP) ? -1 : items[i].count;
			const Texture& tex = ItemData::get(items[i].itemid).get_icon(false);

			storage_icons[i] = std::make_unique<Icon>(
				std::make_unique<StorageItemIcon>(i, current_tab),
				tex, count
			);

			// Try the player's preferred slot for this itemid; fall back
			// to the first empty visual cell.
			int16_t slot = -1;
			auto pref = storage_preferred_slot.find(items[i].itemid);
			if (pref != storage_preferred_slot.end()
				&& pref->second >= 0 && pref->second < total_cells
				&& storage_slot_map[pref->second] == -1)
			{
				slot = pref->second;
			}
			if (slot < 0)
			{
				for (int16_t s = 0; s < total_cells; s++)
				{
					if (storage_slot_map[s] == -1) { slot = s; break; }
				}
			}
			if (slot >= 0)
			{
				storage_slot_map[slot] = i;
				storage_preferred_slot[items[i].itemid] = slot;
			}
		}
	}

	void UIStorage::refresh_inventory_icons()
	{
		inv_icons.clear();

		int16_t total_cells = (inventory_cols > 0) ? inventory_cols * inventory_rows : 0;
		inventory_slot_map.assign(total_cells, -1);

		for (int16_t i = 0; i < static_cast<int16_t>(inventory_items.size()); i++)
		{
			const auto& entry = inventory_items[i];
			int16_t count = (current_tab == InventoryType::Id::EQUIP) ? -1 : entry.count;
			const Texture& tex = ItemData::get(entry.itemid).get_icon(false);

			inv_icons[i] = std::make_unique<Icon>(
				std::make_unique<PlayerItemIcon>(entry.slot, entry.itemid, entry.count),
				tex, count
			);

			int16_t slot = -1;
			auto pref = inventory_preferred_slot.find(entry.slot);
			if (pref != inventory_preferred_slot.end()
				&& pref->second >= 0 && pref->second < total_cells
				&& inventory_slot_map[pref->second] == -1)
			{
				slot = pref->second;
			}
			if (slot < 0)
			{
				for (int16_t s = 0; s < total_cells; s++)
				{
					if (inventory_slot_map[s] == -1) { slot = s; break; }
				}
			}
			if (slot >= 0)
			{
				inventory_slot_map[slot] = i;
				inventory_preferred_slot[entry.slot] = slot;
			}
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
		storage_selection.clear();
		inventory_selection.clear();

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

	void UIStorage::request_take_out_partial(int16_t index, int16_t keep)
	{
		const auto& items = get_storage_tab(current_tab);
		if (index < 0 || index >= static_cast<int16_t>(items.size()))
			return;

		const auto& entry = items[index];
		if (keep <= 0) return;
		if (keep >= entry.count)
		{
			request_take_out(index);
			return;
		}

		// Queue the restore before sending the take-out so the modify
		// callback has the info when the item lands in the inventory.
		PendingRestore pr{ entry.itemid, keep, current_tab };
		pending_restores.push_back(pr);

		request_take_out(index);
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

			UI::get().emplace<UIEnterNumber>("How many would you like to store?", on_enter, max_quantity, 1, -22);
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

		if (col >= storage_cols || row >= storage_rows) return -1;

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

		if (col >= inventory_cols || row >= inventory_rows) return -1;

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
