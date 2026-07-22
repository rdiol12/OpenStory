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
#include "TwoSpriteButton.h"

namespace ms
{
	TwoSpriteButton::TwoSpriteButton(nl::node nsrc, nl::node ssrc, Point<int16_t> np, Point<int16_t> sp) : textures(ssrc, nsrc), npos(np), spos(sp)
	{
		state = Button::State::NORMAL;
		active = true;
	}

	TwoSpriteButton::TwoSpriteButton(nl::node nsrc, nl::node ssrc, Point<int16_t> pos) : TwoSpriteButton(nsrc, ssrc, pos, pos) {}
	TwoSpriteButton::TwoSpriteButton(nl::node nsrc, nl::node ssrc) : TwoSpriteButton(nsrc, ssrc, Point<int16_t>()) {}
	TwoSpriteButton::TwoSpriteButton() : textures({}, {}) {}

	void TwoSpriteButton::draw(Point<int16_t> parentpos) const
	{
		if (active)
		{
			bool selected = state == Button::State::MOUSEOVER || state == Button::State::PRESSED;
			Point<int16_t> pos = selected ? spos : npos;

			textures[selected].draw(DrawArgument(pos + parentpos, btn_scale, btn_scale));
		}
	}

	Rectangle<int16_t> TwoSpriteButton::bounds(Point<int16_t> parentpos) const
	{
		bool selected = state == Button::State::MOUSEOVER || state == Button::State::PRESSED;
		Point<int16_t> pos = selected ? spos : npos;
		Point<int16_t> origin = textures[selected].get_origin();
		Point<int16_t> dim = textures[selected].get_dimensions();

		Point<int16_t> absp = parentpos + pos - Point<int16_t>(
			static_cast<int16_t>(origin.x() * btn_scale),
			static_cast<int16_t>(origin.y() * btn_scale));
		Point<int16_t> rb = absp + Point<int16_t>(
			static_cast<int16_t>(dim.x() * btn_scale),
			static_cast<int16_t>(dim.y() * btn_scale));

		return Rectangle<int16_t>(absp, rb);
	}

	int16_t TwoSpriteButton::width() const
	{
		bool selected = state == Button::State::MOUSEOVER || state == Button::State::PRESSED;

		return textures[selected].width();
	}

	Point<int16_t> TwoSpriteButton::origin() const
	{
		bool selected = state == Button::State::MOUSEOVER || state == Button::State::PRESSED;

		return textures[selected].get_origin();
	}
}