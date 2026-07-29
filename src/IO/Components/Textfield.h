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

#include "../Cursor.h"
#include "../Keyboard.h"

#include "../../Graphics/Geometry.h"

#include <functional>

namespace ms
{
	class Textfield
	{
	public:
		enum State
		{
			NORMAL,
			DISABLED,
			FOCUSED
		};

		Textfield();
		Textfield(Text::Font font, Text::Alignment alignment, Color::Name text_color, Rectangle<int16_t> bounds, size_t limit);
		Textfield(Text::Font font, Text::Alignment alignment, Color::Name text_color, Color::Name marker_color, float marker_opacity, Rectangle<int16_t> bounds, size_t limit);

		void draw(Point<int16_t> position) const;
		void draw(Point<int16_t> position, Point<int16_t> marker_adjust) const;
		void update(Point<int16_t> parentpos);
		void send_key(KeyType::Id type, int32_t code, bool down);
		void add_string(const std::string& str);
		// Inserts one Unicode character as UTF-8. Non-ASCII text can only be
		// entered through here (see UI::send_char).
		void add_codepoint(uint32_t codepoint);

		void set_state(State state);
		void change_text(const std::string& text);
		void set_cryptchar(int8_t character);
		// Wrap typed text at the given pixel width (0 = single line)
		void set_wrap(uint16_t width);
		int16_t text_height() const;

		void set_enter_callback(std::function<void(std::string)> onreturn);
		void set_key_callback(KeyAction::Id key, std::function<void(void)> action);
		void set_text_callback(std::function<void(void)> action);

		Cursor::State send_cursor(Point<int16_t> cursorpos, bool clicked);

		bool empty() const;
		State get_state() const;
		Rectangle<int16_t> get_bounds() const;
		const std::string& get_text() const;
		std::string get_selected_text() const;
		bool can_copy_paste() const;

	private:
		void modifytext(const std::string& t);
		bool belowlimit() const;
		// Byte offsets of the character boundaries either side of `pos`.
		// The caret and every edit must land on a boundary: a UTF-8 Hebrew
		// character is two bytes, so byte-wise editing would split it and
		// leave an invalid string behind.
		size_t prev_boundary(size_t pos) const;
		size_t next_boundary(size_t pos) const;
		size_t index_at(int16_t cursor_x) const;
		bool has_selection() const;
		bool selectable() const;
		void erase_selection();
		void clear_selection();

		Text textlabel;
		Text::Font font = Text::A11M;
		Text::Alignment alignment = Text::LEFT;
		Color::Name text_color = Color::Name::BLACK;
		uint16_t wrap_width = 0;
		std::string text;
		ColorLine marker;
		bool showmarker;
		uint16_t elapsed;
		size_t markerpos;
		size_t sel_anchor = 0;
		size_t sel_end = 0;
		bool selecting = false;
		Rectangle<int16_t> bounds;
		Point<int16_t> parentpos;
		size_t limit;
		int8_t crypt;
		State state;

		std::function<void(std::string)> onreturn;
		std::map<int32_t, std::function<void(void)>> callbacks;
		std::function<void(void)> ontext;
	};
}