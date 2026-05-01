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
#include "UIToastStack.h"

#include "../UI.h"
#include "../../Constants.h"
#include "UIStatusBar.h"

namespace ms
{
	namespace
	{
		// Lifetime, in update ticks (~60Hz). 600 ticks ≈ 10 seconds.
		constexpr int32_t LIFETIME = 600;
		// Fade-in: short ramp so the toast appears responsive.
		constexpr int32_t FADE_IN = 12;
		// Fade-out: longer so it has visual weight as it leaves.
		constexpr int32_t FADE_OUT = 30;
		// X nudge so the toast sits just inside the BT_NOTICE button
		// instead of hanging far to its left. Mirrors UINotificationList.
		constexpr int16_t POCKET_X_NUDGE = 20;
	}

	UIToastStack::UIToastStack()
		: UIElement(Point<int16_t>(0, 0), Point<int16_t>(0, 0), true)
	{
		row.load();
		dimension = row.dims;

		buttons[Buttons::BT_ACCEPT]  = row.accept_button();
		buttons[Buttons::BT_DECLINE] = row.decline_button();
		update_button_visibility();
		place();
	}

	void UIToastStack::push(std::string title, std::string body,
		std::function<void(bool yes)> resolver)
	{
		Entry e;
		e.title_str = std::move(title);
		e.body_str  = std::move(body);
		e.resolver  = std::move(resolver);
		queue.push_back(std::move(e));
		update_button_visibility();
	}

	void UIToastStack::resolve_front(bool yes)
	{
		if (queue.empty())
			return;

		auto resolver = std::move(queue.front().resolver);
		queue.pop_front();
		if (resolver)
			resolver(yes);

		update_button_visibility();
		if (!queue.empty())
			queue.front().age = 0;
	}

	void UIToastStack::expire_front()
	{
		if (queue.empty())
			return;

		// Drop the toast WITHOUT calling its resolver — the drawer
		// entry stays in NotificationCenter so the user can still act
		// on it from the bell button. NotificationCenter::tick()
		// auto-declines if it's never resolved before TTL_TICKS.
		queue.pop_front();
		update_button_visibility();
		if (!queue.empty())
			queue.front().age = 0;
	}

	void UIToastStack::update_button_visibility()
	{
		bool have = !queue.empty();
		buttons[Buttons::BT_ACCEPT]->set_active(have);
		buttons[Buttons::BT_DECLINE]->set_active(have);
	}

	void UIToastStack::place()
	{
		Point<int16_t> anchor;
		if (auto sb = UI::get().get_element<UIStatusBar>())
		{
			anchor = sb->get_notice_anchor();
		}
		else
		{
			int16_t vw = Constants::Constants::get().get_viewwidth();
			int16_t vh = Constants::Constants::get().get_viewheight();
			anchor = Point<int16_t>(vw - 8, vh - 36);
		}
		position = anchor - Point<int16_t>(row.dims.x() - POCKET_X_NUDGE, row.dims.y());
	}

	void UIToastStack::update()
	{
		UIElement::update();

		if (queue.empty())
			return;

		queue.front().age++;
		if (queue.front().age >= LIFETIME)
			expire_front();
	}

	void UIToastStack::draw(float inter) const
	{
		if (queue.empty())
			return;

		const_cast<UIToastStack*>(this)->place();

		const Entry& e = queue.front();

		// Linear fade in / hold / fade out.
		float opacity = 1.0f;
		if (e.age < FADE_IN)
			opacity = static_cast<float>(e.age) / FADE_IN;
		else if (e.age > LIFETIME - FADE_OUT)
			opacity = static_cast<float>(LIFETIME - e.age) / FADE_OUT;

		row.draw(position, e.title_str, e.body_str, opacity);
		UIElement::draw(opacity);
	}

	Cursor::State UIToastStack::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		if (queue.empty())
			return Cursor::State::IDLE;

		Cursor::State btn_state = UIElement::send_cursor(clicked, cursorpos);
		if (btn_state != Cursor::State::IDLE)
			return btn_state;

		return Cursor::State::IDLE;
	}

	Button::State UIToastStack::button_pressed(uint16_t id)
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

	UIElement::Type UIToastStack::get_type() const
	{
		return TYPE;
	}
}
