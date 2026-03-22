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
#include "UIMessenger.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMessenger::UIMessenger() : UIDragElement<PosMESSENGER>(Point<int16_t>(200, 20))
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["Messenger"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = src["backgrnd3"];

		if (backgrnd3.size() > 0)
			sprites.emplace_back(backgrnd3);

		// Name bars for player slots
		nl::node namebar = src["NameBar"];

		for (int i = 0; i < 4; i++)
			name_bars[i] = namebar[std::to_string(i)];

		blink_tex = src["blink"];

		// Chat balloon textures
		nl::node balloon = src["chatBalloon"];

		for (int i = 0; i < 5; i++)
			chat_balloons[i] = balloon[std::to_string(i)];

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_ENTER] = std::make_unique<MapleButton>(src["BtEnter"]);

		// Initialize player slots
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			player_names[i] = "";
			player_occupied[i] = false;
		}

		name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);
		chat_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIMessenger::draw(float inter) const
	{
		UIElement::draw(inter);

		// Draw player name bars and names
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			int16_t slot_y = 30 + i * 28;
			int bar_idx = player_occupied[i] ? 1 : 0;

			name_bars[bar_idx].draw(position + Point<int16_t>(10, slot_y));

			if (player_occupied[i])
			{
				name_label.change_text(player_names[i]);
				name_label.draw(position + Point<int16_t>(20, slot_y + 4));
			}
		}

		// Draw chat lines
		int16_t chat_y = 120;
		size_t start = 0;

		if (chat_lines.size() > MAX_CHAT_LINES)
			start = chat_lines.size() - MAX_CHAT_LINES;

		for (size_t i = start; i < chat_lines.size(); i++)
		{
			const auto& line = chat_lines[i];
			std::string display = line.name + ": " + line.message;

			chat_label.change_text(display);
			chat_label.draw(position + Point<int16_t>(10, chat_y));
			chat_y += 14;
		}
	}

	void UIMessenger::update()
	{
		UIElement::update();
	}

	void UIMessenger::add_player(int8_t slot, const std::string& name)
	{
		if (slot >= 0 && slot < MAX_PLAYERS)
		{
			player_names[slot] = name;
			player_occupied[slot] = true;
		}
	}

	void UIMessenger::remove_player(int8_t slot)
	{
		if (slot >= 0 && slot < MAX_PLAYERS)
		{
			player_names[slot] = "";
			player_occupied[slot] = false;
		}
	}

	void UIMessenger::add_chat(const std::string& name, const std::string& message)
	{
		chat_lines.push_back({ name, message });

		// Limit chat history
		if (chat_lines.size() > 50)
			chat_lines.erase(chat_lines.begin());
	}

	void UIMessenger::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			MessengerLeavePacket().dispatch();
			deactivate();
		}
	}

	UIElement::Type UIMessenger::get_type() const
	{
		return TYPE;
	}

	Button::State UIMessenger::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			MessengerLeavePacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_ENTER:
			// TODO: Open invite dialog to enter a player name
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
