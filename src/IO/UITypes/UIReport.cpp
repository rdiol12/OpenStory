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

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Layout constants for 280x300 panel
	static constexpr int16_t PAD = 8;
	static constexpr int16_t TITLE_H = 24;
	static constexpr int16_t NAME_ROW_Y = TITLE_H + 4;
	static constexpr int16_t NAME_ROW_H = 20;
	static constexpr int16_t REASON_LABEL_Y = NAME_ROW_Y + NAME_ROW_H + 6;
	static constexpr int16_t REASON_TOP = REASON_LABEL_Y + 14;
	static constexpr int16_t REASON_H = 20;
	static constexpr int16_t REASON_STEP = 22;
	static constexpr int16_t DESC_LABEL_Y = REASON_TOP + 5 * REASON_STEP + 4;
	static constexpr int16_t DESC_TOP = DESC_LABEL_Y + 14;
	static constexpr int16_t BTN_H = 20;

	UIReport::UIReport(const std::string& target) :
		UIDragElement<PosREPORT>(Point<int16_t>(280, TITLE_H)),
		target_name(target),
		selected_reason(-1)
	{
		background = ColorBox(W, H, Color::Name::BLACK, 0.85f);

		int16_t desc_h = H - DESC_TOP - BTN_H - 12;
		int16_t btn_y = H - BTN_H - 6;
		int16_t btn_w = 70;

		title_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);

		if (target_name.empty())
			title_text.change_text("Report Player");
		else
			title_text.change_text("Report: " + target_name);

		// Name input field
		namefield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::WHITE, Color::Name::SMALT, 0.75f,
			Rectangle<int16_t>(
				Point<int16_t>(45, NAME_ROW_Y + 2),
				Point<int16_t>(W - PAD, NAME_ROW_Y + NAME_ROW_H)
			),
			12
		);

		if (!target_name.empty())
			namefield.change_text(target_name);

		// Description text field
		input_bg = ColorBox(W - PAD * 2, desc_h, Color::Name::GALLERY, 1.0f);

		descfield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(PAD + 2, DESC_TOP + 2),
				Point<int16_t>(W - PAD - 2, DESC_TOP + desc_h)
			),
			200
		);

		// Close button (top-right)
		buttons[BT_CLOSE] = std::make_unique<AreaButton>(
			Point<int16_t>(W - 20, 4), Point<int16_t>(16, 16)
		);

		// Send button (bottom center)
		buttons[BT_SEND] = std::make_unique<AreaButton>(
			Point<int16_t>(W / 2 - btn_w / 2, btn_y),
			Point<int16_t>(btn_w, BTN_H)
		);

		// Reason buttons
		for (int i = 0; i < NUM_REASONS; i++)
		{
			int16_t y = REASON_TOP + i * REASON_STEP;
			buttons[BT_REASON0 + i] = std::make_unique<AreaButton>(
				Point<int16_t>(PAD, y), Point<int16_t>(W - PAD * 2, REASON_H)
			);
		}

		// Draw objects
		close_x = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "X");
		name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Name:");
		reason_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Select reason:");
		desc_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Details (optional):");
		selected_reason_bg = ColorBox(W - PAD * 2, REASON_H, Color::Name::MALIBU, 0.8f);
		btn_bg = ColorBox(btn_w, BTN_H, Color::Name::MALIBU, 0.8f);
		btn_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Send");

		for (int i = 0; i < NUM_REASONS; i++)
		{
			reason_bgs[i] = ColorBox(W - PAD * 2, REASON_H, Color::Name::MINESHAFT, 0.5f);
			reason_texts[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, REASON_LABELS[i]);
		}

		dimension = Point<int16_t>(W, H);
		dragarea = Point<int16_t>(W, TITLE_H);
	}

	void UIReport::draw(float inter) const
	{
		background.draw(DrawArgument(position));

		int16_t desc_h = H - DESC_TOP - BTN_H - 12;
		int16_t btn_y = H - BTN_H - 6;
		int16_t btn_w = 70;

		title_text.draw(position + Point<int16_t>(PAD, 5));
		close_x.draw(position + Point<int16_t>(W - 12, 5));

		if (target_name.empty())
		{
			name_label.draw(position + Point<int16_t>(PAD, NAME_ROW_Y + 4));
			namefield.draw(position);
		}

		reason_label.draw(position + Point<int16_t>(PAD, REASON_LABEL_Y));

		for (int i = 0; i < NUM_REASONS; i++)
		{
			int16_t y = REASON_TOP + i * REASON_STEP;
			bool selected = (selected_reason == i);

			if (selected)
				selected_reason_bg.draw(DrawArgument(position + Point<int16_t>(PAD, y)));
			else
				reason_bgs[i].draw(DrawArgument(position + Point<int16_t>(PAD, y)));

			reason_texts[i].draw(position + Point<int16_t>(PAD + 5, y + 3));
		}

		desc_label.draw(position + Point<int16_t>(PAD, DESC_LABEL_Y));

		input_bg.draw(DrawArgument(position + Point<int16_t>(PAD, DESC_TOP)));
		descfield.draw(position);

		btn_bg.draw(DrawArgument(position + Point<int16_t>(W / 2 - btn_w / 2, btn_y)));
		btn_text.draw(position + Point<int16_t>(W / 2, btn_y + 3));

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
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> cursoroffset = cursorpos - position;

		int16_t desc_h = H - DESC_TOP - BTN_H - 12;

		if (clicked)
		{
			// Click on description area -> focus it
			Rectangle<int16_t> desc_rect(
				Point<int16_t>(PAD, DESC_TOP),
				Point<int16_t>(W - PAD, DESC_TOP + desc_h)
			);

			if (desc_rect.contains(cursoroffset))
			{
				if (target_name.empty() && namefield.get_state() == Textfield::State::FOCUSED)
					namefield.set_state(Textfield::State::NORMAL);

				descfield.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			// Click on name area -> focus it
			if (target_name.empty())
			{
				Rectangle<int16_t> name_rect(
					Point<int16_t>(45, NAME_ROW_Y + 2),
					Point<int16_t>(W - PAD, NAME_ROW_Y + NAME_ROW_H)
				);

				if (name_rect.contains(cursoroffset))
				{
					if (descfield.get_state() == Textfield::State::FOCUSED)
						descfield.set_state(Textfield::State::NORMAL);

					namefield.set_state(Textfield::State::FOCUSED);
					return Cursor::State::CLICKING;
				}
			}

			// Click outside both textfields -> unfocus
			if (descfield.get_state() == Textfield::State::FOCUSED)
				descfield.set_state(Textfield::State::NORMAL);

			if (namefield.get_state() == Textfield::State::FOCUSED)
				namefield.set_state(Textfield::State::NORMAL);
		}

		// Textfield hover cursors
		if (Cursor::State new_state = descfield.send_cursor(cursorpos, clicked))
			return new_state;

		if (target_name.empty())
		{
			if (Cursor::State new_state = namefield.send_cursor(cursorpos, clicked))
				return new_state;
		}

		return dstate;
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
			{
				title_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::RED, "Enter a name!");
				return;
			}

			target_name = name;
			title_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE, "Report: " + target_name);
		}

		if (selected_reason < 0)
		{
			reason_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::RED, "Select a reason!");
			return;
		}

		std::string desc = descfield.get_text();
		ReportPacket(0, target_name, static_cast<int8_t>(selected_reason), desc).dispatch();

		deactivate();

		UI::get().emplace<UIOk>("Report for '" + target_name + "' sent to GMs.", [](bool) {});
	}

	void UIReport::set_target(const std::string& name)
	{
		target_name = name;
		namefield.change_text(name);
		title_text.change_text("Report: " + target_name);
	}
}
