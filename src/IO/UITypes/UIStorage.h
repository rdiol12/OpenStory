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

#include "../../Data/ItemData.h"

namespace ms
{
	class UIStorage : public UIDragElement<PosSTORAGE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::STORAGE;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIStorage(const Inventory& inventory);

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> position) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void send_scroll(double yoffset) override;

		UIElement::Type get_type() const override;

		// Called by handler to set up storage contents
		void set_storage(int32_t npcid, int8_t slots, int32_t meso);
		// Add an item to the storage display
		void add_stored_item(int8_t invtype, int8_t slot, int32_t itemid, int16_t count);
		// Update mesos display
		void update_meso(int32_t meso);
		// Refresh after store/takeout
		void refresh_items();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void close_storage();
		void change_tab(InventoryType::Id type);
		int16_t slot_by_position(int16_t y) const;
		uint16_t tab_by_inv(InventoryType::Id type) const;

		enum Buttons : int16_t
		{
			BT_CLOSE,
			BT_TAKE_OUT,
			BT_STORE,
			BT_ARRANGE,
			BT_TAB_EQUIP,
			BT_TAB_USE,
			BT_TAB_SETUP,
			BT_TAB_ETC,
			BT_TAB_CASH,
			BT_SLOT0,
			BT_SLOT1,
			BT_SLOT2,
			BT_SLOT3,
			BT_SLOT4,
			BT_SLOT5,
			NUM_BUTTONS
		};

		struct StoredItem
		{
			int32_t itemid;
			int16_t count;
			int8_t slot;
			InventoryType::Id invtype;
			Texture icon;
			Text namelabel;
			Text countlabel;
		};

		const Inventory& inventory;

		Texture background;
		Texture tab_selected;
		Texture selection;
		Texture npc_sprite;
		Text mesolabel;
		Text slotlabel;
		Slider slider;

		std::vector<StoredItem> stored_items;
		std::vector<StoredItem*> display_items;

		int32_t storage_meso;
		int8_t storage_slots;
		int32_t npc_id;

		InventoryType::Id current_tab;
		int16_t offset;
		int16_t selected;

		Point<int16_t> lastcursorpos;
	};
}
