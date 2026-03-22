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
#include "UIReport.h"

#include "../UI.h"
#include "../Components/AreaButton.h"
#include "UINotice.h"

#include "../../Net/Packets/SocialPackets.h"

namespace ms
{
	UIReport::UIReport(const std::string& target) :
		UIDragElement<PosREPORT>(Point<int16_t>(WIDTH, HEADER_HEIGHT)),
		target_name(target),
		selected_reason(-1)
	{
		int16_t total_h = HEADER_HEIGHT + BODY_HEIGHT + INPUT_HEIGHT + 30;

		background = ColorBox(WIDTH, total_h, Color::Name::BLACK, 0.85f);
		header_bg = ColorBox(WIDTH, HEADER_HEIGHT, Color::Name::EMPEROR, 0.9f);
		input_bg = ColorBox(WIDTH - 10, INPUT_HEIGHT, Color::Name::GALLERY, 1.0f);

		title_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);

		if (target_name.empty())
			title_text.change_text("Report Player");
		else
			title_text.change_text("Report: " + target_name);

		// Name input field
		namefield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::WHITE,
			Rectangle<int16_t>(Point<int16_t>(50, 5), Point<int16_t>(WIDTH - 10, 20)),
			15
		);

		if (!target_name.empty())
			namefield.change_text(target_name);

		// Description text field
		descfield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(5, HEADER_HEIGHT + BODY_HEIGHT + 5),
				Point<int16_t>(WIDTH - 10, HEADER_HEIGHT + BODY_HEIGHT + INPUT_HEIGHT)
			),
			200
		);

		// Close button
		buttons[BT_CLOSE] = std::make_unique<AreaButton>(
			Point<int16_t>(WIDTH - 18, 3), Point<int16_t>(15, 15)
		);

		// Send button
		buttons[BT_SEND] = std::make_unique<AreaButton>(
			Point<int16_t>(WIDTH / 2 - 40, HEADER_HEIGHT + BODY_HEIGHT + INPUT_HEIGHT + 8),
			Point<int16_t>(80, 20)
		);

		// Reason buttons
		for (int i = 0; i < NUM_REASONS; i++)
		{
			int16_t y = HEADER_HEIGHT + 30 + i * 28;
			buttons[BT_REASON0 + i] = std::make_unique<AreaButton>(
				Point<int16_t>(10, y), Point<int16_t>(WIDTH - 20, 24)
			);
		}

		// Pre-allocate draw objects
		close_x = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "X");
		name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Name:");
		reason_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Select reason:");
		desc_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Details (optional):");
		selected_reason_bg = ColorBox(WIDTH - 20, 24, Color::Name::MALIBU, 0.8f);
		btn_bg = ColorBox(80, 20, Color::Name::MALIBU, 0.8f);
		btn_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Submit");

		for (int i = 0; i < NUM_REASONS; i++)
		{
			reason_bgs[i] = ColorBox(WIDTH - 20, 24, Color::Name::MINESHAFT, 0.5f);
			reason_texts[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, REASON_LABELS[i]);
		}

		dimension = Point<int16_t>(WIDTH, total_h);
		dragarea = Point<int16_t>(WIDTH, HEADER_HEIGHT);
	}

	void UIReport::draw(float inter) const
	{
		background.draw(DrawArgument(position));
		header_bg.draw(DrawArgument(position));

		title_text.draw(position + Point<int16_t>(5, 5));
		close_x.draw(position + Point<int16_t>(WIDTH - 10, 5));

		if (target_name.empty())
		{
			name_label.draw(position + Point<int16_t>(5, 5));
			namefield.draw(position);
		}

		reason_label.draw(position + Point<int16_t>(10, HEADER_HEIGHT + 10));

		for (int i = 0; i < NUM_REASONS; i++)
		{
			int16_t y = HEADER_HEIGHT + 30 + i * 28;
			bool selected = (selected_reason == i);

			if (selected)
			{
				selected_reason_bg.draw(DrawArgument(position + Point<int16_t>(10, y)));
			}
			else
			{
				reason_bgs[i].draw(DrawArgument(position + Point<int16_t>(10, y)));
			}

			reason_texts[i].draw(position + Point<int16_t>(15, y + 5));
		}

		desc_label.draw(position + Point<int16_t>(5, HEADER_HEIGHT + BODY_HEIGHT - 10));

		input_bg.draw(DrawArgument(position + Point<int16_t>(5, HEADER_HEIGHT + BODY_HEIGHT + 5)));
		descfield.draw(position);

		int16_t btn_y = HEADER_HEIGHT + BODY_HEIGHT + INPUT_HEIGHT + 8;
		btn_bg.draw(DrawArgument(position + Point<int16_t>(WIDTH / 2 - 40, btn_y)));
		btn_text.draw(position + Point<int16_t>(WIDTH / 2, btn_y + 3));

		UIElement::draw_buttons(inter);
	}

	void UIReport::update()
	{
		descfield.update(position);

		if (target_name.empty())
			namefield.update(position);
	}

	Cursor::State UIReport::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		if (descfield.get_state() == Textfield::State::FOCUSED)
		{
			if (clicked)
			{
				Cursor::State tstate = descfield.send_cursor(cursoroffset, clicked);
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

		// Click on description area -> focus it
		Rectangle<int16_t> desc_rect(
			Point<int16_t>(5, HEADER_HEIGHT + BODY_HEIGHT + 5),
			Point<int16_t>(WIDTH - 10, HEADER_HEIGHT + BODY_HEIGHT + INPUT_HEIGHT)
		);

		if (clicked && desc_rect.contains(cursoroffset))
		{
			descfield.set_state(Textfield::State::FOCUSED);
			UI::get().focus_textfield(&descfield);
			return Cursor::State::CLICKING;
		}

		// Click on name area -> focus it
		if (target_name.empty() && clicked)
		{
			Rectangle<int16_t> name_rect(
				Point<int16_t>(50, 3),
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

	void UIReport::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			deactivate();
			return;
		}
	}

	UIElement::Type UIReport::get_type() const
	{
		return TYPE;
	}

	Button::State UIReport::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			deactivate();
			break;
		case BT_SEND:
			send_report();
			break;
		default:
			if (buttonid >= BT_REASON0 && buttonid <= BT_REASON4)
				selected_reason = buttonid - BT_REASON0;
			break;
		}

		return Button::State::NORMAL;
	}

	void UIReport::send_report()
	{
		if (target_name.empty())
		{
			std::string name = namefield.get_text();

			if (name.empty())
				return;

			target_name = name;
			title_text.change_text("Report: " + target_name);
		}

		if (selected_reason < 0)
			return;

		std::string desc = descfield.get_text();
		ReportPacket(0, target_name, static_cast<int8_t>(selected_reason), desc).dispatch();

		deactivate();

		UI::get().emplace<UIOk>("Your report for '" + target_name + "' has been submitted.", [](bool) {});
	}

	void UIReport::set_target(const std::string& name)
	{
		target_name = name;
		namefield.change_text(name);
		title_text.change_text("Report: " + target_name);
	}
}
