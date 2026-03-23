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
	class UIGameSettings : public UIDragElement<PosGAMESETTINGS>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::GAMESETTINGS;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIGameSettings();

		void draw(float inter) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CANCEL,
			BT_OK
		};

		enum Checks : uint16_t
		{
			WHISPERS,
			FRIEND_INVITES,
			CHAT_INVITES,
			TRADE_REQUESTS,
			PARTY_INVITES,
			SIDEKICK_INVITES,
			EXPEDITION_INVITES,
			GUILD_CHAT,
			GUILD_INVITES,
			ALLIANCE_CHAT,
			ALLIANCE_INVITES,
			FAMILY_INVITES,
			FOLLOW,
			NUM_CHECKS
		};

		static constexpr int16_t WIDTH = 119;
		static constexpr int16_t HEIGHT = 287;
		static constexpr int16_t STRIDE_VERT = 18;
		static constexpr int16_t DRAG_HEIGHT = 17;
		static constexpr int16_t CHECK_SIDE_LEN = 6;

		Texture check_texture;
		uint16_t checks_state;
	};
}
