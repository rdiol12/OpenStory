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
#include "../Components/MapleButton.h"
#include "UINotice.h"

#include "../../Gameplay/Stage.h"
#include "../../Character/OtherChar.h"
#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIReport::UIReport(const std::string& target) :
		UIDragElement<PosREPORT>(Point<int16_t>(W, H)),
		target_name(target),
		selected_reason(-1),
		open_list(DropList::NONE),
		hovered_option(-1)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["Claim"]["report"];

		sprites.emplace_back(src["backgrnd"]);
		sprites.emplace_back(src["backgrnd2"]);
		sprites.emplace_back(src["backgrnd3"]);

		buttons[BT_SEND] = std::make_unique<MapleButton>(src["BtClaim"]);
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		namefield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(114, 145),
				Point<int16_t>(236, 159)
			),
			12
		);

		if (!target_name.empty())
			namefield.change_text(target_name);

		descfield = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(114, 184),
				Point<int16_t>(250, 236)
			),
			70
		);

		reason_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);

		dd_box = src["box2"];

		dimension = Point<int16_t>(W, H);
	}

	void UIReport::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		namefield.draw(position);
		descfield.draw(position);

		if (selected_reason >= 0)
			reason_text.draw(position + Point<int16_t>(DD_X + 6, DD_Y + 1));

		UIElement::draw_buttons(inter);

		if (open_list != DropList::NONE)
		{
			Point<int16_t> dd = position + list_pos;
			int16_t rows = static_cast<int16_t>(list_texts.size());

			dd_box.draw(DrawArgument(dd, Point<int16_t>(BOX_W, rows * ROW_H + 4)));

			for (int16_t i = 0; i < rows; i++)
				list_texts[i].draw(dd + Point<int16_t>(6, 2 + i * ROW_H));
		}
	}

	void UIReport::update()
	{
		namefield.update(position);
		descfield.update(position);
	}

	bool UIReport::indragrange(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> window(position, position + dimension);

		if (!window.contains(cursorpos))
			return false;

		Point<int16_t> off = cursorpos - position;

		Rectangle<int16_t> name_row(Point<int16_t>(NB_X, NB_Y), Point<int16_t>(NB_X + BOX_W, NB_Y + BOX_H));
		Rectangle<int16_t> reason_row(Point<int16_t>(DD_X, DD_Y), Point<int16_t>(DD_X + BOX_W, DD_Y + BOX_H));
		Rectangle<int16_t> details(Point<int16_t>(114, 184), Point<int16_t>(250, 236));

		if (name_row.contains(off) || reason_row.contains(off) || details.contains(off))
			return false;

		for (auto& btit : buttons)
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
				return false;

		return true;
	}

	void UIReport::open_name_list()
	{
		list_values.clear();
		list_texts.clear();

		for (auto& entry : *Stage::get().get_chars().get_chars())
		{
			if (list_values.size() >= 8)
				break;

			if (auto* other = static_cast<OtherChar*>(entry.second.get()))
			{
				std::string name = other->get_name();

				if (!name.empty())
				{
					list_values.push_back(name);
					list_texts.push_back(Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, name));
				}
			}
		}

		if (list_values.empty())
		{
			list_values.push_back("");
			list_texts.push_back(Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "(no players nearby)"));
		}

		list_pos = Point<int16_t>(NB_X, NB_Y + BOX_H);
		open_list = DropList::NAME;
		hovered_option = -1;
	}

	void UIReport::open_reason_list()
	{
		list_values.clear();
		list_texts.clear();

		for (int i = 0; i < NUM_REASONS; i++)
		{
			list_values.push_back(REASON_LABELS[i]);
			list_texts.push_back(Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, REASON_LABELS[i]));
		}

		list_pos = Point<int16_t>(DD_X, DD_Y + BOX_H);
		open_list = DropList::REASON;
		hovered_option = -1;
	}

	void UIReport::close_list()
	{
		open_list = DropList::NONE;
		hovered_option = -1;
	}

	Cursor::State UIReport::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroffset = cursorpos - position;

		DropList was_open = open_list;

		if (open_list != DropList::NONE)
		{
			int16_t rows = static_cast<int16_t>(list_texts.size());
			Rectangle<int16_t> list_rect(
				list_pos,
				list_pos + Point<int16_t>(BOX_W, rows * ROW_H + 4)
			);

			if (list_rect.contains(cursoroffset))
			{
				int16_t row = (cursoroffset.y() - (list_pos.y() + 2)) / ROW_H;

				if (row < 0)
					row = 0;

				if (row >= rows)
					row = rows - 1;

				if (hovered_option != row)
				{
					hovered_option = row;

					for (int16_t i = 0; i < rows; i++)
						if (!list_values[i].empty())
							list_texts[i].change_color(i == row ? Color::Name::MEDIUMBLUE : Color::Name::BLACK);
				}

				if (clicked)
				{
					if (!list_values[row].empty())
					{
						if (open_list == DropList::NAME)
						{
							namefield.change_text(list_values[row]);
						}
						else
						{
							selected_reason = row;
							reason_text.change_text(REASON_LABELS[row]);
						}
					}

					close_list();
				}

				return clicked ? Cursor::State::CLICKING : Cursor::State::CANCLICK;
			}

			if (clicked)
				close_list();
		}

		Rectangle<int16_t> name_row(Point<int16_t>(NB_X, NB_Y), Point<int16_t>(NB_X + BOX_W, NB_Y + BOX_H));
		Rectangle<int16_t> reason_row(Point<int16_t>(DD_X, DD_Y), Point<int16_t>(DD_X + BOX_W, DD_Y + BOX_H));

		// Arrow region of the name box opens the nearby-player list;
		// the text region focuses the typing field
		Rectangle<int16_t> name_arrow(Point<int16_t>(NB_X + BOX_W - 18, NB_Y), Point<int16_t>(NB_X + BOX_W, NB_Y + BOX_H));

		if (name_arrow.contains(cursoroffset))
		{
			if (clicked)
			{
				if (namefield.get_state() == Textfield::State::FOCUSED)
					namefield.set_state(Textfield::State::NORMAL);

				if (descfield.get_state() == Textfield::State::FOCUSED)
					descfield.set_state(Textfield::State::NORMAL);

				if (was_open == DropList::NAME)
					close_list();
				else
					open_name_list();

				return Cursor::State::CLICKING;
			}

			return Cursor::State::CANCLICK;
		}

		if (reason_row.contains(cursoroffset))
		{
			if (clicked)
			{
				if (namefield.get_state() == Textfield::State::FOCUSED)
					namefield.set_state(Textfield::State::NORMAL);

				if (descfield.get_state() == Textfield::State::FOCUSED)
					descfield.set_state(Textfield::State::NORMAL);

				if (was_open == DropList::REASON)
					close_list();
				else
					open_reason_list();

				return Cursor::State::CLICKING;
			}

			return Cursor::State::CANCLICK;
		}

		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (clicked)
		{
			Rectangle<int16_t> name_text_rect(
				Point<int16_t>(NB_X, NB_Y),
				Point<int16_t>(NB_X + BOX_W - 18, NB_Y + BOX_H)
			);
			Rectangle<int16_t> details_rect(
				Point<int16_t>(114, 184),
				Point<int16_t>(250, 236)
			);

			if (name_text_rect.contains(cursoroffset))
			{
				if (descfield.get_state() == Textfield::State::FOCUSED)
					descfield.set_state(Textfield::State::NORMAL);

				namefield.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			if (details_rect.contains(cursoroffset))
			{
				if (namefield.get_state() == Textfield::State::FOCUSED)
					namefield.set_state(Textfield::State::NORMAL);

				descfield.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			if (namefield.get_state() == Textfield::State::FOCUSED)
				namefield.set_state(Textfield::State::NORMAL);

			if (descfield.get_state() == Textfield::State::FOCUSED)
				descfield.set_state(Textfield::State::NORMAL);
		}

		if (Cursor::State new_state = namefield.send_cursor(cursorpos, clicked))
			return new_state;

		if (Cursor::State new_state = descfield.send_cursor(cursorpos, clicked))
			return new_state;

		return dstate;
	}

	void UIReport::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (open_list != DropList::NONE)
				close_list();
			else
				deactivate();
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
		case BT_SEND:
			send_report();
			break;
		case BT_CANCEL:
			deactivate();
			break;
		}

		return Button::State::NORMAL;
	}

	void UIReport::send_report()
	{
		std::string name = namefield.get_text();

		if (name.empty())
		{
			UI::get().emplace<UIOk>("Please enter a character name.", [](bool) {});
			return;
		}

		if (selected_reason < 0)
		{
			UI::get().emplace<UIOk>("Please select a reason.", [](bool) {});
			return;
		}

		target_name = name;

		std::string desc = descfield.get_text();
		ReportPacket(0, target_name, static_cast<int8_t>(selected_reason), desc).dispatch();

		deactivate();

		UI::get().emplace<UIOk>("Report for '" + target_name + "' sent to GMs.", [](bool) {});
	}

	void UIReport::set_target(const std::string& name)
	{
		target_name = name;
		namefield.change_text(name);
	}
}
