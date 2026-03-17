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
	// Quest helper/tracker widget — shows active quest progress on screen
	// Uses QuestAlarm NX nodes from UIWindow.img and UIWindow2.img
	class UIQuestHelper : public UIDragElement<PosQUESTHELPER>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::QUESTHELPER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIQuestHelper(const QuestLog& questLog);

		void draw(float inter) const override;
		void update() override;
		void toggle_active() override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Track a specific quest
		void track_quest(int16_t questid);
		// Auto-select a quest to track
		void auto_track();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void refresh_quest_info();

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

		// QuestAlarm backgrounds (scalable)
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

		// QuestGuide marks (forMiniMap variants for UI display)
		Animation quest_mark;
		Animation repeat_quest_mark;
		Animation high_lv_mark;
		Animation low_lv_mark;
		Animation low_lv_repeat_mark;

		// StatusBar3/Quest nodes
		Texture sb_checkarlim_box;
		Texture sb_checkarlim_check;
		Texture sb_quest_info_bg;
		Texture sb_quest_info_bg2;
		Texture sb_quest_info_bg3;
		Texture sb_quest_info_tip;

		// Extended QuestIcon (UIWindow2.img/QuestIcon 12-29)
		std::vector<Animation> extended_quest_icons;

		// Tracked quest state
		int16_t tracked_questid;
		bool minimized;
		bool show_bulb;

		// Quest display info
		Text quest_name;
		Text quest_progress;
		std::vector<Text> requirement_lines;
	};
}
