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

#include <iomanip>
#include <sstream>

namespace ms
{
	UIClock::UIClock()
	{
		int16_t width = Constants::Constants::get().get_viewwidth();
		int16_t height = Constants::Constants::get().get_viewheight();

		// Position in the top-center area of the screen
		position = Point<int16_t>(width / 2 - BG_WIDTH / 2, 10);

		background = ColorBox(BG_WIDTH, BG_HEIGHT, Color::Name::BLACK, 0.6f);

		clock_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE);
		clock_shadow = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::BLACK);

		last_update_time = 0;

		update_text();
	}

	void UIClock::draw(float alpha) const
	{
		const Stage& stage = Stage::get();

		if (!stage.is_clock_active() && !stage.is_countdown_active())
			return;

		// Draw semi-transparent background
		background.draw(DrawArgument(position));

		// Draw text centered in the background
		Point<int16_t> text_pos = position + Point<int16_t>(BG_WIDTH / 2, BG_PADDING);

		clock_shadow.draw(DrawArgument(text_pos + Point<int16_t>(1, 1)));
		clock_text.draw(DrawArgument(text_pos));
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

		update_text();
	}

	void UIClock::update_text()
	{
		const Stage& stage = Stage::get();

		std::ostringstream oss;

		if (stage.is_clock_active())
		{
			oss << std::setfill('0') << std::setw(2) << (int)stage.get_clock_hour() << ":"
				<< std::setfill('0') << std::setw(2) << (int)stage.get_clock_min() << ":"
				<< std::setfill('0') << std::setw(2) << (int)stage.get_clock_sec();
		}
		else if (stage.is_countdown_active())
		{
			int32_t seconds = stage.get_countdown_seconds();
			int32_t mins = seconds / 60;
			int32_t secs = seconds % 60;

			oss << "Time: "
				<< std::setfill('0') << std::setw(2) << mins << ":"
				<< std::setfill('0') << std::setw(2) << secs;
		}
		else
		{
			return;
		}

		std::string text = oss.str();
		clock_text.change_text(text);
		clock_shadow.change_text(text);
	}

	void UIClock::update_screen(int16_t new_width, int16_t new_height)
	{
		position = Point<int16_t>(new_width / 2 - BG_WIDTH / 2, 10);
	}

	UIElement::Type UIClock::get_type() const
	{
		return TYPE;
	}
}
