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
#include "UIStatusMessenger.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/StoragePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIStorage::UIStorage(const Inventory& inv) : UIDragElement<PosSTORAGE>(), inventory(inv)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["Trunk"];

		// Load background sprites
		nl::node backgrnd = src["backgrnd"];
		background = backgrnd;

		auto bg_dim = background.get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];
		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = src["backgrnd3"];
		if (backgrnd3.size() > 0)
			sprites.emplace_back(backgrnd3);

		// Buttons — Trunk NX uses: BtExit, BtGet, BtPut, BtSort
		buttons[BT_CLOSE] = std::make_unique<MapleButton>(src["BtExit"]);

		nl::node bt_take = src["BtGet"];
		if (bt_take.size() > 0)
			buttons[BT_TAKE_OUT] = std::make_unique<MapleButton>(bt_take);
		else
			buttons[BT_TAKE_OUT] = std::make_unique<AreaButton>(Point<int16_t>(10, bg_dim.y() - 40), Point<int16_t>(80, 24));

		nl::node bt_store = src["BtPut"];
		if (bt_store.size() > 0)
			buttons[BT_STORE] = std::make_unique<MapleButton>(bt_store);
		else
			buttons[BT_STORE] = std::make_unique<AreaButton>(Point<int16_t>(100, bg_dim.y() - 40), Point<int16_t>(80, 24));

		nl::node bt_arrange = src["BtSort"];
		if (bt_arrange.size() > 0)
			buttons[BT_ARRANGE] = std::make_unique<MapleButton>(bt_arrange);
		else
			buttons[BT_ARRANGE] = std::make_unique<AreaButton>(Point<int16_t>(190, bg_dim.y() - 40), Point<int16_t>(80, 24));

		// Tab buttons
		nl::node tab_en = src["Tab"]["enabled"];
		nl::node tab_dis = src["Tab"]["disabled"];

		if (tab_en.size() > 0 && tab_dis.size() > 0)
		{
			buttons[BT_TAB_EQUIP] = std::make_unique<TwoSpriteButton>(tab_dis["0"], tab_en["0"]);
			buttons[BT_TAB_USE] = std::make_unique<TwoSpriteButton>(tab_dis["1"], tab_en["1"]);
			buttons[BT_TAB_SETUP] = std::make_unique<TwoSpriteButton>(tab_dis["2"], tab_en["2"]);
			buttons[BT_TAB_ETC] = std::make_unique<TwoSpriteButton>(tab_dis["3"], tab_en["3"]);
			buttons[BT_TAB_CASH] = std::make_unique<TwoSpriteButton>(tab_dis["4"], tab_en["4"]);
		}
		else
		{
			// Fallback: create area buttons for tabs across the top
			int16_t tab_w = bg_dim.x() / 5;
			for (int i = 0; i < 5; i++)
				buttons[BT_TAB_EQUIP + i] = std::make_unique<AreaButton>(Point<int16_t>(7 + tab_w * i, 28), Point<int16_t>(tab_w - 2, 20));
		}

		// Item slot buttons (6 visible at a time)
		int16_t item_y = 70;
		int16_t item_h = 42;

		for (int i = 0; i < 6; i++)
		{
			Point<int16_t> pos(10, item_y + item_h * i);
			Point<int16_t> dim(bg_dim.x() - 40, item_h - 4);
			buttons[BT_SLOT0 + i] = std::make_unique<AreaButton>(pos, dim);
		}

		// Selection highlight
		selection = src["select"];
		tab_selected = src["tabSel"];

		// Labels
		mesolabel = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::MINESHAFT);
		slotlabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MINESHAFT);

		// Slider
		slider = Slider(
			Slider::Type::DEFAULT_SILVER, Range<int16_t>(70, 70 + item_h * 6), bg_dim.x() - 20, 6, 1,
			[&](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				int16_t new_offset = offset + shift;
				int16_t max_offset = static_cast<int16_t>(display_items.size()) - 6;

				if (new_offset >= 0 && new_offset <= max_offset)
					offset = new_offset;
			}
		);

		storage_meso = 0;
		storage_slots = 0;
		npc_id = 0;
		current_tab = InventoryType::Id::EQUIP;
		offset = 0;
		selected = -1;

		dimension = bg_dim;
		dragarea = Point<int16_t>(bg_dim.x(), 20);

		active = false;
	}

	void UIStorage::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Draw NPC sprite
		if (npc_sprite.is_valid())
			npc_sprite.draw(DrawArgument(position + Point<int16_t>(50, 45), true));

		// Draw meso label
		mesolabel.draw(position + Point<int16_t>(dimension.x() - 30, 48));

		// Draw slot count
		slotlabel.draw(position + Point<int16_t>(15, 48));

		// Draw items
		int16_t item_y = 70;
		int16_t item_h = 42;

		for (int16_t i = 0; i < 6; i++)
		{
			int16_t idx = i + offset;
			if (idx < 0 || idx >= static_cast<int16_t>(display_items.size()))
				break;

			StoredItem* item = display_items[idx];
			Point<int16_t> itempos = position + Point<int16_t>(12, item_y + item_h * i);

			// Draw selection highlight
			if (idx == selected && selection.is_valid())
				selection.draw(itempos + Point<int16_t>(0, 4));

			// Draw item icon
			if (item->icon.is_valid())
				item->icon.draw(itempos + Point<int16_t>(2, item_h - 4));

			// Draw item name and count
			item->namelabel.draw(itempos + Point<int16_t>(40, 6));
			item->countlabel.draw(itempos + Point<int16_t>(40, 22));
		}

		// Draw slider
		slider.draw(position);
	}

	void UIStorage::update()
	{
		std::string mesostr = std::to_string(storage_meso);
		string_format::split_number(mesostr);
		mesolabel.change_text(mesostr + " meso");

		int16_t used = static_cast<int16_t>(stored_items.size());
		slotlabel.change_text(std::to_string(used) + "/" + std::to_string(storage_slots));
	}

	Button::State UIStorage::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			close_storage();
			return Button::State::NORMAL;

		case BT_TAKE_OUT:
		{
			if (selected >= 0 && selected < static_cast<int16_t>(display_items.size()))
			{
				StoredItem* item = display_items[selected];

				// Find this item's position in the current tab's filtered list
				int8_t tab_slot = 0;
				for (auto& si : stored_items)
				{
					if (si.invtype == current_tab)
					{
						if (&si == item)
							break;
						tab_slot++;
					}
				}

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

				StorageTakeOutPacket(invtype_byte, tab_slot).dispatch();
				selected = -1;
			}
			return Button::State::NORMAL;
		}

		case BT_STORE:
		{
			// Store currently selected inventory item — for now just send arrange as a placeholder
			// Real implementation would need a way to select from player inventory
			// Show notice
			auto messenger = UI::get().get_element<UIStatusMessenger>();
			if (messenger)
				messenger->show_status(Color::Name::WHITE, "Drag an item from your inventory to store it.");

			return Button::State::NORMAL;
		}

		case BT_ARRANGE:
			StorageArrangePacket().dispatch();
			return Button::State::NORMAL;

		case BT_TAB_EQUIP:
			change_tab(InventoryType::Id::EQUIP);
			return Button::State::IDENTITY;

		case BT_TAB_USE:
			change_tab(InventoryType::Id::USE);
			return Button::State::IDENTITY;

		case BT_TAB_SETUP:
			change_tab(InventoryType::Id::SETUP);
			return Button::State::IDENTITY;

		case BT_TAB_ETC:
			change_tab(InventoryType::Id::ETC);
			return Button::State::IDENTITY;

		case BT_TAB_CASH:
			change_tab(InventoryType::Id::CASH);
			return Button::State::IDENTITY;

		default:
			if (buttonid >= BT_SLOT0 && buttonid <= BT_SLOT5)
			{
				int16_t slot = (buttonid - BT_SLOT0) + offset;

				if (slot >= 0 && slot < static_cast<int16_t>(display_items.size()))
				{
					if (slot == selected)
					{
						// Double click = take out
						button_pressed(BT_TAKE_OUT);
					}
					else
					{
						selected = slot;
					}
				}

				return Button::State::NORMAL;
			}
			break;
		}

		return Button::State::NORMAL;
	}

	Cursor::State UIStorage::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;
		lastcursorpos = cursoroffset;

		if (slider.isenabled())
		{
			Cursor::State sstate = slider.send_cursor(cursoroffset, clicked);
			if (sstate != Cursor::State::IDLE)
				return sstate;
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIStorage::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close_storage();
	}

	void UIStorage::send_scroll(double yoffset)
	{
		if (slider.isenabled())
			slider.send_scroll(yoffset);
	}

	UIElement::Type UIStorage::get_type() const
	{
		return TYPE;
	}

	void UIStorage::set_storage(int32_t npcid, int8_t slots, int32_t meso)
	{
		npc_id = npcid;
		storage_slots = slots;
		storage_meso = meso;

		stored_items.clear();
		display_items.clear();
		offset = 0;
		selected = -1;

		// Load NPC sprite
		std::string strid = string_format::extend_id(npcid, 7);
		npc_sprite = nl::nx::npc[strid + ".img"]["stand"]["0"];

		// Set equip tab as default
		current_tab = InventoryType::Id::EQUIP;

		for (uint16_t i = BT_TAB_EQUIP; i <= BT_TAB_CASH; i++)
			buttons[i]->set_state(Button::State::NORMAL);

		buttons[BT_TAB_EQUIP]->set_state(Button::State::PRESSED);

		makeactive();
	}

	void UIStorage::add_stored_item(int8_t invtype_byte, int8_t slot, int32_t itemid, int16_t count)
	{
		InventoryType::Id invtype;
		switch (invtype_byte)
		{
		case 1: invtype = InventoryType::Id::EQUIP; break;
		case 2: invtype = InventoryType::Id::USE; break;
		case 3: invtype = InventoryType::Id::SETUP; break;
		case 4: invtype = InventoryType::Id::ETC; break;
		case 5: invtype = InventoryType::Id::CASH; break;
		default: invtype = InventoryType::Id::ETC; break;
		}

		StoredItem si;
		si.itemid = itemid;
		si.count = count;
		si.slot = slot;
		si.invtype = invtype;

		const ItemData& idata = ItemData::get(itemid);
		si.icon = idata.get_icon(false);

		std::string name = idata.get_name();
		if (name.length() > 30)
			name = name.substr(0, 30) + "..";

		si.namelabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MINESHAFT, name);

		if (count > 1)
			si.countlabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "x" + std::to_string(count));
		else
			si.countlabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);

		stored_items.push_back(std::move(si));
	}

	void UIStorage::update_meso(int32_t meso)
	{
		storage_meso = meso;
	}

	void UIStorage::refresh_items()
	{
		change_tab(current_tab);
	}

	void UIStorage::close_storage()
	{
		StorageClosePacket().dispatch();
		deactivate();
	}

	void UIStorage::change_tab(InventoryType::Id type)
	{
		// Update button states
		uint16_t old_tab = tab_by_inv(current_tab);
		if (old_tab > 0)
			buttons[old_tab]->set_state(Button::State::NORMAL);

		current_tab = type;

		uint16_t new_tab = tab_by_inv(current_tab);
		if (new_tab > 0)
			buttons[new_tab]->set_state(Button::State::PRESSED);

		// Filter items by tab
		display_items.clear();
		for (auto& si : stored_items)
		{
			if (si.invtype == current_tab)
				display_items.push_back(&si);
		}

		offset = 0;
		selected = -1;

		slider.setrows(6, static_cast<int16_t>(display_items.size()));
	}

	int16_t UIStorage::slot_by_position(int16_t y) const
	{
		int16_t yoff = y - 70;
		if (yoff < 0)
			return -1;

		int16_t slot = yoff / 42;
		if (slot >= 0 && slot < 6)
			return slot;

		return -1;
	}

	uint16_t UIStorage::tab_by_inv(InventoryType::Id type) const
	{
		switch (type)
		{
		case InventoryType::Id::EQUIP: return BT_TAB_EQUIP;
		case InventoryType::Id::USE: return BT_TAB_USE;
		case InventoryType::Id::SETUP: return BT_TAB_SETUP;
		case InventoryType::Id::ETC: return BT_TAB_ETC;
		case InventoryType::Id::CASH: return BT_TAB_CASH;
		default: return 0;
		}
	}
}
