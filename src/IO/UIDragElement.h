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

#include "UIElement.h"

#include "../Configuration.h"

namespace ms
{
	template <typename T>
	// Base class for UI Windows which can be moved with the mouse cursor.
	class UIDragElement : public UIElement
	{
	public:
		void remove_cursor() override
		{
			UIElement::remove_cursor();

			if (dragged)
			{
				dragged = false;

				Setting<T>::get().save(position);
			}
		}

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override
		{
			if (clicked)
			{
				if (dragged)
				{
					position = cursorpos - cursoroffset;

					return Cursor::State::CLICKING;
				}
				else if (indragrange(cursorpos))
				{
					cursoroffset = cursorpos - position;
					dragged = true;

					return UIElement::send_cursor(clicked, cursorpos);
				}
			}
			else
			{
				if (dragged)
				{
					dragged = false;

					Setting<T>::get().save(position);
				}
			}

			return UIElement::send_cursor(clicked, cursorpos);
		}

		void update_screen(int16_t new_width, int16_t new_height) override
		{
			// Clamp position so the window stays visible on screen
			int16_t x = position.x();
			int16_t y = position.y();

			if (x + dimension.x() > new_width)
				x = std::max<int16_t>(0, new_width - dimension.x());
			if (y + dimension.y() > new_height)
				y = std::max<int16_t>(0, new_height - dimension.y());
			if (x < 0) x = 0;
			if (y < 0) y = 0;

			position = Point<int16_t>(x, y);
			Setting<T>::get().save(position);
		}

	protected:
		UIDragElement() : UIDragElement(Point<int16_t>(0, 0)) {}

		UIDragElement(Point<int16_t> d) : dragarea(d)
		{
			position = Setting<T>::get().load();
		}

		bool dragged = false;
		Point<int16_t> dragarea;
		Point<int16_t> cursoroffset;

	private:
		virtual bool indragrange(Point<int16_t> cursorpos) const
		{
			auto bounds = Rectangle<int16_t>(position, position + dragarea);

			return bounds.contains(cursorpos);
		}
	};
}