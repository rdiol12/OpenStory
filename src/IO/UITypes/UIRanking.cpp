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
#include "../Components/TwoSpriteButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIRanking::UIRanking() : UIDragElement<PosRANKING>(), tab(Buttons::BT_TAB0)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["Ranking"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = src["backgrnd3"];

		if (backgrnd3.size() > 0)
			sprites.emplace_back(backgrnd3);

		// Close button positioned at top-right of background
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));

		// Tab buttons — try Tab node with enabled/disabled states first
		nl::node tab_node = src["Tab"];

		if (tab_node && tab_node["enabled"].size() > 0)
		{
			nl::node taben = tab_node["enabled"];
			nl::node tabdis = tab_node["disabled"];

			buttons[Buttons::BT_TAB0] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
			buttons[Buttons::BT_TAB1] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
			buttons[Buttons::BT_TAB2] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
		}
		else
		{
			// Fall back to BtTab buttons
			buttons[Buttons::BT_TAB0] = std::make_unique<MapleButton>(src["BtTab0"]);
			buttons[Buttons::BT_TAB1] = std::make_unique<MapleButton>(src["BtTab1"]);
			buttons[Buttons::BT_TAB2] = std::make_unique<MapleButton>(src["BtTab2"]);
		}

		// Prev/Next page buttons
		nl::node bt_prev = src["BtPrev"];
		nl::node bt_next = src["BtNext"];

		if (bt_prev.size() > 0)
			buttons[Buttons::BT_PREV] = std::make_unique<MapleButton>(bt_prev);

		if (bt_next.size() > 0)
			buttons[Buttons::BT_NEXT] = std::make_unique<MapleButton>(bt_next);

		// Start on the first tab
		change_tab(Buttons::BT_TAB0);

		// Placeholder text shown in the list area
		empty_text = Text(Text::Font::A11M, Text::Alignment::CENTER,
			Color::Name::GRAY, "No ranking data available.");

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIRanking::draw(float inter) const
	{
		UIElement::draw(inter);

		// Draw placeholder text centered in the list area
		empty_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2));
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
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
			change_tab(buttonid);
			return Button::State::PRESSED;
		case Buttons::BT_PREV:
			// No-op: no ranking data to page through
			break;
		case Buttons::BT_NEXT:
			// No-op: no ranking data to page through
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIRanking::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			// Deselect the old tab
			if (buttons.count(oldtab) && buttons[oldtab])
				buttons[oldtab]->set_state(Button::State::NORMAL);
		}

		// Select the new tab
		if (buttons.count(tab) && buttons[tab])
			buttons[tab]->set_state(Button::State::PRESSED);
	}
}
