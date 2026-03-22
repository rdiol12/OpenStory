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
#include "UIWhisper.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"

#include "../../Net/Packets/BuddyPackets.h"

namespace ms
{
	UIWhisper::UIWhisper(const std::string& target) :
		UIDragElement<PosWHISPER>(Point<int16_t>(WIDTH, HEADER_HEIGHT)),
		target_name(target),
		scroll_offset(0)
	{
		int16_t total_h = HEADER_HEIGHT + CHAT_HEIGHT + INPUT_HEIGHT + 4;

		background = ColorBox(WIDTH, total_h, Color::Name::BLACK, 0.85f);
		header_bg = ColorBox(WIDTH, HEADER_HEIGHT, Color::Name::EMPEROR, 0.9f);
		input_bg = ColorBox(WIDTH - 60, INPUT_HEIGHT, Color::Name::GALLERY, 1.0f);

		title_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		update_title();

		// Name input field at the top (for entering target name)
		namefield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::WHITE,
			Rectangle<int16_t>(Point<int16_t>(40, 5), Point<int16_t>(WIDTH - 10, 20)),
			15
		);

		if (!target_name.empty())
			namefield.change_text(target_name);

		// Chat input field at the bottom
		chatfield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(5, HEADER_HEIGHT + CHAT_HEIGHT + 2),
				Point<int16_t>(WIDTH - 60, HEADER_HEIGHT + CHAT_HEIGHT + INPUT_HEIGHT)
			),
			100
		);

		// Buttons
		buttons[BT_CLOSE] = std::make_unique<AreaButton>(
			Point<int16_t>(WIDTH - 18, 3), Point<int16_t>(15, 15)
		);

		buttons[BT_SEND] = std::make_unique<AreaButton>(
			Point<int16_t>(WIDTH - 55, HEADER_HEIGHT + CHAT_HEIGHT + 2),
			Point<int16_t>(50, INPUT_HEIGHT)
		);

		buttons[BT_FIND] = std::make_unique<AreaButton>(
			Point<int16_t>(5, 3), Point<int16_t>(30, 18)
		);

		// Pre-allocate draw objects
		close_x = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "X");
		send_bg = ColorBox(50, INPUT_HEIGHT, Color::Name::MALIBU, 0.8f);
		send_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Send");

		dimension = Point<int16_t>(WIDTH, total_h);
		dragarea = Point<int16_t>(WIDTH, HEADER_HEIGHT);
	}

	void UIWhisper::update_title()
	{
		if (target_name.empty())
			title_text.change_text("To: ");
		else
			title_text.change_text("To: " + target_name);
	}

	void UIWhisper::draw(float inter) const
	{
		// Background
		background.draw(DrawArgument(position));
		header_bg.draw(DrawArgument(position));

		// Title
		title_text.draw(position + Point<int16_t>(5, 5));

		// Close button marker
		close_x.draw(position + Point<int16_t>(WIDTH - 10, 5));

		// Chat area
		int16_t chat_y = HEADER_HEIGHT + CHAT_HEIGHT - LINE_HEIGHT;
		int16_t lines_shown = CHAT_HEIGHT / LINE_HEIGHT;
		int16_t start = static_cast<int16_t>(chat_lines.size()) - lines_shown - scroll_offset;

		if (start < 0) start = 0;

		for (int16_t i = start; i < static_cast<int16_t>(chat_lines.size()) && (i - start) < lines_shown; i++)
		{
			int16_t y_pos = HEADER_HEIGHT + (i - start) * LINE_HEIGHT + 2;
			chat_lines[i].text.draw(position + Point<int16_t>(5, y_pos));
		}

		// Input area
		input_bg.draw(DrawArgument(position + Point<int16_t>(2, HEADER_HEIGHT + CHAT_HEIGHT + 2)));
		chatfield.draw(position);

		// Send button
		send_bg.draw(DrawArgument(position + Point<int16_t>(WIDTH - 55, HEADER_HEIGHT + CHAT_HEIGHT + 2)));
		send_text.draw(position + Point<int16_t>(WIDTH - 30, HEADER_HEIGHT + CHAT_HEIGHT + 6));

		// Name field (if no target set)
		if (target_name.empty())
			namefield.draw(position);

		UIElement::draw_buttons(inter);
	}

	void UIWhisper::update()
	{
		chatfield.update(position);

		if (target_name.empty())
			namefield.update(position);
	}

	Cursor::State UIWhisper::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		if (chatfield.get_state() == Textfield::State::FOCUSED)
		{
			if (clicked)
			{
				Cursor::State tstate = chatfield.send_cursor(cursoroffset, clicked);
				if (tstate != Cursor::State::IDLE)
					return tstate;
			}
		}

		if (target_name.empty() && namefield.get_state() == Textfield::State::FOCUSED)
		{
			if (clicked)
			{
				Cursor::State tstate = namefield.send_cursor(cursoroffset, clicked);
				if (tstate != Cursor::State::IDLE)
					return tstate;
			}
		}

		// Click on chat input area -> focus it
		Rectangle<int16_t> input_rect(
			Point<int16_t>(2, HEADER_HEIGHT + CHAT_HEIGHT + 2),
			Point<int16_t>(WIDTH - 60, HEADER_HEIGHT + CHAT_HEIGHT + INPUT_HEIGHT)
		);

		if (clicked && input_rect.contains(cursoroffset))
		{
			chatfield.set_state(Textfield::State::FOCUSED);
			UI::get().focus_textfield(&chatfield);
			return Cursor::State::CLICKING;
		}

		// Click on name area -> focus it
		if (target_name.empty() && clicked)
		{
			Rectangle<int16_t> name_rect(
				Point<int16_t>(40, 3),
				Point<int16_t>(WIDTH - 10, 20)
			);

			if (name_rect.contains(cursoroffset))
			{
				namefield.set_state(Textfield::State::FOCUSED);
				UI::get().focus_textfield(&namefield);
				return Cursor::State::CLICKING;
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIWhisper::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			deactivate();
			return;
		}
	}

	UIElement::Type UIWhisper::get_type() const
	{
		return TYPE;
	}

	Button::State UIWhisper::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			deactivate();
			break;
		case BT_SEND:
			send_whisper();
			break;
		case BT_FIND:
			if (!target_name.empty())
				FindPlayerPacket(target_name).dispatch();
			break;
		}

		return Button::State::NORMAL;
	}

	void UIWhisper::send_whisper()
	{
		// If no target, take from name field
		if (target_name.empty())
		{
			std::string name = namefield.get_text();

			if (name.empty())
				return;

			target_name = name;
			update_title();
		}

		std::string message = chatfield.get_text();

		if (message.empty())
			return;

		WhisperPacket(target_name, message).dispatch();

		// Add to local chat display
		ChatLine line;
		line.text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MALIBU, "To " + target_name + ": " + message);
		line.color = Color::Name::MALIBU;
		chat_lines.push_back(line);

		if (static_cast<int16_t>(chat_lines.size()) > MAX_LINES)
			chat_lines.erase(chat_lines.begin());

		chatfield.change_text("");
	}

	void UIWhisper::recv_whisper(const std::string& from, const std::string& message, bool from_admin)
	{
		ChatLine line;
		Color::Name color = from_admin ? Color::Name::YELLOW : Color::Name::WHITE;
		line.text = Text(Text::Font::A11M, Text::Alignment::LEFT, color, from + ": " + message);
		line.color = color;
		chat_lines.push_back(line);

		if (static_cast<int16_t>(chat_lines.size()) > MAX_LINES)
			chat_lines.erase(chat_lines.begin());

		// If this whisper is from someone new, update target
		if (target_name.empty())
		{
			target_name = from;
			update_title();
		}
	}

	void UIWhisper::set_target(const std::string& name)
	{
		target_name = name;
		namefield.change_text(name);
		update_title();
	}
}
