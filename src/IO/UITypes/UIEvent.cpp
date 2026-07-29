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
#include "../../Gameplay/Stage.h"
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

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(main["backgrnd2"]);
		sprites.emplace_back(main["backgrnd3"]);

		// Close button
		buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));

		slot_normal = event_node["normal"];
		slot_selected = event_node["select"];
		slot_frame = event_node["slot"];

		for (int i = 0; i < 4; i++)
			event_icons[i] = event_node["icon"][std::to_string(i)];

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

		nl::node clock_num = nl::nx::ui["UIWindow.img"]["muruengRaid"]["number"];

		for (int i = 0; i < 10; i++)
			clock_digit[i] = clock_num[std::to_string(i)];

		clock_colon = clock_num["bar"];

		nl::node gauge = nl::nx::ui["UIWindow4.img"]["TimeEvent"]["Guage"];
		timer_gauge_bg = gauge["backgrnd"];
		timer_gauge_cover = gauge["cover"];
		timer_gauge_fill = gauge["1"]["0"];
		gauge_width = timer_gauge_bg.get_dimensions().x();
		gauge_offset = -timer_gauge_bg.get_origin();

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		request_events();
	}

	void UIEvent::draw(float inter) const
	{
		UIElement::draw(inter);

		draw_countdown(position + Point<int16_t>(BAR_CENTER_X, BAR_CENTER_Y), soonest_remaining());

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
			int16_t sy = SLOT_SPACING * i;

			// The slot art carries origin (-11,-126), so drawing it at the window
			// position places it; inner sprites are authored relative to the slot.
			auto slot_pos = position + Point<int16_t>(0, sy);
			auto content = position + Point<int16_t>(SLOT_LEFT, SLOT_TOP + sy);

			if (slot == selected_slot)
				slot_selected.draw(slot_pos);
			else
				slot_normal.draw(slot_pos);

			if (ev.type >= 0 && ev.type < 4)
				event_icons[ev.type].draw(content);

			event_title[i].draw(content + Point<int16_t>(TITLE_X, TITLE_Y));

			if (ev.seconds_remaining > 0)
				btn_ing.draw(content);
			else if (ev.seconds_remaining == 0)
				btn_clear.draw(content);
			else
				btn_will.draw(content);


			event_desc[i].draw(content + Point<int16_t>(TEXT_X, DESC_Y));

			bool has_rewards = ev.has_item_rewards && !ev.rewards.empty();

			if (has_rewards)
			{
				for (size_t f = 0; f < ev.rewards.size() && f < MAX_REWARDS; f++)
				{
					auto frame_pos = content + Point<int16_t>(TEXT_X + REWARD_PITCH * static_cast<int16_t>(f), REWARD_Y);

					slot_frame.draw(frame_pos);

					const ItemData& item_data = ItemData::get(ev.rewards[f].first);
					item_data.get_icon(true).draw(frame_pos + Point<int16_t>(2, 2));
				}

			}
			else if (ev.seconds_remaining > 0)
			{
				timer_gauge_bg.draw(content);

				if (ev.total_seconds > 0)
				{
					float ratio = static_cast<float>(ev.seconds_remaining) / static_cast<float>(ev.total_seconds);
					int16_t fill_width = static_cast<int16_t>(gauge_width * ratio);

					for (int16_t x = 0; x < fill_width; x++)
						timer_gauge_fill.draw(content + gauge_offset + Point<int16_t>(x, 0));
				}

				timer_gauge_cover.draw(content);
			}
			else
			{
				event_time[i].draw(content + Point<int16_t>(TEXT_X, GAUGE_Y));
			}
		}
	}

	int32_t UIEvent::soonest_remaining() const
	{
		int32_t soonest = 0;

		for (const auto& ev : events)
			if (ev.seconds_remaining > 0 && (soonest == 0 || ev.seconds_remaining < soonest))
				soonest = ev.seconds_remaining;

		return soonest;
	}

	void UIEvent::draw_countdown(Point<int16_t> centre, int32_t seconds) const
	{
		if (seconds < 0)
			seconds = 0;

		int32_t hours = seconds / 3600;
		int32_t mins = (seconds % 3600) / 60;
		int32_t secs = seconds % 60;

		int8_t glyphs[10];
		int count = 0;

		auto push = [&](int32_t value, bool pad)
		{
			if (pad || value >= 10)
				glyphs[count++] = static_cast<int8_t>((value / 10) % 10);

			glyphs[count++] = static_cast<int8_t>(value % 10);
		};

		if (hours > 0)
		{
			push(hours, false);
			glyphs[count++] = SEPARATOR;
		}

		push(mins, true);
		glyphs[count++] = SEPARATOR;
		push(secs, true);

		auto glyph = [&](int i) -> const Texture&
		{
			return (glyphs[i] == SEPARATOR) ? clock_colon : clock_digit[glyphs[i]];
		};

		int16_t total = 0;

		for (int i = 0; i < count; i++)
			total += glyph(i).get_dimensions().x();

		int16_t x = centre.x() - total / 2;

		for (int i = 0; i < count; i++)
		{
			const Texture& tex = glyph(i);
			Point<int16_t> dim = tex.get_dimensions();

			// these sprites carry centred origins, so offset by the origin to
			// land the top-left where we want it
			tex.draw(DrawArgument(Point<int16_t>(x, centre.y() - dim.y() / 2) + tex.get_origin()));
			x += dim.x();
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
		// EVENT_INFO carries only secondsRemaining, so the gauge denominator is
		// the largest value we have ever seen for that event, kept across refreshes
		for (auto& ev : event_list)
		{
			int32_t seen = peak_duration[ev.name];

			if (ev.seconds_remaining > seen)
			{
				seen = ev.seconds_remaining;
				peak_duration[ev.name] = seen;
			}

			ev.total_seconds = seen;
		}

		events = std::move(event_list);

		if (offset > 0 && offset > static_cast<int16_t>(events.size()) - MAX_VISIBLE)
			offset = std::max(0, static_cast<int16_t>(events.size()) - MAX_VISIBLE);

		if (selected_slot >= static_cast<int16_t>(events.size()))
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
					int16_t ry = SLOT_TOP + SLOT_SPACING * slot_idx + REWARD_Y;

					if (cursoroffset.y() >= ry && cursoroffset.y() <= ry + 35)
					{
						for (size_t f = 0; f < ev.rewards.size() && f < MAX_REWARDS; f++)
						{
							int16_t rx = SLOT_LEFT + TEXT_X + REWARD_PITCH * static_cast<int16_t>(f);
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
			int16_t top = SLOT_TOP + SLOT_SPACING * i;
			if (y >= top && y < top + SLOT_HEIGHT)
				return i;
		}

		return -1;
	}
}
