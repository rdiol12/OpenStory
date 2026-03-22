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
#include "UIComboCounter.h"

#include "../../Constants.h"
#include "../../Gameplay/Stage.h"

namespace ms
{
	UIComboCounter::UIComboCounter() : displayed_combo(0), fade_timer(0)
	{
		int16_t width = Constants::Constants::get().get_viewwidth();

		// Position in the right side of the screen, below the buff list area
		position = Point<int16_t>(width - BG_WIDTH - 20, 150);

		background = ColorBox(BG_WIDTH, BG_HEIGHT, Color::Name::BLACK, 0.6f);

		label_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, "COMBO");
		label_shadow = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, "COMBO");

		combo_text = Text(Text::Font::A18M, Text::Alignment::CENTER, Color::Name::WHITE);
		combo_shadow = Text(Text::Font::A18M, Text::Alignment::CENTER, Color::Name::BLACK);

		update_text();
	}

	void UIComboCounter::draw(float alpha) const
	{
		const Stage& stage = Stage::get();

		if (!stage.is_combo_active() && fade_timer <= 0)
			return;

		// Draw semi-transparent background
		background.draw(DrawArgument(position));

		// Draw "COMBO" label
		Point<int16_t> label_pos = position + Point<int16_t>(BG_WIDTH / 2, 4);
		label_shadow.draw(DrawArgument(label_pos + Point<int16_t>(1, 1)));
		label_text.draw(DrawArgument(label_pos));

		// Draw combo count
		Point<int16_t> count_pos = position + Point<int16_t>(BG_WIDTH / 2, 22);
		combo_shadow.draw(DrawArgument(count_pos + Point<int16_t>(1, 1)));
		combo_text.draw(DrawArgument(count_pos));
	}

	void UIComboCounter::update()
	{
		const Stage& stage = Stage::get();
		int32_t current = stage.get_combo();

		if (current != displayed_combo)
		{
			displayed_combo = current;
			fade_timer = FADE_DELAY;
			update_text();
		}
		else if (fade_timer > 0)
		{
			fade_timer -= Constants::TIMESTEP;

			if (fade_timer <= 0 && !stage.is_combo_active())
				fade_timer = 0;
		}
	}

	void UIComboCounter::update_text()
	{
		std::string text = std::to_string(displayed_combo);
		combo_text.change_text(text);
		combo_shadow.change_text(text);
	}

	void UIComboCounter::update_screen(int16_t new_width, int16_t new_height)
	{
		position = Point<int16_t>(new_width - BG_WIDTH - 20, 150);
	}

	UIElement::Type UIComboCounter::get_type() const
	{
		return TYPE;
	}
}
