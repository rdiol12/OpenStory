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
#include "Gauge.h"

#include "../../Graphics/Geometry.h"

#include <cmath>

namespace ms
{
	Gauge::Gauge(Type type, Texture front, int16_t max, float percent) : type(type), barfront(front), maximum(max), percentage(percent)
	{
		target = percentage;
	}

	Gauge::Gauge(Type type, Texture front, Texture mid, int16_t max, float percent) : type(type), barfront(front), barmid(mid), maximum(max), percentage(percent)
	{
		target = percentage;
	}

	Gauge::Gauge(Type type, Texture front, Texture mid, Texture end, int16_t max, float percent) : type(type), barfront(front), barmid(mid), barend(end), maximum(max), percentage(percent)
	{
		target = percentage;
	}

	Gauge::Gauge() {}

	void Gauge::draw(const DrawArgument& args) const
	{
		int16_t length = static_cast<int16_t>(percentage * maximum);

		if (length > 0)
		{
			if (type == Type::GAME)
			{
				barfront.draw(args + DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(length, 0)));
				barmid.draw(args);
				// v83 gauge end-cap (2x13) sits flush at the tip of the fill.
				// (The original +8,+20 offset targeted post-Big-Bang gauges.)
				barend.draw(args + Point<int16_t>(length, 0));

				// Animated glint — a thin bright band sweeps left→right
				// across the fill every ~2.3s, drawn on top of the
				// gauge sprite so it reads as a glossy highlight.
				constexpr int16_t SHINE_PERIOD = 140;
				constexpr int16_t SHINE_WIDTH = 6;
				constexpr int16_t SHINE_START_PAD = 20;
				shine_tick = (shine_tick + 1) % SHINE_PERIOD;

				int16_t sweep_range = length + SHINE_START_PAD * 2;
				int16_t shine_x = static_cast<int16_t>(
					static_cast<int32_t>(shine_tick) * sweep_range / SHINE_PERIOD
				) - SHINE_START_PAD;

				if (shine_x > -SHINE_WIDTH && shine_x < length)
				{
					int16_t draw_x = std::max<int16_t>(shine_x, 0);
					int16_t draw_w = std::min<int16_t>(SHINE_WIDTH, length - draw_x);
					int16_t gauge_h = barfront.height() > 0 ? barfront.height() : 6;

					ColorBox glint(draw_w, gauge_h, Color::Name::WHITE, 0.40f);
					glint.draw(DrawArgument(args.getpos() + Point<int16_t>(draw_x, 0)));
				}
			}
			else if (type == Type::CASHSHOP)
			{
				Point<int16_t> pos_adj = Point<int16_t>(45, 1);

				barfront.draw(args - pos_adj);
				barmid.draw(args + DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(length, 0)));
				barend.draw(args - pos_adj + Point<int16_t>(length + barfront.width(), 0));
			}
		}
	}

	void Gauge::update(float t)
	{
		target = t;

		float diff = target - percentage;
		float absdiff = std::fabs(diff);

		if (absdiff < 0.0005f)
		{
			percentage = target;
			return;
		}

		// Exponential ease-out: close ~12% of the remaining gap each
		// tick. Big jumps ramp quickly then settle in smoothly, instead
		// of the old constant 1/24 linear crawl.
		float move = diff * 0.12f;

		// Floor the per-tick movement so small remaining gaps still
		// close in finite time instead of creeping forever.
		const float min_step = 0.0015f;
		if (std::fabs(move) < min_step)
			move = (diff > 0.0f) ? min_step : -min_step;

		// Don't overshoot.
		if ((diff > 0.0f && move > diff) || (diff < 0.0f && move < diff))
			move = diff;

		percentage += move;
	}
}