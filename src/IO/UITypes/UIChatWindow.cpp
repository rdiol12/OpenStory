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
#include "UIChatWindow.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIChatWindow::UIChatWindow() : UIDragElement<PosCHATWINDOW>(Point<int16_t>(300, 24))
	{
		nl::node chatWindow = nl::nx::ui["UIWindow2.img"]["ChatWindow"];

		backgrnd = Texture(chatWindow["backgrnd"]);

		nl::node box = chatWindow["Box"];
		for (int i = 0; i <= 6; i++)
		{
			nl::node frame = box[std::to_string(i)];
			if (frame)
				box_frames.emplace_back(frame);
		}

		sprites.emplace_back(chatWindow["backgrnd"]);

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(chatWindow["BtClose"]);
		buttons[BT_CONFIRM] = std::make_unique<MapleButton>(chatWindow["BtConfirm"]);
		buttons[BT_YES] = std::make_unique<MapleButton>(chatWindow["BtYes"]);
		buttons[BT_NO] = std::make_unique<MapleButton>(chatWindow["BtNo"]);
		buttons[BT_NEXT] = std::make_unique<MapleButton>(chatWindow["BtNext"]);
		buttons[BT_BEFORE] = std::make_unique<MapleButton>(chatWindow["BtBefore"]);

		// Hide yes/no by default, show confirm
		buttons[BT_YES]->set_active(false);
		buttons[BT_NO]->set_active(false);
		buttons[BT_NEXT]->set_active(false);
		buttons[BT_BEFORE]->set_active(false);

		dimension = backgrnd.get_dimensions();
	}

	void UIChatWindow::draw(float inter) const
	{
		UIElement::draw(inter);
	}

	void UIChatWindow::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			close();
	}

	UIElement::Type UIChatWindow::get_type() const
	{
		return TYPE;
	}

	Button::State UIChatWindow::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
		case BT_CONFIRM:
		case BT_YES:
		case BT_NO:
			close();
			break;
		case BT_NEXT:
		case BT_BEFORE:
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIChatWindow::close()
	{
		deactivate();
	}
}
