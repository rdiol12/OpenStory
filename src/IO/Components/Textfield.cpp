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
#include "Textfield.h"

#include "../UI.h"

#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"

#include <sstream>

namespace ms
{
	Textfield::Textfield()
	{
		text = "";
	}

	Textfield::Textfield(Text::Font font, Text::Alignment alignment, Color::Name text_color, Rectangle<int16_t> bounds, size_t limit) : Textfield(font, alignment, text_color, text_color, 1.0f, bounds, limit) {}

	Textfield::Textfield(Text::Font font, Text::Alignment alignment, Color::Name text_color, Color::Name marker_color, float marker_opacity, Rectangle<int16_t> bounds, size_t limit) : font(font), alignment(alignment), text_color(text_color), bounds(bounds), limit(limit)
	{
		textlabel = Text(font, alignment, text_color, "", 0, false);
		marker = ColorLine(12, marker_color, marker_opacity, true);

		text = "";
		markerpos = 0;
		crypt = 0;
		state = State::NORMAL;
	}

	void Textfield::draw(Point<int16_t> position) const
	{
		draw(position, Point<int16_t>(0, 0));
	}

	void Textfield::draw(Point<int16_t> position, Point<int16_t> marker_adjust) const
	{
		if (state == State::DISABLED)
			return;

		Point<int16_t> absp = bounds.get_left_top() + position;

		if (text.size() > 0)
			textlabel.draw(absp);

		if (state == State::FOCUSED && has_selection() && selectable())
		{
			size_t lo = std::min(sel_anchor, sel_end);
			size_t hi = std::max(sel_anchor, sel_end);
			int16_t x0 = textlabel.advance(lo);
			int16_t x1 = textlabel.advance(hi);
			ColorBox cover(x1 - x0, 14, Color::Name::WHITE, 0.55f);
			cover.draw(absp + Point<int16_t>(x0, 2));
		}

		if (state == State::FOCUSED && showmarker)
		{
			Point<int16_t> mpos;

			if (wrap_width > 0)
			{
				mpos = absp + textlabel.endoffset() + Point<int16_t>(-1, 8) + marker_adjust;
			}
			else
			{
				// markerpos is a byte offset into the LOGICAL string, but the
				// layout was built from the visual one. In Hebrew those differ,
				// so the caret has to be mapped across or it lands on the wrong
				// side of the text entirely.
				size_t vis = Text::logical_to_visual(text, markerpos);
				mpos = absp + Point<int16_t>(textlabel.advance(vis) - 1, 8) + marker_adjust;
			}

			if (crypt > 0)
				mpos.shift(1, -3);

			marker.draw(mpos);
		}
	}

	void Textfield::set_wrap(uint16_t width)
	{
		wrap_width = width;
		textlabel = Text(font, alignment, text_color, text, width, false);
	}

	int16_t Textfield::text_height() const
	{
		return textlabel.height();
	}

	void Textfield::update(Point<int16_t> parent)
	{
		if (state == State::DISABLED)
			return;

		parentpos = parent;
		elapsed += Constants::TIMESTEP;

		if (elapsed > 256)
		{
			showmarker = !showmarker;
			elapsed = 0;
		}
	}

	void Textfield::set_state(State st)
	{
		if (state != st)
		{
			state = st;

			if (state != State::DISABLED)
			{
				elapsed = 0;
				showmarker = true;
			}
			else
			{
				UI::get().remove_textfield();
			}

			if (state == State::FOCUSED)
				UI::get().focus_textfield(this);
		}
	}

	void Textfield::set_enter_callback(std::function<void(std::string)> onr)
	{
		onreturn = onr;
	}

	void Textfield::set_key_callback(KeyAction::Id key, std::function<void(void)> action)
	{
		callbacks[key] = action;
	}

	void Textfield::set_text_callback(std::function<void(void)> action)
	{
		ontext = action;
	}

	void Textfield::send_key(KeyType::Id type, int32_t key, bool pressed)
	{
		if (pressed)
		{
			if (type == KeyType::Id::ACTION)
			{
				switch (key)
				{
					case KeyAction::Id::LEFT:
					{
						clear_selection();

						markerpos = prev_boundary(markerpos);

						break;
					}
					case KeyAction::Id::RIGHT:
					{
						clear_selection();

						markerpos = next_boundary(markerpos);

						break;
					}
					case KeyAction::Id::BACK:
					{
						if (has_selection())
						{
							erase_selection();
						}
						else if (text.size() > 0 && markerpos > 0)
						{
							size_t start = prev_boundary(markerpos);

							text.erase(start, markerpos - start);

							markerpos = start;

							modifytext(text);
						}


						break;
					}
					case KeyAction::Id::RETURN:
					{
						if (onreturn)
							onreturn(text);

						break;
					}
					case KeyAction::Id::SPACE:
					{
						add_string(" ");
						break;
					}
					case KeyAction::Id::HOME:
					{
						markerpos = 0;
						break;
					}
					case KeyAction::Id::END:
					{
						markerpos = text.size();
						break;
					}
					case KeyAction::Id::DELETE:
					{
						if (has_selection())
						{
							erase_selection();
						}
						else if (text.size() > 0 && markerpos < text.size())
						{
							text.erase(markerpos, next_boundary(markerpos) - markerpos);

							modifytext(text);
						}

						break;
					}
					default:
					{
						if (callbacks.count(key))
							callbacks.at(key)();

						break;
					}
				}
			}
			else if (type == KeyType::Id::TEXT)
			{
				if (ontext)
				{
					if (isdigit(key) || isalpha(key))
					{
						ontext();
						return;
					}
				}

				std::stringstream ss;
				char a = static_cast<int8_t>(key);

				ss << a;

				add_string(ss.str());
			}
		}
	}

	size_t Textfield::prev_boundary(size_t pos) const
	{
		if (pos == 0)
			return 0;

		size_t i = pos - 1;

		// Continuation bytes are 10xxxxxx; step back over them to the lead byte.
		while (i > 0 && (static_cast<uint8_t>(text[i]) & 0xC0) == 0x80)
			--i;

		return i;
	}

	size_t Textfield::next_boundary(size_t pos) const
	{
		if (pos >= text.size())
			return text.size();

		size_t i = pos + 1;

		while (i < text.size() && (static_cast<uint8_t>(text[i]) & 0xC0) == 0x80)
			++i;

		return i;
	}

	void Textfield::add_codepoint(uint32_t codepoint)
	{
		// Fields with a text callback (search boxes and the like) consume
		// alphanumerics as a signal rather than inserting them.
		if (ontext && codepoint < 0x80 && (isdigit(static_cast<int>(codepoint)) || isalpha(static_cast<int>(codepoint))))
		{
			ontext();
			return;
		}

		if (has_selection())
			erase_selection();

		if (!belowlimit())
			return;

		std::string encoded;
		Text::utf8_encode(codepoint, encoded);

		text.insert(markerpos, encoded);
		markerpos += encoded.size();

		modifytext(text);
	}

	void Textfield::add_string(const std::string& str)
	{
		if (has_selection())
			erase_selection();

		for (char c : str)
		{
			if (belowlimit())
			{
				text.insert(markerpos, 1, c);

				markerpos++;

				modifytext(text);
			}
		}
	}

	void Textfield::modifytext(const std::string& t)
	{
		if (crypt > 0)
		{
			std::string crypted;
			crypted.insert(0, t.size(), crypt);

			textlabel.change_text(crypted);
		}
		else
		{
			textlabel.change_text(t);
		}

		text = t;
	}

	Cursor::State Textfield::send_cursor(Point<int16_t> cursorpos, bool clicked)
	{
		if (state == State::DISABLED)
			return Cursor::State::IDLE;

		if (get_bounds().contains(cursorpos))
		{
			if (clicked)
			{
				if (state == State::NORMAL)
					set_state(State::FOCUSED);

				if (selectable())
				{
					size_t idx = index_at(cursorpos.x());

					if (!selecting)
					{
						selecting = true;
						sel_anchor = idx;
					}

					sel_end = idx;
					markerpos = idx;
				}

				return Cursor::State::CLICKING;
			}
			else
			{
				selecting = false;

				return Cursor::State::CANCLICK;
			}
		}
		else
		{
			if (clicked)
			{
				if (selecting)
				{
					sel_end = index_at(cursorpos.x());
					markerpos = sel_end;

					return Cursor::State::CLICKING;
				}

				if (state == State::FOCUSED)
					set_state(State::NORMAL);
			}
			else
			{
				selecting = false;
			}

			return Cursor::State::IDLE;
		}
	}

	void Textfield::change_text(const std::string& t)
	{
		clear_selection();
		modifytext(t);

		markerpos = text.size();
	}

	size_t Textfield::index_at(int16_t cursor_x) const
	{
		int16_t relx = cursor_x - get_bounds().get_left_top().x();
		size_t best = 0;
		int16_t bestdist = INT16_MAX;

		for (size_t i = 0; i <= text.size(); i++)
		{
			int16_t dist = std::abs(textlabel.advance(i) - relx);

			if (dist < bestdist)
			{
				bestdist = dist;
				best = i;
			}
		}

		return best;
	}

	bool Textfield::has_selection() const
	{
		return sel_anchor != sel_end && sel_anchor <= text.size() && sel_end <= text.size();
	}

	bool Textfield::selectable() const
	{
		return alignment == Text::Alignment::LEFT && wrap_width == 0 && crypt == 0;
	}

	void Textfield::erase_selection()
	{
		size_t lo = std::min(sel_anchor, sel_end);
		size_t hi = std::max(sel_anchor, sel_end);
		text.erase(lo, hi - lo);
		markerpos = lo;
		clear_selection();
		modifytext(text);
	}

	void Textfield::clear_selection()
	{
		sel_anchor = 0;
		sel_end = 0;
		selecting = false;
	}

	std::string Textfield::get_selected_text() const
	{
		if (!has_selection())
			return text;

		size_t lo = std::min(sel_anchor, sel_end);
		size_t hi = std::max(sel_anchor, sel_end);

		return text.substr(lo, hi - lo);
	}

	void Textfield::set_cryptchar(int8_t character)
	{
		crypt = character;
	}

	bool Textfield::belowlimit() const
	{
		if (limit > 0)
		{
			return text.size() < limit;
		}
		else
		{
			uint16_t advance = textlabel.advance(text.size());

			return (advance + 50) < bounds.get_horizontal().length();
		}
	}

	const std::string& Textfield::get_text() const
	{
		return text;
	}

	bool Textfield::can_copy_paste() const
	{
		if (ontext)
		{
			ontext();

			return false;
		}
		else
		{
			return true;
		}
	}

	bool Textfield::empty() const
	{
		return text.empty();
	}

	Textfield::State Textfield::get_state() const
	{
		return state;
	}

	Rectangle<int16_t> Textfield::get_bounds() const
	{
		return Rectangle<int16_t>(
			bounds.get_left_top() + parentpos,
			bounds.get_right_bottom() + parentpos
			);
	}
}