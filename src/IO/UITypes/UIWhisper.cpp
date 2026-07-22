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
#include "UIChatBar.h"
#include "../Components/MapleButton.h"

#include "../../Net/Packets/BuddyPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIWhisper::UIWhisper(const std::string& target) :
		UIDragElement<PosWHISPER>(Point<int16_t>(W, H)),
		target_name(target)
	{
		nl::node memo = nl::nx::ui["UIWindow2.img"]["MemoInGame"];
		nl::node send = memo["Send"];

		sprites.emplace_back(memo["backgrnd"]);
		sprites.emplace_back(send["backgrnd"]);
		sprites.emplace_back(send["backgrnd2"]);

		buttons[BT_SEND] = std::make_unique<MapleButton>(send["BtOK"]);
		buttons[BT_CLOSE] = std::make_unique<MapleButton>(send["BtCancle"]);

		divider = memo["Get"]["line"];

		namefield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(NAME_L, NAME_T),
				Point<int16_t>(NAME_R, NAME_B)
			),
			15
		);

		if (!target_name.empty())
			namefield.change_text(target_name);

		// Typing line at the top of the white area, history below it;
		// text wraps at the box width so long messages keep going down
		chatfield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(AREA_L + 4, AREA_T),
				Point<int16_t>(AREA_R - 4, AREA_T + 14)
			),
			120
		);
		chatfield.set_wrap(AREA_R - AREA_L - 10);

		chatfield.set_enter_callback(
			[this](std::string)
			{
				send_whisper();
			}
		);

		dimension = Point<int16_t>(W, H);
	}

	void UIWhisper::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		namefield.draw(position, Point<int16_t>(0, -5));

		chatfield.draw(position, Point<int16_t>(0, -5));

		// Divider tracks the wrapped typing block; history flows below it
		int16_t typing_h = chatfield.text_height();

		if (typing_h < 14)
			typing_h = 14;

		int16_t div_y = AREA_T + typing_h + 8;

		if (div_y > AREA_B - 6)
			div_y = AREA_B - 6;

		divider.draw(DrawArgument(
			position + Point<int16_t>(AREA_L + 2, div_y) + divider.get_origin()));

		int16_t count = static_cast<int16_t>(chat_lines.size());
		int16_t start = count > SHOWN_LINES ? count - SHOWN_LINES : 0;
		int16_t line_y = div_y + 3;

		for (int16_t i = start; i < count; i++)
		{
			if (line_y + LINE_HEIGHT > AREA_B)
				break;

			chat_lines[i].text.draw(
				position + Point<int16_t>(AREA_L + 4, line_y));
			line_y += LINE_HEIGHT;
		}

		UIElement::draw_buttons(inter);
	}

	void UIWhisper::update()
	{
		namefield.update(position);
		chatfield.update(position);
	}

	bool UIWhisper::indragrange(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> window(position, position + dimension);

		if (!window.contains(cursorpos))
			return false;

		Point<int16_t> off = cursorpos - position;

		Rectangle<int16_t> name_box(Point<int16_t>(NAME_L - 3, NAME_T - 2), Point<int16_t>(NAME_R + 3, NAME_B + 2));
		Rectangle<int16_t> area(Point<int16_t>(AREA_L, AREA_T), Point<int16_t>(AREA_R, AREA_B));

		if (name_box.contains(off) || area.contains(off))
			return false;

		for (auto& btit : buttons)
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
				return false;

		return true;
	}

	Cursor::State UIWhisper::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> cursoroffset = cursorpos - position;

		if (clicked)
		{
			Rectangle<int16_t> name_rect(
				Point<int16_t>(NAME_L - 3, NAME_T - 2),
				Point<int16_t>(NAME_R + 3, NAME_B + 2)
			);

			if (name_rect.contains(cursoroffset))
			{
				if (chatfield.get_state() == Textfield::State::FOCUSED)
					chatfield.set_state(Textfield::State::NORMAL);

				namefield.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			// Anywhere in the white message area focuses the input line
			Rectangle<int16_t> input_rect(
				Point<int16_t>(AREA_L, AREA_T),
				Point<int16_t>(AREA_R, AREA_B)
			);

			if (input_rect.contains(cursoroffset))
			{
				if (namefield.get_state() == Textfield::State::FOCUSED)
					namefield.set_state(Textfield::State::NORMAL);

				chatfield.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			if (namefield.get_state() == Textfield::State::FOCUSED)
				namefield.set_state(Textfield::State::NORMAL);

			if (chatfield.get_state() == Textfield::State::FOCUSED)
				chatfield.set_state(Textfield::State::NORMAL);
		}

		if (Cursor::State new_state = namefield.send_cursor(cursorpos, clicked))
			return new_state;

		if (Cursor::State new_state = chatfield.send_cursor(cursorpos, clicked))
			return new_state;

		return dstate;
	}

	void UIWhisper::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIWhisper::get_type() const
	{
		return TYPE;
	}

	Button::State UIWhisper::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_SEND:
			send_whisper();
			break;
		case BT_CLOSE:
			deactivate();
			break;
		}

		return Button::State::NORMAL;
	}

	void UIWhisper::send_whisper()
	{
		std::string name = namefield.get_text();

		if (!name.empty())
			target_name = name;

		if (target_name.empty())
			return;

		std::string message = chatfield.get_text();

		if (message.empty())
			return;

		WhisperPacket(target_name, message).dispatch();

		// Echo into the global chat bar in PINK so the player sees
		// their own outgoing whisper alongside replies.
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(
				"To " + target_name + " <Whisper>: " + message,
				UIChatBar::LineType::PINK);

		// Also keep a copy in the whisper window's own chat history.
		ChatLine line;
		line.text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::MEDIUMBLUE, "To " + target_name + ": " + message);
		line.color = Color::Name::MEDIUMBLUE;
		chat_lines.push_back(line);

		if (static_cast<int16_t>(chat_lines.size()) > MAX_LINES)
			chat_lines.erase(chat_lines.begin());

		chatfield.change_text("");
	}

	void UIWhisper::recv_whisper(const std::string& from, const std::string& message, bool from_admin)
	{
		ChatLine line;
		Color::Name color = from_admin ? Color::Name::ORANGE : Color::Name::BLACK;
		line.text = Text(Text::Font::A11M, Text::Alignment::LEFT, color, from + ": " + message);
		line.color = color;
		chat_lines.push_back(line);

		if (static_cast<int16_t>(chat_lines.size()) > MAX_LINES)
			chat_lines.erase(chat_lines.begin());

		// If this whisper is from someone new, update target
		if (target_name.empty())
		{
			target_name = from;
			namefield.change_text(from);
		}
	}

	void UIWhisper::set_target(const std::string& name)
	{
		target_name = name;
		namefield.change_text(name);
	}
}
