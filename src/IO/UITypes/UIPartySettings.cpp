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
#include "UIPartySettings.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartySettings::UIPartySettings(bool make_mode_)
		: UIDragElement<PosPARTYSETTINGS>(Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::DRAGAREA_H)),
		  make_mode(make_mode_)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"]["PopupSettings"];

		sprites.emplace_back(src["backgrnd"]);
		chrome.load(src);

		buttons[BT_OK]     = std::make_unique<MapleButton>(src["BtSave"]);
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		dimension = Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::HEIGHT);
		dragarea  = Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::DRAGAREA_H);
	}

	void UIPartySettings::draw(float inter) const
	{
		UIElement::draw(inter);

		// Title sprite — already origin-shifted in NX so we just draw
		// at `position`. The chrome handles make-vs-settings selection.
		chrome.draw_title(position, make_mode);
	}

	void UIPartySettings::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed) return;
		if (escape)
		{
			deactivate();
			return;
		}
	}

	UIElement::Type UIPartySettings::get_type() const
	{
		return TYPE;
	}

	Button::State UIPartySettings::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_OK:
			// Cosmic v83 has no persisted party settings to write back —
			// retail used this for /Party Search/ visibility + level
			// range, neither of which is server-side configurable here.
			deactivate();
			break;
		case BT_CANCEL:
			deactivate();
			break;
		}
		return Button::State::NORMAL;
	}
}
