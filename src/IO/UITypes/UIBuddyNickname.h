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
#include "../Components/Textfield.h"

#include "../../Graphics/Text.h"

#include <cstdint>
#include <string>

namespace ms
{
	// Memo / nickname popup. Backdrop is UIWindow2.img/UserList/Nickname
	// (252x162). Saves to BuddyMemoStore on OK.
	class UIBuddyNickname : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::BUDDYNICKNAME;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIBuddyNickname(int32_t cid, const std::string& real_name);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_OK
		};

		void commit();

		int32_t target_cid;
		std::string target_name;

		Text title;
		Text label_real;
		Text label_memo;
		Textfield nickname_field;   // small, top-right — the displayed alias
		Textfield memo_field;       // large area below — long memo text
	};
}
