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

#include "../Components/Charset.h"
#include "../Components/Slider.h"
#include "../Components/StatefulIcon.h"
#include "../Components/Textfield.h"

#include "../../Character/CharStats.h"
#include "../../Character/SkillBook.h"

namespace ms
{

	class UISkillBook : public UIDragElement<PosSKILL>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SKILLBOOK;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UISkillBook(const CharStats& stats, const SkillBook& skillbook);

		void draw(float alpha) const override;

		void toggle_active() override;
		void doubleclick(Point<int16_t> cursorpos) override;
		void remove_cursor() override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void update() override;

		bool send_icon(const Icon& icon, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		void update_stat(MapleStat::Id stat, int16_t value);
		void update_skills(int32_t skill_id);

		// Called by SkillMacrosHandler at login
		void load_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3);
		// Fire the 3 skills of the macro via combat.use_move. Called
		// when a hotkey bound to (KeyType::MACRO, index) is pressed.
		void trigger_macro(int32_t index);

	protected:
		Button::State button_pressed(uint16_t id) override;

	private:
		class SkillIcon : public StatefulIcon::Type
		{
		public:
			SkillIcon(int32_t skill_id);

			void drop_on_stage() const override {}
			void drop_on_equips(EquipSlot::Id) const override {}
			bool drop_on_items(InventoryType::Id, EquipSlot::Id, int16_t, bool) const override { return true; }
			void drop_on_bindings(Point<int16_t> cursorposition, bool remove) const override;
			void set_count(int16_t) override {}
			void set_state(StatefulIcon::State) override {}
			Icon::IconType get_type() override;
			int32_t get_action_id() const override { return skill_id; }

		private:
			int32_t skill_id;
		};

		// Drops into KeyConfig as a Mapping(KeyType::MACRO, macro_index)
		// so pressing that hotkey later fires the macro.
		class MacroIcon : public Icon::Type
		{
		public:
			MacroIcon(int32_t macro_index);

			void drop_on_stage() const override {}
			void drop_on_equips(EquipSlot::Id) const override {}
			bool drop_on_items(InventoryType::Id, EquipSlot::Id, int16_t, bool) const override { return true; }
			void drop_on_bindings(Point<int16_t> cursorposition, bool remove) const override;
			void set_count(int16_t) override {}
			Icon::IconType get_type() override { return Icon::IconType::MACRO; }
			int32_t get_action_id() const override { return macro_index; }

		private:
			int32_t macro_index;
		};

		class SkillDisplayMeta
		{
		public:
			SkillDisplayMeta(int32_t id, int32_t level);

			void draw(const DrawArgument& args) const;

			int32_t get_id() const;
			int32_t get_level() const;
			StatefulIcon* get_icon() const;

		private:
			int32_t id;
			int32_t level;
			std::unique_ptr<StatefulIcon> icon;
			Text name_text;
			Text level_text;
		};

		void change_job(uint16_t id);
		void change_sp();
		void change_tab(uint16_t new_tab);
		void change_offset(uint16_t new_offset);

		void show_skill(int32_t skill_id);
		void clear_tooltip();

		bool can_raise(int32_t skill_id) const;
		void send_spup(uint16_t row);
		void spend_sp(int32_t skill_id);

		Job::Level joblevel_by_tab(uint16_t tab) const;
		const UISkillBook::SkillDisplayMeta* skill_by_position(Point<int16_t> cursorpos) const;

		void close();
		bool check_required(int32_t id) const;

		void set_macro(bool enabled);

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_HYPER,
			BT_GUILDSKILL,
			BT_RIDE,
			BT_MACRO,
			BT_MACRO_OK,
			BT_TAB0,
			BT_TAB1,
			BT_TAB2,
			BT_TAB3,
			BT_TAB4,
			BT_SPUP0,
			BT_SPUP1,
			BT_SPUP2,
			BT_SPUP3
		};

		static constexpr int16_t ROWS = 4;
		static constexpr int16_t ROW_HEIGHT = 40;
		static constexpr Point<int16_t> SKILL_OFFSET = Point<int16_t>(11, 93);
		static constexpr Point<int16_t> ICON_OFFSET = Point<int16_t>(2, 2);
		static constexpr Point<int16_t> LINE_OFFSET = Point<int16_t>(2, 37);

		const CharStats& stats;
		const SkillBook& skillbook;

		Slider slider;
		Texture skille;
		Texture skilld;
		Texture skillb;
		Texture line;
		Texture bookicon;
		Text booktext;
		Text splabel;

		Job job;
		int16_t sp;
		int16_t beginner_sp;

		uint16_t tab;
		uint16_t skillcount;
		uint16_t offset;

		std::vector<SkillDisplayMeta> skills;
		bool grabbing;

		Point<int16_t> bg_dimensions;

		bool macro_enabled;
		Texture macro_backgrnd;
		Texture macro_backgrnd2;
		Texture macro_backgrnd3;

		// Inline macro editor controls (drawn on top of macro_backgrnd* when macro_enabled)
		static constexpr int16_t MACRO_COUNT = 3;
		Texture macro_slot;
		// Layout (relative to macro panel top-left = position + (bg_dimensions.x(), 0))
		Point<int16_t> macro_slots_origin;   // top-left of the 3x3 slot grid
		int16_t macro_slot_size;             // width/height of each slot
		int16_t macro_slot_gap;              // spacing between slots
		int16_t macro_row_gap;               // spacing between rows
		Point<int16_t> macro_name_origin;    // top-left of the single name field
		Point<int16_t> macro_name_size;      // width/height of name field
		int16_t selected_macro;              // which row the bottom name field edits
		Textfield macro_name_field;          // single shared bottom field
		std::string macro_names[MACRO_COUNT];
		bool macro_shouts[MACRO_COUNT];
		Texture macro_check_sprite;
		Texture macro_select_sprite;
		Texture macro_handle_sprites[MACRO_COUNT];   // per-row macro icons
		std::unique_ptr<Icon> macro_row_icons[MACRO_COUNT];
		// Queue of skill ids pending from a triggered macro — fired one
		// at a time in update() once the player can attack again.
		std::vector<int32_t> pending_macro_skills;
		int32_t macro_skills[MACRO_COUNT][3];

		void macro_select_row(int16_t row);
		void save_macros();
	};
}