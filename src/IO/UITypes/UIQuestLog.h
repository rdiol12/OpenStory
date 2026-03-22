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
#include "../Components/Textfield.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Animation.h"

#include "../../Character/QuestLog.h"

namespace ms
{
	class UIQuestLog : public UIDragElement<PosQUEST>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::QUESTLOG;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIQuestLog(const QuestLog& questLog);

		void draw(float inter) const override;
		void update() override;
		void toggle_active() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicking, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		bool is_in_range(Point<int16_t> cursorpos) const override;

		UIElement::Type get_type() const override;

		void load_quests();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_tab(uint16_t tabid);
		void select_quest(int16_t index, bool from_recommended = false);

		// Safe button accessors (many buttons don't exist in v83)
		void set_btn_active(uint16_t id, bool active);
		void set_btn_state(uint16_t id, Button::State state);
		void set_btn_position(uint16_t id, Point<int16_t> pos);

		enum Buttons : uint16_t
		{
			TAB0,
			TAB1,
			TAB2,
			CLOSE,
			ALL_LEVEL,
			MY_LEVEL,
			BT_SEARCH,
			BT_NEXT,
			BT_PREV,
			BT_ALLLOCN,
			BT_MYLOCATION,
			BT_ICONINFO,
			GIVEUP,
			DETAIL,
			MARK_NPC,
			BT_ACCEPT,
			BT_FINISH,
			BT_DETAIL_CLOSE,
			BT_ARLIM,
			BT_DELIVERY_ACCEPT,
			BT_DELIVERY_COMPLETE,
			BT_NO,
			BT_ALERT,
			TAB3
		};

		static constexpr int16_t ROWS = 10;
		static constexpr int16_t ROW_HEIGHT = 24;
		static constexpr int16_t LIST_Y = 75;

		const QuestLog& questlog;

		uint16_t tab;
		uint16_t offset;
		std::vector<Sprite> notice_sprites;
		Textfield search;
		Text placeholder;
		Slider slider;
		Texture search_area;
		Texture quest_state_started;
		Texture quest_state_completed;
		Texture select_texture;
		Texture drop_texture;
		Animation quest_icon_anim;

		// Quest root icons
		Texture icon0;
		Texture icon1;
		Texture icon4;
		Texture icon10;
		Animation icon2_anim;
		Animation icon5_anim;
		Animation icon6_anim;
		Animation icon7_anim;
		Animation icon8_anim;
		Animation icon9_anim;
		Animation iconQM0_anim;
		Animation iconQM1_anim;

		// Gauge2 - quest progress bar (root level)
		Texture gauge2_frame;
		Texture gauge2_bar;
		Texture gauge2_spot;

		// TimeQuest
		Animation time_alarm_clock;
		Animation time_bar;

		// icon_info panel
		Texture icon_info_backgrnd;
		Texture icon_info_backgrnd2;
		Texture icon_info_sheet;
		bool show_icon_info;

		// List extras
		Texture complete_count;
		Texture recommend_title;
		Texture recommend_focus;

		struct QuestEntry
		{
			int16_t id;
			Text name;
		};

		std::vector<QuestEntry> available_entries;
		std::vector<QuestEntry> recommended_entries;
		std::vector<QuestEntry> active_entries;
		std::vector<QuestEntry> completed_entries;
		std::vector<QuestEntry> weekly_entries;
		bool filter_my_level;
		bool filter_my_location;
		bool show_recommended;
		std::string search_text;

		int16_t selected_entry;
		int16_t hover_entry;
		Point<int16_t> last_cursor_pos;
		bool selected_from_recommended;

		// Returns total virtual row count for TAB0 (headers + entries)
		uint16_t get_available_row_count() const;
		// Maps a virtual row index to what it represents
		enum class RowType { RECOMMEND_HEADER, RECOMMEND_ENTRY, AVAILABLE_HEADER, AVAILABLE_ENTRY };
		RowType get_row_type(int16_t row, int16_t& entry_index) const;
		// Select quest from a virtual row
		void select_row(int16_t row);

		// Detail panel
		bool show_detail;
		int16_t detail_scroll;
		mutable int16_t detail_content_height;
		Texture detail_backgrnd;
		Texture detail_backgrnd2;
		Texture detail_backgrnd3;
		Texture detail_summary;
		Texture detail_summary_pattern;
		Texture detail_tip;
		Texture detail_reward_icon;
		Text detail_quest_name;
		Text detail_quest_desc;
		Text detail_quest_summary;
		Text detail_level_req;

		// Detail panel gauge (quest progress)
		Texture detail_gauge_frame;
		Texture detail_gauge_bar;
		Texture detail_gauge_spot;
		float detail_progress;

		// NPC sprite
		Texture detail_npc_sprite;
		Text detail_npc_name;
		int32_t detail_npcid;

		// Reward/requirement items
		struct QuestItem
		{
			Texture icon;
			Text name;
			Text count;
		};
		std::vector<QuestItem> detail_rewards;
		std::vector<QuestItem> detail_requirements;

		// Quest job requirement text
		Text detail_job_req;

		// Prerequisite quest names
		std::vector<Text> detail_prereq_quests;

		// Say.img NPC dialog lines
		struct SayEntry
		{
			std::string text;
			bool has_yes_no;
		};
		std::vector<SayEntry> detail_say_lines;

		// PQuest.img data
		std::string detail_pquest_name;
		std::string detail_pquest_mark;

		// Exclusive.img medal
		std::string detail_medal;

		// QuestInfo.img extras
		int32_t detail_area;
		std::string detail_parent;
		int32_t detail_order;
		bool detail_auto_start;
		bool detail_auto_complete;
		int32_t detail_time_limit;

		// v83 root textures from UIWindow.img/Quest
		Texture basic_texture;
		Texture prob_texture;
		Texture reward_texture;
		Texture summary_texture;

		// Per-tab backgrounds (one per tab, never stacked)
		Texture list_backgrnd;
		Texture list_backgrnd2;
		Texture v83_backgrnd3;
		Texture v83_backgrnd4;
		Texture v83_backgrnd5;

		// notice animated set (0,1,2) from UIWindow.img/Quest/notice
		std::vector<Texture> notice_set;

		// QuestAlarm (UIWindow.img/QuestAlarm)
		Texture quest_alarm_bg_bottom;
		Texture quest_alarm_bg_center;
		Texture quest_alarm_bg_max;
		Texture quest_alarm_bg_min;
		Animation quest_alarm_anim;
		bool show_quest_alarm;

		// QuestIcon (UIWindow.img/QuestIcon) — animated status icons 0-11
		std::vector<Animation> quest_status_icons;

		// Pre-allocated draw objects (avoid per-frame GPU allocations)
		ColorBox tab_bg_active;
		ColorBox tab_bg_inactive;
		mutable Text tab_label_text;
		ColorBox hover_bg_recommend;
		ColorBox sel_bg_recommend;
		ColorBox hover_bg_normal;
		ColorBox sel_bg_normal;
		ColorBox sep_line_box;
		ColorBox header_sep_box;
		mutable Text say_preview_text;
		mutable Text medal_text;
		mutable Text time_text;
	};
}
