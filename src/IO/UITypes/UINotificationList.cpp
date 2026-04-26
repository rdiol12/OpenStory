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
#include "UINotificationList.h"

#include "UIStatusBar.h"

#include "../UI.h"
#include "../NotificationCenter.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		// Per-row backdrop is the same FadeYesNo bitmap UIAlarmInvite
		// uses, so each notification reads as a stacked copy of the
		// existing alarm banner.
		constexpr int16_t ROW_GAP = 4;
	}

	UINotificationList::UINotificationList()
		: UINotificationList(Point<int16_t>(800 - 8, 600 - 36)) {}

	UINotificationList::UINotificationList(Point<int16_t> anchor_bottom_right)
		: UIElement(Point<int16_t>(0, 0), Point<int16_t>(0, 0), true),
		  anchor(anchor_bottom_right)
	{
		title         = Text(Text::Font::A12B, Text::Alignment::LEFT,   Color::Name::WHITE,    "Notifications", 0);
		empty_label   = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DUSTYGRAY, "No new notifications.", 0);
		row_title     = Text(Text::Font::A11B, Text::Alignment::LEFT,   Color::Name::WHITE,    "", 0);
		row_body      = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::WHITE,    "", 0);
		accept_label  = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE,    "Accept", 0);
		decline_label = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE,    "Decline", 0);

		computed_height = 0;
		rebuild_layout();
	}

	void UINotificationList::rebuild_layout()
	{
		// Lazy-load the FadeYesNo backdrop so we can size by its real
		// dimensions (varies per locale).
		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];
		Texture probe(root["backgrnd"]);
		Point<int16_t> row_dims = probe.is_valid()
			? probe.get_dimensions()
			: Point<int16_t>(218, 66);

		const auto& entries = NotificationCenter::get().list();
		int16_t rows = static_cast<int16_t>(entries.size());
		int16_t body_h = (rows == 0)
			? row_dims.y()       // empty-state slot is one row tall
			: rows * row_dims.y() + (rows - 1) * ROW_GAP;

		computed_height = body_h;
		dimension = Point<int16_t>(row_dims.x(), computed_height);
		// Anchor's bottom-right matches the BT_NOTICE button's top-left
		// (so the stack grows upward from the status bar).
		position = anchor - Point<int16_t>(row_dims.x(), computed_height);
	}

	void UINotificationList::draw(float inter) const
	{
		const_cast<UINotificationList*>(this)->rebuild_layout();
		hits.clear();

		nl::node root = nl::nx::ui["UIWindow.img"]["FadeYesNo"];
		Texture row_bg(root["backgrnd"]);
		if (!row_bg.is_valid())
			row_bg = Texture(root["backgrnd1"]);

		Point<int16_t> row_dims = row_bg.is_valid()
			? row_bg.get_dimensions()
			: Point<int16_t>(218, 66);

		const auto& entries = NotificationCenter::get().list();
		if (entries.empty())
		{
			row_bg.draw(DrawArgument(position));
			empty_label.draw(position + Point<int16_t>(row_dims.x() / 2, row_dims.y() / 2 - 6));
			return;
		}

		int16_t y = 0;
		int row_index = 0;
		for (const auto& e : entries)
		{
			Point<int16_t> row_pos = position + Point<int16_t>(0, y);

			// Same backdrop UIAlarmInvite uses — one banner per pending
			// notification, stacked.
			row_bg.draw(DrawArgument(row_pos));

			// Optional icon — FadeYesNo/icon{N} maps to a small marker.
			Texture icon(root["icon0"]);
			int16_t icon_w = icon.is_valid() ? icon.get_dimensions().x() : 0;
			if (icon.is_valid())
				icon.draw(DrawArgument(row_pos + Point<int16_t>(6, 6)));

			row_title.change_text(e.title);
			row_title.draw(row_pos + Point<int16_t>(6 + icon_w + 6, 4));

			row_body.change_text(e.body);
			row_body.draw(row_pos + Point<int16_t>(6 + icon_w + 6, 22));

			// Accept / Decline pills tucked into the bottom-right corner
			// of each row (mirrors UIAlarmInvite's BtOK/BtCancel slot).
			constexpr int16_t BTN_W = 50;
			constexpr int16_t BTN_H = 18;
			Point<int16_t> accept_tl  = row_pos + Point<int16_t>(row_dims.x() - 2 * BTN_W - 12, row_dims.y() - BTN_H - 6);
			Point<int16_t> decline_tl = accept_tl + Point<int16_t>(BTN_W + 4, 0);

			ColorBox accept_bg(BTN_W, BTN_H, Color::Name::JAPANESELAUREL, 0.92f);
			accept_bg.draw(DrawArgument(accept_tl));
			ColorBox decline_bg(BTN_W, BTN_H, Color::Name::DARKRED, 0.92f);
			decline_bg.draw(DrawArgument(decline_tl));

			accept_label.draw(accept_tl + Point<int16_t>(BTN_W / 2, 2));
			decline_label.draw(decline_tl + Point<int16_t>(BTN_W / 2, 2));

			RowHits rh;
			rh.id = e.id;
			rh.accept  = Rectangle<int16_t>(accept_tl,  accept_tl  + Point<int16_t>(BTN_W, BTN_H));
			rh.decline = Rectangle<int16_t>(decline_tl, decline_tl + Point<int16_t>(BTN_W, BTN_H));
			hits.push_back(rh);

			y += row_dims.y() + ROW_GAP;
			++row_index;
		}
	}

	Cursor::State UINotificationList::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (clicked)
		{
			for (const auto& h : hits)
			{
				if (h.accept.contains(cursorpos))
				{
					NotificationCenter::get().resolve(h.id, true);
					if (NotificationCenter::get().empty())
						if (auto sb = UI::get().get_element<UIStatusBar>())
							sb->clear_notification();
					return Cursor::State::CLICKING;
				}
				if (h.decline.contains(cursorpos))
				{
					NotificationCenter::get().resolve(h.id, false);
					if (NotificationCenter::get().empty())
						if (auto sb = UI::get().get_element<UIStatusBar>())
							sb->clear_notification();
					return Cursor::State::CLICKING;
				}
			}

			Rectangle<int16_t> panel(position, position + dimension);
			if (!panel.contains(cursorpos))
			{
				deactivate();
				return Cursor::State::IDLE;
			}
		}
		else
		{
			for (const auto& h : hits)
				if (h.accept.contains(cursorpos) || h.decline.contains(cursorpos))
					return Cursor::State::CANCLICK;
		}

		return Cursor::State::IDLE;
	}

	void UINotificationList::send_key(int32_t, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UINotificationList::get_type() const
	{
		return TYPE;
	}
}
