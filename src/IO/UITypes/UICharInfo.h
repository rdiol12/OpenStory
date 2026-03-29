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

#include "../../Character/Char.h"
#include "../../Data/ItemData.h"

#include <vector>

namespace ms
{
	// Simple item entry for bot inventory display
	struct BotItem
	{
		int32_t item_id;
		int16_t count;
		int16_t slot;
	};

	struct BotInventoryData
	{
		int32_t char_id = 0;
		std::string name;
		int32_t level = 0;
		int32_t meso = 0;
		std::vector<BotItem> equip;     // inventoryType 1
		std::vector<BotItem> use;       // inventoryType 2
		std::vector<BotItem> setup;     // inventoryType 3
		std::vector<BotItem> etc;       // inventoryType 4
		std::vector<BotItem> equipped;  // inventoryType 5

		bool is_valid() const { return char_id != 0; }

		// idx: 0=Equip, 1=Use, 2=Setup, 3=Etc, 4=Equipped
		const std::vector<BotItem>& get_tab(int idx) const
		{
			switch (idx)
			{
			case 0: return equip;
			case 1: return use;
			case 2: return setup;
			case 3: return etc;
			case 4: return equipped;
			default: return equip;
			}
		}

		// invType for packets: 1=EQUIP, 2=USE, 3=SETUP, 4=ETC, 5=EQUIPPED
		int8_t get_inv_type(int idx) const
		{
			return static_cast<int8_t>(idx < 4 ? idx + 1 : 5);
		}
	};

	class UICharInfo : public UIDragElement<PosCHARINFO>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHARINFO;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UICharInfo(int32_t cid);

		void draw(float inter) const override;
		void update() override;

		Button::State button_pressed(uint16_t buttonid) override;

		bool is_in_range(Point<int16_t> cursorpos) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void remove_cursor() override;

		UIElement::Type get_type() const override;

		void update_stats(int32_t character_id, int16_t job_id, int8_t level, int16_t fame, std::string guild, std::string alliance);

		void set_bot_inventory(BotInventoryData data);
		int32_t get_char_id() const;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		void send_scroll(double yoffset) override;
		bool send_icon(const Icon& icon, Point<int16_t> cursorpos) override;

	private:
		void draw_bot_inventory(float inter) const;
		void draw_bot_item_grid(const std::vector<BotItem>& items, int16_t start_y) const;
		void load_bot_icons();
		int16_t bot_slot_by_position(Point<int16_t> cursor_relative) const;
		Point<int16_t> get_bot_slotpos(int16_t slot_index) const;
		Icon* get_bot_icon(int16_t slot_index);

		class BotItemIcon : public Icon::Type
		{
		public:
			BotItemIcon(int32_t bot_id, int8_t inv_type, int16_t slot, int32_t item_id, int16_t count, int8_t sub_tab);

			void drop_on_stage() const override;
			void drop_on_equips(EquipSlot::Id eqslot) const override;
			bool drop_on_items(InventoryType::Id tab, EquipSlot::Id eqslot, int16_t slot, bool equip) const override;
			void drop_on_bindings(Point<int16_t> cursorposition, bool remove) const override;
			void set_count(int16_t count) override;
			Icon::IconType get_type() override;
			int16_t get_source_slot() const override { return slot; }
			InventoryType::Id get_source_tab() const override { return InventoryType::Id::NONE; }

		private:
			int32_t bot_id;
			int8_t inv_type;
			int16_t slot;
			int32_t item_id;
			int16_t count;
			int8_t sub_tab;
		};

		enum Buttons : uint16_t
		{
			BtClose,
			BtFamily,
			BtParty,
			BtPet,
			BtRide,
			BtTrad,
			BtItem,
			BtPopUp,
			BtPopDown,
			BtBotEquip,
			BtBotUse,
			BtBotSetup,
			BtBotEtc,
			BtBotEquipped
		};

		/// Main Window
		Text name;
		Text job;
		Text level;
		Text fame;
		Text guild;
		Text alliance;

		Char* target_character;
		int32_t target_char_id;
		Point<int16_t> charinfo_dim;

		/// Bot Inventory
		BotInventoryData bot_inventory;
		bool bot_tab_active;
		int8_t bot_sub_tab;          // 0=Equip, 1=Use, 2=Setup, 3=Etc, 4=Equipped

		Text bot_name_label;
		Text bot_level_label;
		Text bot_meso_label;

		int16_t bot_scroll_offset;

		// NX sprites for bot inventory (reuse Item inventory assets)
		Texture bot_backgrnd;
		Texture bot_backgrnd2;
		Texture bot_backgrnd3;
		Texture bot_slot_disabled;

		std::map<int16_t, std::unique_ptr<Icon>> bot_icons;

		static constexpr int16_t BOT_GRID_X = 10;
		static constexpr int16_t BOT_GRID_Y = 51;
		static constexpr int16_t BOT_ICON_W = 36;
		static constexpr int16_t BOT_ICON_H = 35;
		static constexpr int16_t BOT_COLS = 4;
		static constexpr int16_t BOT_MAX_SLOTS = 24;
	};
}
