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

#include "../UIElement.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Geometry.h"

#include <cstdint>
#include <vector>

namespace ms
{
	// Drawer popup that opens above BT_NOTICE on the status bar. Each
	// row corresponds to a NotificationCenter entry; row width is fixed,
	// height grows with the queue. Click Accept/Decline to resolve the
	// entry; click outside the panel to close.
	class UINotificationList : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::NOTIFICATIONLIST;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		// `anchor_bottom_right` is a screen-space point — the popup
		// positions itself so that its bottom-right corner sits there
		// (i.e. just above the BT_NOTICE button on the status bar).
		UINotificationList();
		UINotificationList(Point<int16_t> anchor_bottom_right);

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	private:
		struct RowHits
		{
			int32_t id;
			Rectangle<int16_t> accept;
			Rectangle<int16_t> decline;
		};

		void rebuild_layout();

		mutable Text title;
		mutable Text empty_label;
		mutable Text row_title;
		mutable Text row_body;
		mutable Text accept_label;
		mutable Text decline_label;

		mutable std::vector<RowHits> hits;

		// Computed each draw from the current NotificationCenter entry
		// count so the panel grows / shrinks as items resolve.
		mutable int16_t computed_height;

		Point<int16_t> anchor;
	};
}
