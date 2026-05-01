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

namespace ms
{
	namespace
	{
		// Mirrors UIToastStack's anchor offset so toast and drawer
		// occupy the same on-screen rectangle.
		constexpr int16_t POCKET_X_NUDGE = 20;
	}

	UINotificationList::UINotificationList()
		: UINotificationList(Point<int16_t>(800 - 8, 600 - 36)) {}

	UINotificationList::UINotificationList(Point<int16_t> anchor_bottom_right)
		: UIElement(Point<int16_t>(0, 0), Point<int16_t>(0, 0), true),
		  anchor(anchor_bottom_right)
	{
		row.load();
		dimension = row.dims;

		empty_label = Text(Text::Font::A11M, Text::Alignment::CENTER,
			Color::Name::DUSTYGRAY, "No new notifications.", 0);

		buttons[Buttons::BT_ACCEPT]  = row.accept_button();
		buttons[Buttons::BT_DECLINE] = row.decline_button();

		layout();
	}

	void UINotificationList::layout()
	{
		position = anchor - Point<int16_t>(row.dims.x() - POCKET_X_NUDGE, row.dims.y());
		bool have_entry = !NotificationCenter::get().empty();
		buttons[Buttons::BT_ACCEPT]->set_active(have_entry);
		buttons[Buttons::BT_DECLINE]->set_active(have_entry);
	}

	void UINotificationList::resolve_front(bool yes)
	{
		const auto& list = NotificationCenter::get().list();
		if (list.empty())
			return;

		int32_t front_id = list.front().id;
		NotificationCenter::get().resolve(front_id, yes);

		if (NotificationCenter::get().empty())
		{
			if (auto sb = UI::get().get_element<UIStatusBar>())
				sb->clear_notification();
			deactivate();
		}
	}

	void UINotificationList::draw(float inter) const
	{
		const_cast<UINotificationList*>(this)->layout();

		const auto& entries = NotificationCenter::get().list();

		if (entries.empty())
		{
			row.draw(position, "", "");
			empty_label.draw(position + Point<int16_t>(row.dims.x() / 2,
				row.dims.y() / 2 - 6));
		}
		else
		{
			const auto& e = entries.front();
			row.draw(position, e.title, e.body);
		}

		// Renders the Accept/Decline buttons.
		UIElement::draw(inter);
	}

	Cursor::State UINotificationList::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State btn_state = UIElement::send_cursor(clicked, cursorpos);
		if (btn_state != Cursor::State::IDLE)
			return btn_state;

		if (clicked)
		{
			Rectangle<int16_t> panel(position, position + dimension);
			if (!panel.contains(cursorpos))
			{
				deactivate();
				return Cursor::State::IDLE;
			}
		}

		return Cursor::State::IDLE;
	}

	void UINotificationList::send_key(int32_t, bool pressed, bool escape)
	{
		if (!pressed) return;
		if (escape)
			deactivate();
	}

	Button::State UINotificationList::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case Buttons::BT_ACCEPT:
			resolve_front(true);
			return Button::State::NORMAL;
		case Buttons::BT_DECLINE:
			resolve_front(false);
			return Button::State::NORMAL;
		}
		return Button::State::DISABLED;
	}

	UIElement::Type UINotificationList::get_type() const
	{
		return TYPE;
	}
}
