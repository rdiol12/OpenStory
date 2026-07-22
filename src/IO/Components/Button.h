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

namespace ms
{
	// Base class for different button types.
	class Button
	{
	public:
		enum State
		{
			NORMAL,
			DISABLED,
			MOUSEOVER,
			PRESSED,
			KEYFOCUSED,
			IDENTITY,
			NUM_STATES
		};

		virtual ~Button() {}

		virtual void draw(Point<int16_t> parentpos) const = 0;
		virtual void update() = 0;
		virtual Rectangle<int16_t> bounds(Point<int16_t> parentpos) const = 0;
		virtual int16_t width() const = 0;
		virtual Point<int16_t> origin() const = 0;
		virtual Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) = 0;
		virtual bool in_combobox(Point<int16_t> cursorpos);
		virtual uint16_t get_selected() const;

		void set_position(Point<int16_t> position);
		virtual void set_state(State state);
		void set_active(bool active);
		void toggle_pressed();
		// Uniform draw/bounds scale, used by login-flow screens that render
		// the whole 800x600 design scaled up for large views
		void set_scale(float scale) { btn_scale = scale; }

		bool is_active() const;
		State get_state() const;
		bool is_pressed() const;

	protected:
		State state;
		Point<int16_t> position;
		bool active;
		bool pressed;
		float btn_scale = 1.0f;
	};
}