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
#include "../Constants.h"

#include <algorithm>

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

				save_drag_position();
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

					save_drag_position();
				}
			}

			return UIElement::send_cursor(clicked, cursorpos);
		}

		// The logical viewport changed (cash shop entry/exit, a resolution
		// switch). Re-anchor rather than merely clamp: a fixed-size panel keeps
		// the same fraction of the free space around it, which preserves
		// left/right/top/bottom edge anchoring and centring for free.
		void update_screen(int16_t new_width, int16_t new_height) override
		{
			reanchor(new_width, new_height);
		}

		// update_screen is only dispatched to *active* elements, so a window
		// that was closed across the change never hears about it. Catch up
		// here, just before it becomes visible again.
		void makeactive() override
		{
			reanchor(
				Constants::Constants::get().get_viewwidth(),
				Constants::Constants::get().get_viewheight());

			UIElement::makeactive();
		}

	protected:
		UIDragElement() : UIDragElement(Point<int16_t>(0, 0)) {}

		UIDragElement(Point<int16_t> d) : dragarea(d)
		{
			position = Setting<T>::get().load();

			// The reference layout is whatever the derived constructor ends up
			// with, measured against the viewport it was built for. anchor_pos
			// itself is captured lazily (see reanchor) because subclasses such
			// as UIKeyConfig / UIOptionMenu overwrite position after this runs.
			anchor_screen = Point<int16_t>(
				Constants::Constants::get().get_viewwidth(),
				Constants::Constants::get().get_viewheight());
		}

		// Records the window's current spot as the user's intent: persisted to
		// the config and used as the reference for any later re-anchoring.
		// Call this instead of saving the setting by hand.
		void save_drag_position()
		{
			anchor_pos = position;
			anchor_screen = Point<int16_t>(
				Constants::Constants::get().get_viewwidth(),
				Constants::Constants::get().get_viewheight());
			anchored = true;

			Setting<T>::get().save(position);
		}

		// Map the reference position into a viewport of the given size.
		//
		// Always derived from (anchor_pos, anchor_screen) rather than from the
		// current position, so repeated changes never accumulate rounding
		// error and returning to the original viewport restores the original
		// spot exactly. Nothing is written back to the config: only an actual
		// drag expresses where the user wants the window.
		void reanchor(int16_t new_width, int16_t new_height)
		{
			if (new_width <= 0 || new_height <= 0)
				return;

			if (!anchored)
			{
				anchor_pos = position;
				anchored = true;
			}

			if (anchor_screen.x() == new_width && anchor_screen.y() == new_height)
			{
				position = anchor_pos;
				return;
			}

			position = Point<int16_t>(
				reanchor_axis(anchor_pos.x(), dimension.x(), anchor_screen.x(), new_width),
				reanchor_axis(anchor_pos.y(), dimension.y(), anchor_screen.y(), new_height));
		}

		bool dragged = false;
		Point<int16_t> dragarea;
		Point<int16_t> cursoroffset;

	private:
		// Keep the same fraction of the slack (viewport minus window) on one
		// axis. Flush-left stays flush-left, flush-right stays flush-right,
		// centred stays centred, and the mapping is its own inverse, so a
		// cash-shop round trip is lossless.
		static int16_t reanchor_axis(int16_t pos, int16_t dim, int16_t old_screen, int16_t new_screen)
		{
			int32_t old_slack = static_cast<int32_t>(old_screen) - dim;
			int32_t new_slack = static_cast<int32_t>(new_screen) - dim;

			// Window is (or would be) wider/taller than the viewport: the only
			// sensible spot left is the origin.
			if (old_slack <= 0 || new_slack <= 0)
				return 0;

			int32_t p = pos;
			int32_t half = old_slack / 2;
			int32_t scaled = (p >= 0)
				? (p * new_slack + half) / old_slack
				: (p * new_slack - half) / old_slack;

			// Safety net: a window that started on screen must end on screen.
			// The formula above already guarantees this; the clamp only bites
			// for saved positions that were already out of bounds.
			if (p >= 0 && p <= old_slack)
				scaled = std::clamp<int32_t>(scaled, 0, new_slack);

			return static_cast<int16_t>(scaled);
		}

		Point<int16_t> anchor_pos;
		Point<int16_t> anchor_screen;
		bool anchored = false;

		virtual bool indragrange(Point<int16_t> cursorpos) const
		{
			Point<int16_t> area = dragarea;

			// If no drag area was specified, use the element's full width
			// with a 20px title bar height as a reasonable default
			if (area.x() == 0 && area.y() == 0)
				area = Point<int16_t>(dimension.x() > 0 ? dimension.x() : 300, 20);

			auto bounds = Rectangle<int16_t>(position, position + area);

			return bounds.contains(cursorpos);
		}
	};
}