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
#include "MapleButton.h"

namespace ms
{
	MapleButton::MapleButton(nl::node src, Point<int16_t> pos)
	{
		// Any state with several frames is an animation (e.g. the v83 world
		// planks wiggle on mouse-over)
		auto load_state = [&](Button::State st, nl::node node)
		{
			if (node.size() > 1)
				animations[st] = node;
			else
				textures[st] = node["0"];
		};

		load_state(Button::State::NORMAL, src["normal"]);
		load_state(Button::State::PRESSED, src["pressed"]);
		load_state(Button::State::MOUSEOVER, src["mouseOver"]);
		load_state(Button::State::DISABLED, src["disabled"]);

		if (src["keyFocused"])
			load_state(Button::State::KEYFOCUSED, src["keyFocused"]);

		// A mouse-over animation plays through once and freezes on its last
		// frame; leaving and re-entering the state replays it (set_state
		// rewinds it)
		animations[Button::State::MOUSEOVER].set_hold_last(true);

		active = true;
		position = pos;
		state = Button::State::NORMAL;
	}

	MapleButton::MapleButton(nl::node src, int16_t x, int16_t y) : MapleButton(src, Point<int16_t>(x, y)) {}
	MapleButton::MapleButton(nl::node src) : MapleButton(src, Point<int16_t>()) {}

	void MapleButton::draw(Point<int16_t> parentpos) const
	{
		if (active)
		{
			textures[state].draw(DrawArgument(position + parentpos, btn_scale, btn_scale));
			animations[state].draw(DrawArgument(position + parentpos, btn_scale, btn_scale), 1.0f);
		}
	}

	void MapleButton::draw(Point<int16_t> parentpos, float alpha) const
	{
		if (alpha <= 0.0f)
			return;

		textures[state].draw(DrawArgument(position + parentpos, btn_scale, btn_scale, alpha));
		animations[state].draw(DrawArgument(position + parentpos, btn_scale, btn_scale, alpha), 1.0f);
	}

	void MapleButton::update()
	{
		// Real timestep so every animation plays through at authored speed
		if (active)
			animations[state].update();
	}

	void MapleButton::set_state(Button::State st)
	{
		if (st != state)
			animations[st].reset();

		Button::set_state(st);
	}

	Rectangle<int16_t> MapleButton::bounds(Point<int16_t> parentpos) const
	{
		Point<int16_t> origin = textures[state].is_valid()
			? textures[state].get_origin()
			: animations[state].get_origin();
		Point<int16_t> dim = textures[state].is_valid()
			? textures[state].get_dimensions()
			: animations[state].get_dimensions();

		Point<int16_t> lt = parentpos + position - Point<int16_t>(
			static_cast<int16_t>(origin.x() * btn_scale),
			static_cast<int16_t>(origin.y() * btn_scale));
		Point<int16_t> rb = lt + Point<int16_t>(
			static_cast<int16_t>(dim.x() * btn_scale),
			static_cast<int16_t>(dim.y() * btn_scale));

		return Rectangle<int16_t>(lt, rb);
	}

	int16_t MapleButton::width() const
	{
		return textures[state].width();
	}

	Point<int16_t> MapleButton::origin() const
	{
		return textures[state].get_origin();
	}
}