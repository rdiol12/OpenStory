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
#include "UIClock.h"

#include "../../Constants.h"
#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIClock::UIClock()
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["muruengRaid"];

		// Clock background sprite
		clock_bg = src["clock"]["0"];
		bg_width = clock_bg.get_dimensions().x();
		bg_height = clock_bg.get_dimensions().y();

		// Digit sprites 0-9
		nl::node numbers = src["number"];
		for (int i = 0; i < 10; i++)
			digit[i] = numbers[std::to_string(i)];

		digit_width = digit[0].get_dimensions().x();

		// Colon separator
		colon = numbers["bar"];
		colon_width = colon.get_dimensions().x();

		// Top-center; the bg origin (102,26) centers it on this point,
		// so y must clear half the bg height or the top gets cut off
		int16_t screen_w = Constants::Constants::get().get_viewwidth();
		position = Point<int16_t>(screen_w / 2, 32);

		last_update_time = 0;
	}

	void UIClock::draw(float alpha) const
	{
		const Stage& stage = Stage::get();

		if (!stage.is_clock_active() && !stage.is_countdown_active())
			return;

		// Draw clock background centered at position
		clock_bg.draw(DrawArgument(position));

		// Calculate digit positions centered in the background
		// The background origin is (102, 26) so it draws centered at position
		// Digit/colon sprites carry centered origins, so each is drawn at
		// its own center point on the bg's center line
		int16_t cy = position.y();

		if (stage.is_clock_active())
		{
			// HH:MM:SS — 6 digits + 2 colons
			int16_t total_w = 6 * digit_width + 2 * colon_width;
			int16_t x = position.x() - total_w / 2;

			draw_number(stage.get_clock_hour(), 2, Point<int16_t>(x, cy));
			x += 2 * digit_width;

			colon.draw(DrawArgument(Point<int16_t>(x + colon_width / 2, cy)));
			x += colon_width;

			draw_number(stage.get_clock_min(), 2, Point<int16_t>(x, cy));
			x += 2 * digit_width;

			colon.draw(DrawArgument(Point<int16_t>(x + colon_width / 2, cy)));
			x += colon_width;

			draw_number(stage.get_clock_sec(), 2, Point<int16_t>(x, cy));
		}
		else if (stage.is_countdown_active())
		{
			int32_t seconds = stage.get_countdown_seconds();
			int32_t mins = seconds / 60;
			int32_t secs = seconds % 60;

			// Long countdowns show hours as well
			if (mins >= 100)
			{
				int32_t hours = mins / 60;
				mins %= 60;

				int16_t total_w = 6 * digit_width + 2 * colon_width;
				int16_t x = position.x() - total_w / 2;

				draw_number(hours, 2, Point<int16_t>(x, cy));
				x += 2 * digit_width;

				colon.draw(DrawArgument(Point<int16_t>(x + colon_width / 2, cy)));
				x += colon_width;

				draw_number(mins, 2, Point<int16_t>(x, cy));
				x += 2 * digit_width;

				colon.draw(DrawArgument(Point<int16_t>(x + colon_width / 2, cy)));
				x += colon_width;

				draw_number(secs, 2, Point<int16_t>(x, cy));
			}
			else
			{
				// MM:SS — 4 digits + 1 colon
				int16_t total_w = 4 * digit_width + colon_width;
				int16_t x = position.x() - total_w / 2;

				draw_number(mins, 2, Point<int16_t>(x, cy));
				x += 2 * digit_width;

				colon.draw(DrawArgument(Point<int16_t>(x + colon_width / 2, cy)));
				x += colon_width;

				draw_number(secs, 2, Point<int16_t>(x, cy));
			}
		}
	}

	void UIClock::draw_number(int value, int digits, Point<int16_t> pos) const
	{
		// pos.x is the left edge, pos.y the center line; the digit origin
		// (half the sprite) centers each glyph on the point passed in
		int divisor = 1;
		for (int i = 1; i < digits; i++)
			divisor *= 10;

		int16_t x = pos.x();

		for (int i = 0; i < digits; i++)
		{
			int d = (value / divisor) % 10;
			digit[d].draw(DrawArgument(Point<int16_t>(x + digit_width / 2, pos.y())));
			x += digit_width;
			divisor /= 10;
		}
	}

	void UIClock::update()
	{
		const Stage& stage = Stage::get();

		if (!stage.is_clock_active() && !stage.is_countdown_active())
			return;

		// Decrement countdown each frame tick (TIMESTEP = 8ms)
		if (stage.is_countdown_active())
		{
			last_update_time += Constants::TIMESTEP;

			// Every 1000ms, decrement the countdown by 1 second
			if (last_update_time >= 1000)
			{
				last_update_time -= 1000;

				int32_t remaining = stage.get_countdown_seconds();

				if (remaining > 0)
					Stage::get().set_countdown(remaining - 1);
				else
					Stage::get().clear_clock();
			}
		}
	}

	void UIClock::update_screen(int16_t new_width, int16_t new_height)
	{
		position = Point<int16_t>(new_width / 2, 32);
	}

	UIElement::Type UIClock::get_type() const
	{
		return TYPE;
	}
}
