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
#pragma once

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "MapleButton.h"

#include <memory>
#include <string>

namespace ms
{
	// Visual primitive for one row in the notification surfaces (the
	// floating UIToastStack and the bell-button UINotificationList drawer).
	// Owns the FadeYesNo backdrop, the message-type icon, the BtOK /
	// BtCancel buttons, and the title / body Text objects, plus the
	// constants that position them. Both surfaces look identical because
	// both delegate their per-row rendering here.
	struct NotificationRow
	{
		// Button layout (BtOK / BtCancel are 12x12 sprites stacked
		// vertically near the top-right of the row).
		static constexpr int16_t BTN_X = 16;
		static constexpr int16_t BTN_TOP_Y = 6;
		static constexpr int16_t BTN_ROW_GAP = 14;

		Texture backdrop;
		Texture icon;
		Point<int16_t> dims;

		// Lazily-set in the parent's button map; keys come from the
		// caller's own Buttons enum. Owned by the parent UIElement.
		std::unique_ptr<MapleButton> accept_button() const;
		std::unique_ptr<MapleButton> decline_button() const;

		mutable Text title_text;
		mutable Text body_text;

		// Loads the FadeYesNo chrome and primes the title/body Text
		// objects with sensible defaults. Call once during the parent
		// UI element's construction.
		void load();

		// Render the chrome (backdrop + icon + title/body labels) at
		// `pos`. `opacity` is applied to the backdrop and icon; text
		// is drawn at full opacity. The parent draws its buttons
		// separately via UIElement::draw.
		void draw(Point<int16_t> pos, const std::string& title,
			const std::string& body, float opacity = 1.0f) const;

		// Width of the area available for text (icon offset accounted
		// for). Used by callers that need to reflow text.
		int16_t text_area_width() const;
	};
}
