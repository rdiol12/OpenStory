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
	// Social chat window (socialChat from UIWindow2.img)
	class UISocialChat : public UIDragElement<PosSOCIALCHAT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SOCIALCHAT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UISocialChat();

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
			BT_ADDFRIENDS,
			BT_ENTER,
			BT_IMOTICON,
			BT_INFO,
			BT_INVITE,
			BT_MAX,
			BT_MIN,
			BT_MINIGAME,
			BT_SOCIALPOINT,
			BT_WISPER,
			BT_CLAME,
			BT_BIG
		};

		Texture backgrnd;
		Texture cover;
		Texture close_slot;

		// Chat slots
		Texture c_slots[6];
		Texture name_tags[6];
	};
}
