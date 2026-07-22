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
#include "UIRanking.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIRanking::UIRanking() : UIDragElement<PosRANKING>(Point<int16_t>(303, 25))
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["Ranking"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 21, 7));

		name_text = Text(Text::Font::A13B, Text::Alignment::CENTER, Color::Name::BLACK);
		label_text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		value_text = Text(Text::Font::A12B, Text::Alignment::RIGHT, Color::Name::BLACK);
		hint_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DUSTYGRAY, "Ranks update with the server's ranking sweep.", 260);

		divider = nl::nx::ui["UIWindow2.img"]["MemoInGame"]["Get"]["line"];

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 25);
	}

	void UIRanking::draw(float inter) const
	{
		UIElement::draw(inter);

		const CharStats& stats = Stage::get().get_player().get_stats();

		// A movement char rides along with each rank ('+'/'-'/'=');
		// climbing shows green, dropping red
		auto rank_string = [](std::pair<int32_t, int8_t> r) -> std::string
		{
			if (r.first <= 0)
				return "Unranked";

			std::string s = "#" + std::to_string(r.first);

			if (r.second == '+')
				s += "  (up)";
			else if (r.second == '-')
				s += "  (down)";

			return s;
		};

		auto rank_color = [](std::pair<int32_t, int8_t> r) -> Color::Name
		{
			if (r.first <= 0)
				return Color::Name::DUSTYGRAY;
			if (r.second == '+')
				return Color::Name::JAPANESELAUREL;
			if (r.second == '-')
				return Color::Name::RED;

			return Color::Name::BLACK;
		};

		name_text.change_text(stats.get_name());
		name_text.draw(position + Point<int16_t>(dimension.x() / 2, 62));

		struct Row
		{
			const char* label;
			std::string value;
			Color::Name color;
		};

		Row rows[5] = {
			{ "Job", stats.get_jobname(), Color::Name::BLACK },
			{ "Overall Rank", rank_string(stats.get_rank()), rank_color(stats.get_rank()) },
			{ "Job Rank", rank_string(stats.get_jobrank()), rank_color(stats.get_jobrank()) },
			{ "Level", std::to_string(stats.get_stat(MapleStat::Id::LEVEL)), Color::Name::BLACK },
			{ "Fame", std::to_string(static_cast<int16_t>(stats.get_stat(MapleStat::Id::FAME))), Color::Name::BLACK }
		};

		for (int i = 0; i < 5; i++)
		{
			int16_t y = ROW_Y + i * ROW_STEP;

			label_text.change_text(rows[i].label);
			label_text.draw(position + Point<int16_t>(ROW_X, y));

			value_text.change_color(rows[i].color);
			value_text.change_text(rows[i].value);
			value_text.draw(position + Point<int16_t>(VALUE_X, y));

			divider.draw(DrawArgument(
				position + Point<int16_t>(ROW_X, y + 26) + divider.get_origin(),
				Point<int16_t>(255, 0)));
		}

		hint_text.draw(position + Point<int16_t>(dimension.x() / 2, ROW_Y + 5 * ROW_STEP + 4));
	}

	void UIRanking::update()
	{
		UIElement::update();
	}

	void UIRanking::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UIRanking::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		return UIDragElement::send_cursor(clicking, cursorpos);
	}

	UIElement::Type UIRanking::get_type() const
	{
		return TYPE;
	}

	Button::State UIRanking::button_pressed(uint16_t buttonid)
	{
		if (buttonid == BT_CLOSE)
			deactivate();

		return Button::State::NORMAL;
	}
}
