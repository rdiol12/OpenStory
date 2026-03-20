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
#include "UIMonsterCarnival.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMonsterCarnival::UIMonsterCarnival() : UIElement(Point<int16_t>(400, 50), Point<int16_t>(0, 0), ScaleMode::CENTER_OFFSET),
		my_cp(0), my_total_cp(0), enemy_cp(0), enemy_total_cp(0), my_team(0)
	{
		nl::node main = nl::nx::ui["UIWindow.img"]["MonsterCarnival"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = main["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_SIDE] = std::make_unique<MapleButton>(main["BtSide"]);

		icon0 = main["icon0"];
		icon1 = main["icon1"];
		icon2 = main["icon2"];
		icon3 = main["icon3"];

		for (int i = 0; i < 4; i++)
			summon_counts[i] = 0;

		my_cp_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::BLUE, "CP: 0 / 0");
		enemy_cp_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::RED, "CP: 0 / 0");
		team_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Team: ?");

		dimension = bg_dimensions;
	}

	void UIMonsterCarnival::draw(float inter) const
	{
		UIElement::draw(inter);

		// Draw category icons
		icon0.draw(position + Point<int16_t>(20, 40));
		icon1.draw(position + Point<int16_t>(20, 70));
		icon2.draw(position + Point<int16_t>(20, 100));
		icon3.draw(position + Point<int16_t>(20, 130));

		// Draw CP gauges
		my_cp_label.draw(position + Point<int16_t>(dimension.x() / 4, 160));
		enemy_cp_label.draw(position + Point<int16_t>(3 * dimension.x() / 4, 160));

		team_label.draw(position + Point<int16_t>(dimension.x() / 2, 20));
	}

	void UIMonsterCarnival::update()
	{
		UIElement::update();
	}

	void UIMonsterCarnival::set_cp(int32_t mcp, int32_t mtotal, int32_t ecp, int32_t etotal)
	{
		my_cp = mcp;
		my_total_cp = mtotal;
		enemy_cp = ecp;
		enemy_total_cp = etotal;

		my_cp_label.change_text("CP: " + std::to_string(my_cp) + " / " + std::to_string(my_total_cp));
		enemy_cp_label.change_text("CP: " + std::to_string(enemy_cp) + " / " + std::to_string(enemy_total_cp));
	}

	void UIMonsterCarnival::set_team(int8_t team)
	{
		my_team = team;
		team_label.change_text(team == 0 ? "Team: Maple Red" : "Team: Maple Blue");
	}

	void UIMonsterCarnival::set_summon_count(int8_t category, int16_t count)
	{
		if (category >= 0 && category < 4)
			summon_counts[category] = count;
	}

	Cursor::State UIMonsterCarnival::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIMonsterCarnival::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIMonsterCarnival::get_type() const
	{
		return TYPE;
	}

	Button::State UIMonsterCarnival::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_SIDE:
			side_view = !side_view;
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line("Side view toggled.", UIChatBar::YELLOW);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
