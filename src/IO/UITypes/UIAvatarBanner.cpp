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
#include "UIAvatarBanner.h"

#include "../../Constants.h"
#include "../../Graphics/Geometry.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		// Map 5390xxx cash item ids to the variant node names at
		// Map.nx/MapHelper.img/AvatarMegaphone.
		const char* variant_for(int32_t itemid)
		{
			switch (itemid)
			{
			case 5390000: return "Burning";
			case 5390001: return "Bright";
			case 5390002: return "Heart";
			case 5390005: return "Tiger1";
			case 5390006: return "Tiger2";
			default:      return "Burning";
			}
		}
	}

	UIAvatarBanner::UIAvatarBanner() : remaining_ms(0)
	{
		dimension = Point<int16_t>(225, 180);

		// Right side of the screen, below the top HUD.
		// Anchored at the top-right of the viewport (pushed slightly
		// further right).
		int16_t vw = Constants::Constants::get().get_viewwidth();
		position = Point<int16_t>(vw - dimension.x() + 4, 4);

		// White bold sender label — painted on top of the name-tag bar.
		sender_text = Text(Text::Font::A12B, Text::Alignment::CENTER,
			Color::Name::WHITE, "", 200);
	}

	void UIAvatarBanner::show(const std::string& sender,
		const std::vector<std::string>& lines,
		int32_t itemid,
		int32_t duration_ms,
		const LookEntry& look)
	{
#ifdef USE_NX
		nl::node root = nl::nx::map["MapHelper.img"]["AvatarMegaphone"];
		variant_anim = Animation(root[variant_for(itemid)]);
		name_bar = Texture(root["name"]["1"]);
#endif

		sender_text.change_text(sender);

		line_texts.clear();
		// Each server message becomes one Text with a fixed max-width so it
		// word-wraps (never mid-word) across the dotted rows of the banner
		// sprite. The sprite has 3 dotted rows; when a single message
		// wraps onto more than one visible row, the following message
		// continues from the next free row. Messages past 3 rows drop.
		const uint16_t ROW_WIDTH = 130;
		for (const std::string& ln : lines)
		{
			if (ln.empty()) continue;
			line_texts.emplace_back(Text::Font::A11M, Text::Alignment::CENTER,
				Color::Name::BLACK, Text::Background::NONE,
				ln, ROW_WIDTH, /*formatted=*/false);
		}

		// Build a live CharLook from the payload so we can render the
		// broadcaster's actual avatar inside the frame.
		sender_look = std::make_unique<CharLook>(look);

		remaining_ms = duration_ms > 0 ? duration_ms : 10000;

		// Anchored at the top-right of the viewport (pushed slightly
		// further right).
		int16_t vw = Constants::Constants::get().get_viewwidth();
		position = Point<int16_t>(vw - dimension.x() + 4, 4);

		makeactive();
	}

	void UIAvatarBanner::draw(float inter) const
	{
		variant_anim.draw(DrawArgument(position), inter);

		// Sender name sits at the very top of the banner over the nameTag
		// bar (moved up + rendered in white per spec).
		name_bar.draw(DrawArgument(position + Point<int16_t>(34, -6)));
		sender_text.draw(position + Point<int16_t>(dimension.x() / 2, -10));

		// Avatar on the LEFT half, nudged a bit to the right from the far
		// edge of the frame.
		if (sender_look)
			sender_look->draw(
				position + Point<int16_t>(70, 92),
				false, Stance::Id::STAND1, Expression::Id::DEFAULT);

		// Message area. The sprite has exactly 3 dotted rows at fixed Y
		// positions. Each server message becomes one Text that word-wraps
		// at the column width; every wrapped visual line sits above its
		// own dot in order. Messages that would overflow past row 3 are
		// dropped.
		const int16_t MSG_ROW_CX     = 156;
		const int16_t DOT_ROW_Y[3]   = { 20, 52, 84 };
		constexpr int MAX_ROWS       = 3;

		int row = 0;
		for (const Text& t : line_texts)
		{
			if (row >= MAX_ROWS) break;

			t.draw(position + Point<int16_t>(MSG_ROW_CX, DOT_ROW_Y[row]));

			// If this message word-wrapped into multiple visible lines,
			// the Text's own height() tells us how much vertical space
			// it consumed — convert that into "dot rows used".
			int rows_used = 1;
			if (t.height() > 22) rows_used = 2;
			if (t.height() > 36) rows_used = 3;
			row += rows_used;
		}
	}

	void UIAvatarBanner::update()
	{
		UIElement::update();
		variant_anim.update();

		if (remaining_ms > 0)
		{
			remaining_ms -= static_cast<int32_t>(Constants::TIMESTEP);
			if (remaining_ms <= 0)
				deactivate();
		}
	}

	UIElement::Type UIAvatarBanner::get_type() const
	{
		return TYPE;
	}
}
