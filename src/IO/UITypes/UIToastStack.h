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
#include "../Components/NotificationRow.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace ms
{
	// Single floating "toast" docked above the status bar. Each entry
	// fades in, holds for ~10s, fades out, then advances to the next
	// queued entry (FIFO). Only one toast is on screen at a time —
	// additional pushes while a toast is already up wait their turn.
	// Accept/Decline use the BtOK / BtCancel sprites via NotificationRow,
	// matching the bell-button drawer. The drawer (UINotificationList)
	// keeps a parallel persistent copy of every entry until the user
	// resolves it, so toasts that auto-expire never strand the request.
	class UIToastStack : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::TOASTSTACK;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIToastStack();

		void push(std::string title, std::string body,
			std::function<void(bool yes)> resolver);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t id) override;

	private:
		enum Buttons : uint16_t
		{
			BT_ACCEPT,
			BT_DECLINE
		};

		struct Entry
		{
			std::string title_str;
			std::string body_str;
			std::function<void(bool yes)> resolver;
			int32_t age = 0;
		};

		// Pop the head of the queue and invoke its resolver — used by
		// explicit Accept/Decline button clicks.
		void resolve_front(bool yes);
		// Pop the head WITHOUT invoking the resolver — used by the
		// 10-second fade-out path. The matching drawer entry stays in
		// NotificationCenter so the user can still act on it.
		void expire_front();

		// Repositions the toast at the BT_NOTICE bell anchor.
		void place();
		// Toggle the resolution buttons based on whether the queue
		// has a front entry.
		void update_button_visibility();

		std::deque<Entry> queue;
		NotificationRow row;
	};
}
