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

#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"
#include "../../Character/QuestLog.h"

namespace ms
{
	class UIQuestHelper : public UIDragElement<PosQUESTHELPER>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::QUESTHELPER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		static constexpr int MAX_TRACKED = 5;

		UIQuestHelper(const QuestLog& questLog);

		void draw(float inter) const override;
		void update() override;
		void toggle_active() override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicking, Point<int16_t> cursorpos) override;
		bool is_in_range(Point<int16_t> cursorpos) const override;

		UIElement::Type get_type() const override;

		void track_quest(int16_t questid);
		void untrack_quest(int16_t questid);
		void auto_track();
		void refresh_all();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		struct TrackedQuest
		{
			int16_t questid;
			Text name;
			Text progress;
			std::vector<Text> requirements;
			bool collapsed;

			TrackedQuest() : questid(-1), collapsed(false) {}
		};

		void refresh_quest_info(TrackedQuest& tq);
		void recalc_dimension();
		int16_t draw_quest_entry(const TrackedQuest& tq, Point<int16_t> pos, float alpha) const;
		int16_t get_quest_entry_height(const TrackedQuest& tq) const;

		enum Buttons : uint16_t
		{
			BT_Q,
			BT_AUTO,
			BT_DELETE,
			BT_MAX,
			BT_MIN,
			BT_BULB_OPEN,
			BT_BULB_CLOSE
		};

		const QuestLog& questlog;

		// QuestAlarm backgrounds
		Texture bg_min;
		Texture bg_center;
		Texture bg_bottom;
		Texture bg_max;

		// QuestBulb animations
		Animation bulb_open_anim;
		Animation bulb_close_anim;

		// QuestMessenger
		Texture messenger_alice_bg;
		Texture messenger_alice_notice;
		Texture messenger_thomas_bg;
		Texture messenger_thomas_notice;

		// Per-quest close button textures (from BtDelete)
		Texture close_btn_normal;
		Texture close_btn_mouseover;
		Texture close_btn_pressed;

		// QuestGuide marks
		Animation quest_mark;
		Animation repeat_quest_mark;
		Animation high_lv_mark;
		Animation low_lv_mark;
		Animation low_lv_repeat_mark;

		// StatusBar3/Quest
		Texture sb_checkarlim_box;
		Texture sb_checkarlim_check;
		Texture sb_quest_info_bg;
		Texture sb_quest_info_bg2;
		Texture sb_quest_info_bg3;
		Texture sb_quest_info_tip;

		// Extended QuestIcon
		std::vector<Animation> extended_quest_icons;

		// Multi-quest tracking
		std::vector<TrackedQuest> tracked_quests;
		bool minimized;
		bool show_messenger;
		Point<int16_t> expanded_dimension;

		// Hit test positions for per-quest X buttons and collapse toggles (set during draw)
		struct QuestHitArea
		{
			int16_t questid;
			Rectangle<int16_t> close_btn;
			Rectangle<int16_t> header_area;
		};
		mutable std::vector<QuestHitArea> quest_hit_areas;
		int16_t hovered_close_questid;

		// Pre-allocated draw objects
		mutable Text title_text;
		Text no_quest_text;
	};
}
