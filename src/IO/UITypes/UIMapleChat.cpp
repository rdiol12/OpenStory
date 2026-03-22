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
#include "UIMapleChat.h"
#include "UIUserList.h"
#include "UIGuild.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMapleChat::UIMapleChat() : UIDragElement<PosMAPLECHAT1>(Point<int16_t>(200, 24))
	{
		chat_open = true;

		nl::node mapleChat = nl::nx::ui["UIWindow2.img"]["mapleChat1"];

		backgrnd = Texture(mapleChat["backgrnd"]);
		chat_area = Texture(mapleChat["chat"]);
		chat_top = Texture(mapleChat["chatTop"]);
		chat_bottom = Texture(mapleChat["chatBottom"]);

		sprites.emplace_back(mapleChat["backgrnd"]);

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(mapleChat["chatClose"]);
		buttons[BT_OPEN] = std::make_unique<MapleButton>(mapleChat["chatOpen"]);
		buttons[BT_SMALL] = std::make_unique<MapleButton>(mapleChat["btSmall"]);
		buttons[BT_FRIEND] = std::make_unique<MapleButton>(mapleChat["btFriend"]);
		buttons[BT_GUILD] = std::make_unique<MapleButton>(mapleChat["btGuild"]);

		buttons[BT_OPEN]->set_active(false);

		dimension = backgrnd.get_dimensions();
	}

	void UIMapleChat::draw(float inter) const
	{
		UIElement::draw(inter);

		if (chat_open)
		{
			chat_top.draw(DrawArgument(position));
			chat_area.draw(DrawArgument(position));
			chat_bottom.draw(DrawArgument(position));
		}
	}

	void UIMapleChat::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close();
	}

	UIElement::Type UIMapleChat::get_type() const
	{
		return TYPE;
	}

	Button::State UIMapleChat::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			toggle_chat_area();
			break;
		case BT_OPEN:
			toggle_chat_area();
			break;
		case BT_SMALL:
			close();
			break;
		case BT_FRIEND:
			UI::get().emplace<UIUserList>(UIUserList::Tab::FRIEND);
			break;
		case BT_GUILD:
			UI::get().emplace<UIGuild>();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIMapleChat::toggle_chat_area()
	{
		chat_open = !chat_open;

		buttons[BT_CLOSE]->set_active(chat_open);
		buttons[BT_OPEN]->set_active(!chat_open);
	}

	void UIMapleChat::close()
	{
		deactivate();
	}
}
