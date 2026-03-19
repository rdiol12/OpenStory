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
#include "UIWedding.h"

#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIWedding::UIWedding() : UIElement(Point<int16_t>(400, 300), Point<int16_t>(0, 0)),
		wedding_type(0), countdown(0)
	{
		nl::node wedding = nl::nx::ui["UIWindow.img"]["Wedding"];
		nl::node invite = wedding["Invitation"];
		nl::node counter = wedding["Counter"];

		cathedral_tex = invite["Cathedral"];
		vegas_tex = invite["Vegas"];
		stopwatch_tex = counter["StopWatch"];

		// Default to Cathedral
		Point<int16_t> bg_dimensions = cathedral_tex.get_dimensions();

		sprites.emplace_back(invite["Cathedral"]);

		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(invite["BtOK"]);

		names_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "");
		countdown_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, "");

		dimension = bg_dimensions;
	}

	void UIWedding::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> center = position + Point<int16_t>(dimension.x() / 2, 0);

		names_label.draw(center + Point<int16_t>(0, dimension.y() / 2 - 20));

		if (countdown > 0)
		{
			stopwatch_tex.draw(center + Point<int16_t>(0, dimension.y() / 2 + 10));
			countdown_label.draw(center + Point<int16_t>(0, dimension.y() / 2 + 30));
		}
	}

	void UIWedding::update()
	{
		UIElement::update();
	}

	void UIWedding::set_invitation(int8_t type, const std::string& groom, const std::string& bride)
	{
		wedding_type = type;
		groom_name = groom;
		bride_name = bride;

		names_label.change_text(groom + " & " + bride);

		// Swap background for Vegas type
		if (type == 1 && !sprites.empty())
		{
			sprites.clear();
			sprites.emplace_back(nl::nx::ui["UIWindow.img"]["Wedding"]["Invitation"]["Vegas"]);
			dimension = vegas_tex.get_dimensions();
		}
	}

	void UIWedding::set_countdown(int32_t seconds)
	{
		countdown = seconds;

		if (seconds > 0)
		{
			int32_t mins = seconds / 60;
			int32_t secs = seconds % 60;
			countdown_label.change_text(std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs));
		}
		else
		{
			countdown_label.change_text("");
		}
	}

	void UIWedding::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIWedding::get_type() const
	{
		return TYPE;
	}

	Button::State UIWedding::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_OK:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
