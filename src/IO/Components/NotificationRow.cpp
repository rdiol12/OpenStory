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
#include "NotificationRow.h"

#include "../../Graphics/DrawArgument.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void NotificationRow::load()
	{
		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];

		backdrop = Texture(root["backgrnd"]);
		icon     = Texture(root["icon0"]);
		dims = backdrop.is_valid()
			? backdrop.get_dimensions()
			: Point<int16_t>(218, 66);

		int16_t maxw = text_area_width();
		title_text = Text(Text::Font::A11B, Text::Alignment::LEFT,
			Color::Name::WHITE, "", maxw);
		body_text  = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, "", maxw);
	}

	std::unique_ptr<MapleButton> NotificationRow::accept_button() const
	{
		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];
		return std::make_unique<MapleButton>(root["BtOK"],
			Point<int16_t>(dims.x() - BTN_X, BTN_TOP_Y));
	}

	std::unique_ptr<MapleButton> NotificationRow::decline_button() const
	{
		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];
		return std::make_unique<MapleButton>(root["BtCancel"],
			Point<int16_t>(dims.x() - BTN_X, BTN_TOP_Y + BTN_ROW_GAP));
	}

	int16_t NotificationRow::text_area_width() const
	{
		int16_t icon_w = icon.is_valid() ? icon.get_dimensions().x() : 0;
		int16_t row_w = backdrop.is_valid()
			? backdrop.get_dimensions().x()
			: 218;
		return row_w - 6 - icon_w - 6 - 6;
	}

	void NotificationRow::draw(Point<int16_t> pos, const std::string& title,
		const std::string& body, float opacity) const
	{
		if (backdrop.is_valid())
			backdrop.draw(DrawArgument(pos, opacity));

		int16_t icon_w = 0;
		if (icon.is_valid())
		{
			icon.draw(DrawArgument(pos + Point<int16_t>(6, 6), opacity));
			icon_w = icon.get_dimensions().x();
		}

		int16_t text_x = 6 + icon_w + 6;
		title_text.change_text(title);
		title_text.draw(pos + Point<int16_t>(text_x, 0));
		body_text.change_text(body);
		// Body sits just under the title; Text auto-wraps at maxwidth
		// so short messages render as a single line and only long
		// ones break onto a second.
		body_text.draw(pos + Point<int16_t>(text_x, 16));
	}
}
