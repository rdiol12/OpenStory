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
#include "UIEquipInventory.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../UITypes/UIItemInventory.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"

#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIEquipInventory::UIEquipInventory(const Inventory& invent) : UIDragElement<PosEQINV>(Point<int16_t>(184, 20)), inventory(invent), tab(Buttons::BT_TAB1), hasPendantSlot(false), hasPocketSlot(false)
	{
		// UIWindow2 doll cells: cols x=10,43,76,109,142; rows y=27..225 (step 33)
		iconpositions[EquipSlot::Id::BOOK]     = Point<int16_t>(10, 27);
		iconpositions[EquipSlot::Id::HAT]      = Point<int16_t>(43, 27);
		iconpositions[EquipSlot::Id::BADGE]    = Point<int16_t>(76, 27);
		iconpositions[EquipSlot::Id::ANDROID]  = Point<int16_t>(109, 27);
		iconpositions[EquipSlot::Id::HEART]    = Point<int16_t>(142, 27);

		iconpositions[EquipSlot::Id::MEDAL]    = Point<int16_t>(10, 60);
		iconpositions[EquipSlot::Id::FACE]     = Point<int16_t>(43, 60);
		iconpositions[EquipSlot::Id::RING1]    = Point<int16_t>(109, 60);
		iconpositions[EquipSlot::Id::RING2]    = Point<int16_t>(142, 60);

		iconpositions[EquipSlot::Id::EYEACC]   = Point<int16_t>(43, 93);
		iconpositions[EquipSlot::Id::EARACC]   = Point<int16_t>(109, 93);
		iconpositions[EquipSlot::Id::SHOULDER] = Point<int16_t>(142, 93);

		iconpositions[EquipSlot::Id::CAPE]     = Point<int16_t>(10, 126);
		iconpositions[EquipSlot::Id::TOP]      = Point<int16_t>(43, 126);
		iconpositions[EquipSlot::Id::PENDANT1] = Point<int16_t>(76, 126);
		iconpositions[EquipSlot::Id::WEAPON]   = Point<int16_t>(109, 126);
		iconpositions[EquipSlot::Id::SHIELD]   = Point<int16_t>(142, 126);

		iconpositions[EquipSlot::Id::GLOVES]   = Point<int16_t>(10, 159);
		iconpositions[EquipSlot::Id::BOTTOM]   = Point<int16_t>(43, 159);
		iconpositions[EquipSlot::Id::BELT]     = Point<int16_t>(76, 159);
		iconpositions[EquipSlot::Id::RING3]    = Point<int16_t>(109, 159);
		iconpositions[EquipSlot::Id::RING4]    = Point<int16_t>(142, 159);

		iconpositions[EquipSlot::Id::SHOES]    = Point<int16_t>(76, 192);

		iconpositions[EquipSlot::Id::TAMEDMOB] = Point<int16_t>(10, 225);
		iconpositions[EquipSlot::Id::SADDLE]   = Point<int16_t>(43, 225);

		tab_source[Buttons::BT_TAB0] = "Equip";
		tab_source[Buttons::BT_TAB1] = "Cash";
		tab_source[Buttons::BT_TAB2] = "Pet";
		tab_source[Buttons::BT_TAB3] = "Android";

		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node character = nl::nx::ui["UIWindow2.img"]["Equip"]["character"];

		nl::node backgrnd = character["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(character["backgrnd2"]);
		sprites.emplace_back(character["backgrnd3"]);

		totem_dimensions = Point<int16_t>(0, 0);
		totem_adj = Point<int16_t>(0, 0);

		disabled = character["disabled"];

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 5));

		tab = Buttons::BT_TAB0;

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(bg_dimensions.x(), 20);

		load_icons();
	}

	void UIEquipInventory::draw(float alpha) const
	{
		UIElement::draw(alpha);

		for (auto iter : icons)
		{
			// Unmapped slots (emblem/pocket/etc — not on this doll) default
			// to (0,0); skip them so no icon stacks in the corner.
			Point<int16_t> ipos = iconpositions[iter.first];

			if (iter.second && (ipos.x() != 0 || ipos.y() != 0))
				iter.second->draw(position + ipos + Point<int16_t>(1, 1));
		}
	}

	Button::State UIEquipInventory::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case Buttons::BT_CLOSE:
			toggle_active();
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
			change_tab(id);

			return Button::State::IDENTITY;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIEquipInventory::update_slot(EquipSlot::Id slot)
	{
		// Forget a dangling drag reference before replacing/destroying the
		// icon (also lets the empty-slot branch actually free it — release()
		// leaked the icon to dodge exactly this dangling problem)
		if (icons[slot])
			UI::get().purge_icon(icons[slot].get());

		if (int32_t item_id = inventory.get_item_id(InventoryType::Id::EQUIPPED, slot))
		{
			const Texture& texture = ItemData::get(item_id).get_icon(false);

			icons[slot] = std::make_unique<Icon>(
				std::make_unique<EquipIcon>(slot),
				texture,
				-1
				);
		}
		else if (icons[slot])
		{
			icons[slot].reset();
		}

		clear_tooltip();
	}

	void UIEquipInventory::load_icons()
	{
		for (auto iter : EquipSlot::values)
			if (icons[iter])
				UI::get().purge_icon(icons[iter].get());

		icons.clear();

		for (auto iter : EquipSlot::values)
			update_slot(iter);
	}

	Cursor::State UIEquipInventory::send_cursor(bool pressed, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(pressed, cursorpos);

		if (dragged)
		{
			clear_tooltip();

			return dstate;
		}

		EquipSlot::Id slot = slot_by_position(cursorpos);

		if (auto icon = icons[slot].get())
		{
			if (pressed)
			{
				icon->start_drag(cursorpos - position - iconpositions[slot]);

				UI::get().drag_icon(icon);

				clear_tooltip();

				return Cursor::State::GRABBING;
			}
			else
			{
				show_equip(slot);

				return Cursor::State::CANGRAB;
			}
		}
		else
		{
			clear_tooltip();

			return Cursor::State::IDLE;
		}
	}

	void UIEquipInventory::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				toggle_active();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				uint16_t newtab = tab + 1;

				if (newtab >= Buttons::BT_TABE)
					newtab = Buttons::BT_TAB0;

				change_tab(newtab);
			}
		}
	}

	UIElement::Type UIEquipInventory::get_type() const
	{
		return TYPE;
	}

	void UIEquipInventory::doubleclick(Point<int16_t> cursorpos)
	{
		EquipSlot::Id slot = slot_by_position(cursorpos);

		if (icons[slot])
			if (int16_t freeslot = inventory.find_free_slot(InventoryType::Id::EQUIP))
				UnequipItemPacket(slot, freeslot).dispatch();
	}

	bool UIEquipInventory::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + dimension);

		Rectangle<int16_t> totem_bounds = Rectangle<int16_t>(position, position + totem_dimensions);
		totem_bounds.shift(totem_adj);

		return bounds.contains(cursorpos) || totem_bounds.contains(cursorpos);
	}

	bool UIEquipInventory::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (EquipSlot::Id slot = slot_by_position(cursorpos))
			icon.drop_on_equips(slot);

		return true;
	}

	void UIEquipInventory::toggle_active()
	{
		clear_tooltip();

		UIElement::toggle_active();
	}

	void UIEquipInventory::modify(int16_t pos, int8_t mode, int16_t arg)
	{
		EquipSlot::Id eqpos = EquipSlot::by_id(pos);
		EquipSlot::Id eqarg = EquipSlot::by_id(arg);

		switch (mode)
		{
		case 0:
		case 3:
			update_slot(eqpos);
			break;
		case 2:
			update_slot(eqpos);
			update_slot(eqarg);
			break;
		}
	}

	void UIEquipInventory::show_equip(EquipSlot::Id slot)
	{
		UI::get().show_equip(Tooltip::Parent::EQUIPINVENTORY, slot);
	}

	void UIEquipInventory::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::EQUIPINVENTORY);
	}

	EquipSlot::Id UIEquipInventory::slot_by_position(Point<int16_t> cursorpos) const
	{
		if (tab != Buttons::BT_TAB0)
			return EquipSlot::Id::NONE;

		for (auto iter : iconpositions)
		{
			Rectangle<int16_t> iconrect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(32, 32)
				);

			if (iconrect.contains(cursorpos))
				return iter.first;
		}

		return EquipSlot::Id::NONE;
	}

	void UIEquipInventory::change_tab(uint16_t tabid)
	{
		uint8_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			clear_tooltip();

			buttons[oldtab]->set_state(Button::State::NORMAL);
			buttons[tab]->set_state(Button::State::PRESSED);

			if (tab == Buttons::BT_TAB0)
				buttons[Buttons::BT_SLOT]->set_active(true);
			else
				buttons[Buttons::BT_SLOT]->set_active(false);

			if (tab == Buttons::BT_TAB2)
			{
				buttons[Buttons::BT_CONSUMESETTING]->set_active(true);
				buttons[Buttons::BT_EXCEPTION]->set_active(true);
			}
			else
			{
				buttons[Buttons::BT_CONSUMESETTING]->set_active(false);
				buttons[Buttons::BT_EXCEPTION]->set_active(false);
			}

			if (tab == Buttons::BT_TAB3)
				buttons[Buttons::BT_SHOP]->set_active(true);
			else
				buttons[Buttons::BT_SHOP]->set_active(false);
		}
	}

	UIEquipInventory::EquipIcon::EquipIcon(int16_t s)
	{
		source = s;
	}

	void UIEquipInventory::EquipIcon::drop_on_stage() const
	{
		Sound(Sound::Name::DRAGEND).play();
	}

	void UIEquipInventory::EquipIcon::drop_on_equips(EquipSlot::Id slot) const
	{
		if (source == slot)
			Sound(Sound::Name::DRAGEND).play();
	}

	bool UIEquipInventory::EquipIcon::drop_on_items(InventoryType::Id tab, EquipSlot::Id eqslot, int16_t slot, bool equip) const
	{
		if (tab != InventoryType::Id::EQUIP)
		{
			if (auto iteminventory = UI::get().get_element<UIItemInventory>())
			{
				if (iteminventory->is_active())
				{
					iteminventory->change_tab(InventoryType::Id::EQUIP);
					return false;
				}
			}
		}

		if (equip)
		{
			if (eqslot == source)
				EquipItemPacket(slot, eqslot).dispatch();
		}
		else
		{
			UnequipItemPacket(source, slot).dispatch();
		}

		return true;
	}

	Icon::IconType UIEquipInventory::EquipIcon::get_type()
	{
		return Icon::IconType::EQUIP;
	}
}