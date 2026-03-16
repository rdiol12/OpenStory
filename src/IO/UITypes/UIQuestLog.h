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

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_tab(uint16_t tabid);
		void load_quests();
		void select_quest(int16_t index);

		enum Buttons : uint16_t
		{
			TAB0,
			TAB1,
			TAB2,
			CLOSE,
			SEARCH,
			ALL_LEVEL,
			MY_LOCATION
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

		struct QuestEntry
		{
			int16_t id;
			Text name;
		};

		std::vector<QuestEntry> active_entries;
		std::vector<QuestEntry> completed_entries;

		int16_t selected_entry;
		int16_t hover_entry;

		// Detail panel
		bool show_detail;
		Texture detail_backgrnd;
		Texture detail_backgrnd2;
		Texture detail_backgrnd3;
		Texture detail_summary;
		Texture detail_tip;
		Texture detail_reward_icon;
		Text detail_quest_name;
		Text detail_quest_desc;
		Text detail_quest_summary;
		Text detail_level_req;

		// NPC sprite
		Texture detail_npc_sprite;
		Text detail_npc_name;

		// Reward/requirement items
		struct QuestItem
		{
			Texture icon;
			Text name;
			Text count;
		};
		std::vector<QuestItem> detail_rewards;
		std::vector<QuestItem> detail_requirements;
	};
}
