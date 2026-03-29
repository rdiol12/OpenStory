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
#include "UIEvent.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Constants.h"
#include "../../Data/ItemData.h"
#include "../../Net/Packets/GameplayPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIEvent::UIEvent() : UIDragElement<PosEVENT>(Point<int16_t>(250, 20))
	{
		offset = 0;
		selected_slot = 0;
		refresh_counter = 0;
		countdown_accumulator = 0;

		nl::node main = nl::nx::ui["UIWindow2.img"]["EventList"]["main"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node event_node = main["event"];

		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		// Layered backgrounds
		sprites.emplace_back(backgrnd);
		sprites.emplace_back(main["backgrnd2"], Point<int16_t>(1, 0));
		sprites.emplace_back(main["backgrnd3"], Point<int16_t>(6, 29));

		// Close button
		buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));

		// Event slot backgrounds (316x78)
		slot_normal = event_node["normal"];
		slot_selected = event_node["select"];

		// Item reward slot frame (35x35)
		slot_frame = event_node["slot"];

		// Event type icons (19x19)
		for (int i = 0; i < 4; i++)
			event_icons[i] = event_node["icon"][std::to_string(i)];

		// Status button textures
		btn_ing = event_node["BtIng"]["normal"]["0"];
		btn_will = event_node["BtWill"]["normal"]["0"];
		btn_clear = event_node["BtClear"]["normal"]["0"];

		// Text labels
		for (size_t i = 0; i < MAX_VISIBLE; i++)
		{
			event_title[i] = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK);
			event_desc[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
			event_time[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::JAPANESELAUREL);
		}

		empty_text = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::GRAY, "Requesting events...");

		// Load TimeEvent timer widget from UIWindow4.img/TimeEvent
		nl::node te = nl::nx::ui["UIWindow4.img"]["TimeEvent"];

		timer_bg = te["backgrnd"];
		timer_bg2 = te["backgrnd2"];
		timer_bg3 = te["backgrnd3"];

		// Gauge
		nl::node gauge = te["Guage"];
		timer_gauge_bg = gauge["backgrnd"];
		timer_gauge_cover = gauge["cover"];
		timer_gauge_fill = gauge["1"]["0"];
		gauge_width = Texture(gauge["backgrnd"]).get_dimensions().x();

		// Digit sprites 0-9 (~9x12)
		nl::node numbers = te["Number"];
		for (int i = 0; i < 10; i++)
			timer_digit[i] = numbers[std::to_string(i)];

		timer_digit_width = timer_digit[0].get_dimensions().x();

		// State overlays
		timer_state_timer = te["State"]["Timer"]["0"];
		timer_state_end = te["State"]["End"]["0"];
		timer_state_complete = te["State"]["Complete"]["0"];

		// Icon frame
		timer_icon_frame = te["icon"]["frame"];

		// Effect animation
		timer_effect = te["Effect"];

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		request_events();
	}

	void UIEvent::draw(float inter) const
	{
		UIElement::draw(inter);

		if (events.empty())
		{
			empty_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2));
			return;
		}

		for (int16_t i = 0; i < MAX_VISIBLE; i++)
		{
			int16_t slot = i + offset;

			if (slot >= static_cast<int16_t>(events.size()))
				break;

			const EventData& ev = events[slot];
			int16_t sy = SLOT_START_Y + SLOT_SPACING * i;

			auto slot_pos = position + Point<int16_t>(SLOT_X, sy);

			if (slot == selected_slot)
				slot_selected.draw(slot_pos);
			else
				slot_normal.draw(slot_pos);

			// Event type icon (19x19)
			if (ev.type >= 0 && ev.type < 4)
				event_icons[ev.type].draw(slot_pos + Point<int16_t>(6, 6));

			// Title text (next to icon)
			event_title[i].draw(slot_pos + Point<int16_t>(28, 6));

			// Status indicator top-right (inside slot, 57x32)
			int16_t btn_x = 316 - 57 - 4;
			if (ev.seconds_remaining > 0)
				btn_ing.draw(slot_pos + Point<int16_t>(btn_x, 4));
			else if (ev.seconds_remaining == 0)
				btn_clear.draw(slot_pos + Point<int16_t>(btn_x, 4));
			else
				btn_will.draw(slot_pos + Point<int16_t>(btn_x, 4));

			// Description
			event_desc[i].draw(slot_pos + Point<int16_t>(8, 26));

			// Compact inline gauge + time text
			if (ev.seconds_remaining > 0)
			{
				auto gauge_pos = slot_pos + Point<int16_t>(8, 44);
				timer_gauge_bg.draw(gauge_pos);

				if (ev.total_seconds > 0)
				{
					float ratio = static_cast<float>(ev.seconds_remaining) / static_cast<float>(ev.total_seconds);
					int16_t fill_width = static_cast<int16_t>(gauge_width * ratio);

					for (int16_t x = 0; x < fill_width; x++)
						timer_gauge_fill.draw(gauge_pos + Point<int16_t>(x, 0));
				}

				timer_gauge_cover.draw(gauge_pos);

				// Time digits right of gauge
				event_time[i].draw(slot_pos + Point<int16_t>(gauge_width + 14, 44));
			}
			else
			{
				event_time[i].draw(slot_pos + Point<int16_t>(8, 44));
			}

			// Item reward slots (compact, within slot bottom area)
			if (ev.has_item_rewards && !ev.rewards.empty())
			{
				for (size_t f = 0; f < ev.rewards.size() && f < 5; f++)
				{
					int16_t rx = 8 + 38 * static_cast<int16_t>(f);
					int16_t ry = 58;

					slot_frame.draw(slot_pos + Point<int16_t>(rx, ry));

					const ItemData& item_data = ItemData::get(ev.rewards[f].first);
					const Texture& icon = item_data.get_icon(true);
					icon.draw(slot_pos + Point<int16_t>(rx + 2, ry + 2));
				}
			}
		}
	}

	void UIEvent::draw_timer(Point<int16_t> pos, int32_t seconds_remaining, int32_t total_seconds) const
	{
		if (seconds_remaining <= 0)
		{
			// Draw "End" state
			timer_state_end.draw(pos);
			return;
		}

		// Draw "Timer" state label
		timer_state_timer.draw(pos);

		// Draw gauge bar below the state label
		auto gauge_pos = pos + Point<int16_t>(0, 20);
		timer_gauge_bg.draw(gauge_pos);

		// Fill the gauge based on remaining time
		if (total_seconds > 0)
		{
			float ratio = static_cast<float>(seconds_remaining) / static_cast<float>(total_seconds);
			int16_t fill_width = static_cast<int16_t>(gauge_width * ratio);

			// Draw fill segments
			for (int16_t x = 0; x < fill_width; x++)
				timer_gauge_fill.draw(gauge_pos + Point<int16_t>(x, 0));
		}

		timer_gauge_cover.draw(gauge_pos);

		// Draw countdown digits to the right of the gauge: MM:SS
		int32_t mins = seconds_remaining / 60;
		int32_t secs = seconds_remaining % 60;

		auto digit_pos = pos + Point<int16_t>(gauge_width + 5, 22);
		draw_timer_number(mins, 2, digit_pos);
		// Simple colon using two dots would need a sprite; just use spacing
		digit_pos = digit_pos + Point<int16_t>(2 * timer_digit_width + 2, 0);
		draw_timer_number(secs, 2, digit_pos);
	}

	void UIEvent::draw_timer_number(int value, int digits, Point<int16_t> pos) const
	{
		int divisor = 1;
		for (int i = 1; i < digits; i++)
			divisor *= 10;

		int16_t x = pos.x();

		for (int i = 0; i < digits; i++)
		{
			int d = (value / divisor) % 10;
			timer_digit[d].draw(DrawArgument(Point<int16_t>(x, pos.y())));
			x += timer_digit_width;
			divisor /= 10;
		}
	}

	void UIEvent::update()
	{
		UIElement::update();

		refresh_counter++;
		if (refresh_counter >= REFRESH_INTERVAL)
		{
			refresh_counter = 0;
			request_events();
		}

		// Local countdown decrement
		countdown_accumulator += Constants::TIMESTEP;
		if (countdown_accumulator >= 1000)
		{
			countdown_accumulator -= 1000;

			for (auto& ev : events)
			{
				if (ev.seconds_remaining > 0)
					ev.seconds_remaining--;
			}
		}

		// Update effect animation
		timer_effect.update();

		// Update displayed text
		for (int16_t i = 0; i < MAX_VISIBLE; i++)
		{
			int16_t slot = i + offset;

			if (slot >= static_cast<int16_t>(events.size()))
				break;

			const EventData& ev = events[slot];

			std::string title = ev.name;
			if (ev.multiplier > 100)
				title += " (" + std::to_string(ev.multiplier / 100) + "." + std::to_string((ev.multiplier % 100) / 10) + "x)";

			if (title.length() > 30)
				title = title.substr(0, 30) + "..";

			event_title[i].change_text(title);

			std::string desc = ev.description;
			if (desc.length() > 42)
				desc = desc.substr(0, 42) + "..";
			event_desc[i].change_text(desc);

			if (ev.seconds_remaining > 0)
			{
				int32_t mins = ev.seconds_remaining / 60;
				int32_t secs = ev.seconds_remaining % 60;
				std::string time_str = (mins < 10 ? "0" : "") + std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
				event_time[i].change_text(time_str);
			}
			else
			{
				event_time[i].change_text("Ended");
			}
		}

		if (events.empty())
			empty_text.change_text("No events running right now");
	}

	void UIEvent::set_events(std::vector<EventData> event_list)
	{
		// Set total_seconds for gauge on first receive
		for (auto& ev : event_list)
		{
			if (ev.total_seconds <= 0)
				ev.total_seconds = ev.seconds_remaining;
		}

		events = std::move(event_list);
		offset = 0;
		selected_slot = 0;
		countdown_accumulator = 0;
	}

	void UIEvent::request_events()
	{
		RequestEventInfoPacket().dispatch();
	}

	void UIEvent::remove_cursor()
	{
		UIDragElement::remove_cursor();
		UI::get().clear_tooltip(Tooltip::Parent::EVENT);
	}

	Cursor::State UIEvent::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;
		int16_t slot_idx = slot_by_position(cursoroffset.y());

		if (clicked && slot_idx >= 0)
			selected_slot = slot_idx + offset;

		// Item tooltip on hover
		if (slot_idx >= 0)
		{
			int16_t actual_slot = slot_idx + offset;
			if (actual_slot < static_cast<int16_t>(events.size()))
			{
				const EventData& ev = events[actual_slot];
				if (ev.has_item_rewards && !ev.rewards.empty())
				{
					int16_t ry = SLOT_START_Y + SLOT_SPACING * slot_idx + 58;

					if (cursoroffset.y() >= ry && cursoroffset.y() <= ry + 35)
					{
						for (size_t f = 0; f < ev.rewards.size() && f < 5; f++)
						{
							int16_t rx = SLOT_X + 8 + 38 * static_cast<int16_t>(f);
							if (cursoroffset.x() >= rx && cursoroffset.x() <= rx + 35)
							{
								UI::get().show_item(Tooltip::Parent::EVENT, ev.rewards[f].first);
								return UIDragElement::send_cursor(clicked, cursorpos);
							}
						}
					}
				}
			}
		}

		UI::get().clear_tooltip(Tooltip::Parent::EVENT);
		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIEvent::send_scroll(double yoffset)
	{
		if (events.empty())
			return;

		int16_t shift = (yoffset > 0) ? -1 : 1;
		int16_t new_offset = offset + shift;
		int16_t max_offset = std::max(0, static_cast<int16_t>(events.size()) - MAX_VISIBLE);

		if (new_offset >= 0 && new_offset <= max_offset)
			offset = new_offset;
	}

	void UIEvent::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close();
	}

	UIElement::Type UIEvent::get_type() const
	{
		return TYPE;
	}

	Button::State UIEvent::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::CLOSE:
			close();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIEvent::close()
	{
		deactivate();
	}

	int16_t UIEvent::slot_by_position(int16_t y)
	{
		for (int16_t i = 0; i < MAX_VISIBLE; i++)
		{
			int16_t top = SLOT_START_Y + SLOT_SPACING * i;
			if (y >= top && y < top + SLOT_SPACING)
				return i;
		}

		return -1;
	}
}
