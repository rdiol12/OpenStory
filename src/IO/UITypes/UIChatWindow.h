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

#include "../UIDragElement.h"

namespace ms
{
	// System chat/message window (ChatWindow from UIWindow2.img)
	class UIChatWindow : public UIDragElement<PosCHATWINDOW>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHATWINDOW;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIChatWindow();

		void draw(float inter) const override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void close();

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_CONFIRM,
			BT_YES,
			BT_NO,
			BT_NEXT,
			BT_BEFORE
		};

		Texture backgrnd;
		std::vector<Texture> box_frames;
	};
}
