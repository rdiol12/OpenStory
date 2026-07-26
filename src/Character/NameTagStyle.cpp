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
#include "NameTagStyle.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	NameTagStyle::NameTagStyle(const std::string& name) : namelabel(Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NAMETAG, name))
	{
		// Default nametag color is plain white. Player/OtherChar call
		// apply(is_gm) after construction to load the real NameTag.img style
		// (w/c/e sprite pieces + clr). We drop Text::Background::NAMETAG
		// (programmatic black rectangle) there and instead paint the real
		// sprite pieces in draw.
		name_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	}

	void NameTagStyle::draw(Point<int16_t> absp) const
	{
		// If ever changing code for namelabel confirm placements with map 10000
		// v83 nametag is a 9-slice sprite from NameTag.img/<style>:
		//   w (left edge)  — origin is its right corner  (pivot on the c join)
		//   c (center)     — origin is top-left; we stretch it to text width
		//   e (right edge) — origin is top-left          (pivot on the c join)
		// Drawing all three at the text anchor point lets each sprite's
		// origin offset do the vertical alignment automatically, so the bar
		// sits centered on the name glyphs. The name is drawn last, on top.
		Point<int16_t> name_center = absp + Point<int16_t>(0, -4);
		int16_t text_w = namelabel.width();

		if (tag_w.is_valid() || tag_c.is_valid() || tag_e.is_valid())
		{
			int16_t c_total = text_w > 0 ? text_w : 1;
			int16_t half = c_total / 2;
			// Nudge the sprite down a few px so the bar sits below the
			// glyph baseline rather than clipping the ascenders.
			int16_t tag_y = name_center.y() + 5;

			// Left edge (pivot on its right → draw at left-of-center so its
			// right edge meets the center piece).
			tag_w.draw(DrawArgument(Point<int16_t>(
				name_center.x() - half, tag_y)));
			// Stretched center, spanning the text width.
			tag_c.draw(DrawArgument(
				Point<int16_t>(name_center.x() - half, tag_y),
				Point<int16_t>(c_total, 0)));
			// Right edge (pivot on its left, draws from center to right).
			tag_e.draw(DrawArgument(Point<int16_t>(
				name_center.x() + half, tag_y)));
		}

		// Pass name_color via DrawArgument so drawtext multiplies the glyph
		// wordcolor (WHITE) by it, yielding the exact ARGB from NameTag.img.
		namelabel.draw(DrawArgument(name_center, name_color));
	}

	void NameTagStyle::apply(bool is_gm)
	{
#ifdef USE_NX
		// The GM set effect is driven by the equipped GM hat (see
		// Char::refresh_ring_effect), not by GM status. Only the name plate here.

		// Like the original game: the decorative name plate is not for everyone.
		// Regular players keep the plain default name (translucent background).
		// Only GMs get a distinct plate.
		if (!is_gm)
			return;

		// First existing style wins ("11" is the distinct GM plate if present,
		// otherwise fall back to whatever the NX actually carries).
		nl::node nt = nl::nx::ui["NameTag.img"];
		nl::node style;

		for (const char* k : { "11", "0", "1", "2", "3", "4", "5", "10" })
		{
			if (nt[k] && nt[k].size() > 0)
			{
				style = nt[k];
				break;
			}
		}

		if (!style)
			return;

		// The plate provides its own background — drop the default one so they
		// don't stack.
		namelabel = Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NONE, namelabel.get_text());

		// Load the 9-slice sprite pieces (w = left edge, c = tiled center,
		// e = right edge).
		tag_w = Texture(style["w"]);
		tag_c = Texture(style["c"]);
		tag_e = Texture(style["e"]);

		if (nl::node clr_node = style["clr"])
		{
			try
			{
				int64_t argb = clr_node.get_integer();
				uint32_t v = static_cast<uint32_t>(argb);
				uint8_t a = static_cast<uint8_t>((v >> 24) & 0xFF);
				uint8_t r = static_cast<uint8_t>((v >> 16) & 0xFF);
				uint8_t g = static_cast<uint8_t>((v >> 8) & 0xFF);
				uint8_t b = static_cast<uint8_t>(v & 0xFF);
				if (a == 0) a = 255;
				name_color = Color(r, g, b, a);
			}
			catch (...) {}
		}
#else
		(void)is_gm;
#endif
	}

	std::string NameTagStyle::get_name() const
	{
		return namelabel.get_text();
	}
}
