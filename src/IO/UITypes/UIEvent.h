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
#include "../../Graphics/SpecialText.h"
#include "../../Graphics/Text.h"

#include <unordered_map>
#include <vector>

namespace ms
{
	struct EventData
	{
		int8_t type;
		std::string name;
		std::string description;
		int32_t seconds_remaining;
		int32_t total_seconds; // initial duration for gauge calculation
		int16_t multiplier;
		bool has_item_rewards;
		std::vector<std::pair<int32_t, int16_t>> rewards; // itemId, quantity
	};

	class UIEvent : public UIDragElement<PosEVENT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::EVENT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIEvent();

		void draw(float inter) const override;
		void update() override;

		void remove_cursor() override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void send_scroll(double yoffset) override;

		UIElement::Type get_type() const override;

		void set_events(std::vector<EventData> event_list);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		// Slot geometry comes from the art: event/normal is 316x78 with origin
		// (-11,-126), and its inner sprites are authored relative to the slot.
		static constexpr int16_t MAX_VISIBLE = 4;
		static constexpr int16_t SLOT_LEFT = 11;
		static constexpr int16_t SLOT_TOP = 126;
		static constexpr int16_t SLOT_HEIGHT = 78;
		static constexpr int16_t SLOT_SPACING = 82;

		static constexpr int16_t TEXT_X = 8;
		static constexpr int16_t TITLE_X = 28;
		static constexpr int16_t TITLE_Y = 6;
		static constexpr int16_t DESC_Y = 27;
		static constexpr int16_t GAUGE_Y = 60;
		static constexpr int16_t REWARD_Y = 41;
		static constexpr int16_t REWARD_PITCH = 38;
		static constexpr size_t MAX_REWARDS = 5;
		// backgrnd3 is the empty dark plaque at (11,88), 329x35, with BtCard parked
		// at its right end — the countdown digits centre in the space before it.
		static constexpr int16_t BAR_CENTER_X = 163;
		static constexpr int16_t BAR_CENTER_Y = 105;
		static constexpr int8_t SEPARATOR = -1;

		void close();
		void request_events();
		int16_t slot_by_position(int16_t y);
		int32_t soonest_remaining() const;
		void draw_countdown(Point<int16_t> centre, int32_t seconds) const;

		enum Buttons : uint16_t
		{
			CLOSE
		};

		int16_t offset;
		int16_t selected_slot;

		// Event slot backgrounds
		Texture slot_normal;
		Texture slot_selected;
		Texture slot_frame;
		Texture event_icons[4];

		// Status buttons
		Texture btn_ing;
		Texture btn_will;
		Texture btn_clear;

		// Text per visible slot
		Text event_title[MAX_VISIBLE];
		Text event_desc[MAX_VISIBLE];
		Text event_time[MAX_VISIBLE];

		std::vector<EventData> events;
		std::unordered_map<std::string, int32_t> peak_duration;
		Text empty_text;

		Texture clock_digit[10];
		Texture clock_colon;
		Texture timer_gauge_bg;
		Texture timer_gauge_cover;
		Texture timer_gauge_fill;
		int16_t gauge_width;
		Point<int16_t> gauge_offset;

		// Local countdown
		int64_t countdown_accumulator;

		int32_t refresh_counter;
		static constexpr int32_t REFRESH_INTERVAL = 1250;
	};
}
