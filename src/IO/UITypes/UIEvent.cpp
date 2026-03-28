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
#include "UIClock.h"

#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
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

		// Panel is 351 wide, slot is 316 wide, centered: (351-316)/2 = 17
		// Slot area starts below header at y ~64
		// 3 slots with 125px spacing fits in the 458px tall panel

		for (int16_t i = 0; i < MAX_VISIBLE; i++)
		{
			int16_t slot = i + offset;

			if (slot >= static_cast<int16_t>(events.size()))
				break;

			const EventData& ev = events[slot];
			int16_t sy = SLOT_START_Y + SLOT_SPACING * i;

			// Slot background at centered x position
			auto slot_pos = position + Point<int16_t>(SLOT_X, sy);

			if (slot == selected_slot)
				slot_selected.draw(slot_pos);
			else
				slot_normal.draw(slot_pos);

			// Title text inside the slot
			event_title[i].draw(slot_pos + Point<int16_t>(8, 8));

			// Description below title
			event_desc[i].draw(slot_pos + Point<int16_t>(8, 28));

			// Event time status text
			event_time[i].draw(slot_pos + Point<int16_t>(8, 48));

			// Status indicator inside the slot, top-right area
			// Slot is 316 wide, button is 57 wide -> 316-57-5 = 254
			if (ev.seconds_remaining > 0)
				btn_ing.draw(slot_pos + Point<int16_t>(254, 6));
			else if (ev.seconds_remaining == 0)
				btn_clear.draw(slot_pos + Point<int16_t>(254, 6));
			else
				btn_will.draw(slot_pos + Point<int16_t>(254, 6));

			// Item reward slots at bottom of slot area
			if (ev.has_item_rewards && !ev.rewards.empty())
			{
				for (size_t f = 0; f < ev.rewards.size() && f < 5; f++)
				{
					int16_t rx = 8 + 38 * static_cast<int16_t>(f);
					int16_t ry = 60;

					slot_frame.draw(slot_pos + Point<int16_t>(rx, ry));

					const ItemData& item_data = ItemData::get(ev.rewards[f].first);
					const Texture& icon = item_data.get_icon(true);
					icon.draw(slot_pos + Point<int16_t>(rx + 2, ry + 2));
				}
			}
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
				event_time[i].change_text("In Progress");
			else
				event_time[i].change_text("Ended");
		}

		if (events.empty())
			empty_text.change_text("No events running right now");
	}

	void UIEvent::set_events(std::vector<EventData> event_list)
	{
		events = std::move(event_list);
		offset = 0;
		selected_slot = 0;

		// Set UIClock countdown to the first active event's time
		for (const auto& ev : events)
		{
			if (ev.seconds_remaining > 0)
			{
				Stage::get().set_countdown(ev.seconds_remaining);
				UI::get().emplace<UIClock>();
				break;
			}
		}
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
					int16_t ry = SLOT_START_Y + SLOT_SPACING * slot_idx + 60;

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
