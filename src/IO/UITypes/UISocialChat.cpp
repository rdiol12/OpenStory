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
#include "UISocialChat.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UISocialChat::UISocialChat() : UIDragElement<PosSOCIALCHAT>(Point<int16_t>(300, 24))
	{
		nl::node socialChat = nl::nx::ui["UIWindow2.img"]["socialChat"];

		backgrnd = Texture(socialChat["backgrnd"]);
		cover = Texture(socialChat["cover"]);
		close_slot = Texture(socialChat["closeSlot"]);

		// Load chat slot textures
		nl::node cSlot = socialChat["cSlot"];
		for (int i = 0; i < 6; i++)
		{
			nl::node slot = cSlot[std::to_string(i)];
			if (slot)
				c_slots[i] = Texture(slot);
		}

		// Load name tag textures
		nl::node nameTag = socialChat["nameTag"];
		for (int i = 0; i < 6; i++)
		{
			nl::node tag = nameTag[std::to_string(i)];
			if (tag)
				name_tags[i] = Texture(tag);
		}

		// Background sprites
		sprites.emplace_back(socialChat["backgrnd"]);
		sprites.emplace_back(socialChat["backgrnd2"]);
		sprites.emplace_back(socialChat["backgrnd3"]);
		sprites.emplace_back(socialChat["backgrnd4"]);

		// Main buttons
		buttons[BT_CLOSE] = std::make_unique<MapleButton>(socialChat["btX"]);
		buttons[BT_ADDFRIENDS] = std::make_unique<MapleButton>(socialChat["BtAddFriends"]);
		buttons[BT_ENTER] = std::make_unique<MapleButton>(socialChat["BtEnter"]);
		buttons[BT_IMOTICON] = std::make_unique<MapleButton>(socialChat["BtImoticon"]);
		buttons[BT_INFO] = std::make_unique<MapleButton>(socialChat["BtInfo"]);
		buttons[BT_INVITE] = std::make_unique<MapleButton>(socialChat["BtInvite"]);
		buttons[BT_MAX] = std::make_unique<MapleButton>(socialChat["BtMax"]);
		buttons[BT_MIN] = std::make_unique<MapleButton>(socialChat["BtMin"]);
		buttons[BT_MINIGAME] = std::make_unique<MapleButton>(socialChat["BtMiniGame"]);
		buttons[BT_SOCIALPOINT] = std::make_unique<MapleButton>(socialChat["BtSocialPoint"]);
		buttons[BT_WISPER] = std::make_unique<MapleButton>(socialChat["BtWisper"]);
		buttons[BT_CLAME] = std::make_unique<MapleButton>(socialChat["BtClame"]);
		buttons[BT_BIG] = std::make_unique<MapleButton>(socialChat["BtBig"]);

		dimension = backgrnd.get_dimensions();
	}

	void UISocialChat::draw(float inter) const
	{
		UIElement::draw(inter);

		cover.draw(DrawArgument(position));
	}

	void UISocialChat::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close();
	}

	UIElement::Type UISocialChat::get_type() const
	{
		return TYPE;
	}

	Button::State UISocialChat::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			close();
			break;
		case BT_MIN:
			close();
			break;
		case BT_ADDFRIENDS:
		case BT_ENTER:
		case BT_IMOTICON:
		case BT_INFO:
		case BT_INVITE:
		case BT_MAX:
		case BT_MINIGAME:
		case BT_SOCIALPOINT:
		case BT_WISPER:
		case BT_CLAME:
		case BT_BIG:
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UISocialChat::close()
	{
		deactivate();
	}
}
