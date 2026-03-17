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
#include "UIFarmChat.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIFarmChat::UIFarmChat() : UIDragElement<PosFARMCHAT>(Point<int16_t>(250, 24))
	{
		nl::node farmChat = nl::nx::ui["UIWindow2.img"]["farmChat"];

		backgrnd = Texture(farmChat["backgrnd"]);
		chat_cover = Texture(farmChat["chatCover"]);
		chat_default = Texture(farmChat["chatDefault"]);
		chat_gm = Texture(farmChat["chatGm"]);

		// Load chat balloon styles
		for (int i = 0; i < 3; i++)
		{
			std::string name = (i == 0) ? "chatBalloon" : "chatBalloon" + std::to_string(i + 1);
			nl::node balloon = farmChat[name];

			balloons[i].top = Texture(balloon["top"]);
			balloons[i].dotline = Texture(balloon["dotline"]);
			balloons[i].bottom = Texture(balloon["bottom"]);
		}

		sprites.emplace_back(farmChat["backgrnd"]);

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(farmChat["btClose"]);
		buttons[BT_WRITE] = std::make_unique<MapleButton>(farmChat["btWrite"]);
		buttons[BT_ADDFRIEND] = std::make_unique<MapleButton>(farmChat["btAddFriend"]);
		buttons[BT_VISITFARM] = std::make_unique<MapleButton>(farmChat["btVisitFarm"]);
		buttons[BT_NEXT] = std::make_unique<MapleButton>(farmChat["btNext"]);
		buttons[BT_PREV] = std::make_unique<MapleButton>(farmChat["btPrev"]);

		dimension = backgrnd.get_dimensions();
	}

	void UIFarmChat::draw(float inter) const
	{
		UIElement::draw(inter);
	}

	void UIFarmChat::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close();
	}

	UIElement::Type UIFarmChat::get_type() const
	{
		return TYPE;
	}

	Button::State UIFarmChat::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			close();
			break;
		case BT_WRITE:
		case BT_ADDFRIEND:
		case BT_VISITFARM:
		case BT_NEXT:
		case BT_PREV:
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIFarmChat::close()
	{
		deactivate();
	}
}
