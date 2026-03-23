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
#include "UIChannel.h"

#include "../KeyAction.h"
#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Audio/Audio.h"
#include "../../Configuration.h"
#include "../../Net/Packets/GameplayPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIChannel::UIChannel() : UIDragElement<PosCHANNEL>(Point<int16_t>(WIDTH, DRAG_HEIGHT))
	{
		uint8_t selected_world = Configuration::get().get_worldid();
		current_channel = Configuration::get().get_channelid();
		selected_channel = current_channel;
		channel_count = 20;

		nl::node source = nl::nx::ui["UIWindow2.img"]["Channel"];

		sprites.emplace_back(source["backgrnd"]);
		sprites.emplace_back(source["backgrnd2"]);
		sprites.emplace_back(source["backgrnd3"]);
		sprites.emplace_back(source["world"][selected_world], DrawArgument(Point<int16_t>(16, 30)));

		buttons[BT_CANCEL] = std::make_unique<MapleButton>(source["BtCancel"]);
		buttons[BT_CHANGE] = std::make_unique<MapleButton>(source["BtChange"]);

		channel_cover[true] = source["channel1"];
		channel_cover[false] = source["channel0"];

		nl::node ch_src = source["ch"];
		Point<int16_t> ch_pos = CH_SPRITE_OFFSET;

		for (uint8_t row = 0; row < ROWS; row++)
		{
			for (uint8_t col = 0; col < COLS; col++)
			{
				uint8_t n = row * COLS + col;

				if (n >= channel_count)
					break;

				ch_sprites.emplace_back(ch_src[n], DrawArgument(ch_pos));
				ch_pos.shift_x(STRIDE_HORIZ);
			}

			ch_pos.set_x(CH_SPRITE_OFFSET.x());
			ch_pos.shift_y(STRIDE_VERT);
		}

		dimension = Point<int16_t>(WIDTH, HEIGHT);
		dragarea = Point<int16_t>(WIDTH, DRAG_HEIGHT);
	}

	void UIChannel::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> base_pos = COVER_OFFSET + position;

		// Draw current channel marker
		Point<int16_t> curr_pos = base_pos + Point<int16_t>(
			(current_channel % COLS) * STRIDE_HORIZ,
			(current_channel / COLS) * STRIDE_VERT
		);
		channel_cover[false].draw(DrawArgument(curr_pos));

		// Draw selected channel marker (if different)
		if (selected_channel != current_channel)
		{
			Point<int16_t> sel_pos = base_pos + Point<int16_t>(
				(selected_channel % COLS) * STRIDE_HORIZ,
				(selected_channel / COLS) * STRIDE_VERT
			);
			channel_cover[true].draw(DrawArgument(sel_pos));
		}

		// Draw channel number sprites
		for (auto& sprite : ch_sprites)
			sprite.draw(position, inter);
	}

	void UIChannel::update()
	{
		UIElement::update();

		for (auto& sprite : ch_sprites)
			sprite.update();
	}

	int8_t UIChannel::channel_by_pos(Point<int16_t> cursorpos) const
	{
		Point<int16_t> relative = cursorpos - position - COVER_OFFSET;

		if (relative.x() < 0 || relative.y() < 0)
			return -1;

		if (relative.x() >= COLS * STRIDE_HORIZ || relative.y() >= ROWS * STRIDE_VERT)
			return -1;

		int16_t col = relative.x() / STRIDE_HORIZ;
		int16_t row = relative.y() / STRIDE_VERT;
		uint8_t ch = row * COLS + col;

		if (ch >= channel_count)
			return -1;

		return ch;
	}

	void UIChannel::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
			return;

		if (escape)
		{
			deactivate();
			return;
		}

		switch (keycode)
		{
		case KeyAction::Id::RETURN:
			change_channel();
			break;
		case KeyAction::Id::UP:
			if (selected_channel >= COLS)
				selected_channel -= COLS;
			else
				selected_channel += COLS * (ROWS - 1);

			if (selected_channel >= channel_count)
				selected_channel = current_channel;
			break;
		case KeyAction::Id::DOWN:
			selected_channel += COLS;

			if (selected_channel >= channel_count)
				selected_channel %= COLS;
			break;
		case KeyAction::Id::LEFT:
			if (selected_channel > 0)
				selected_channel--;
			else
				selected_channel = channel_count - 1;
			break;
		case KeyAction::Id::RIGHT:
			if (selected_channel < channel_count - 1)
				selected_channel++;
			else
				selected_channel = 0;
			break;
		}
	}

	Cursor::State UIChannel::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (clicked)
		{
			int8_t clicked_ch = channel_by_pos(cursorpos);

			if (clicked_ch >= 0 && clicked_ch != current_channel)
				selected_channel = clicked_ch;
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIChannel::get_type() const
	{
		return TYPE;
	}

	Button::State UIChannel::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CANCEL:
			deactivate();
			return Button::State::NORMAL;
		case BT_CHANGE:
			change_channel();
			return Button::State::NORMAL;
		default:
			return Button::State::NORMAL;
		}
	}

	void UIChannel::change_channel()
	{
		if (selected_channel != current_channel)
		{
			UI::get().disable();
			ChangeChannelPacket(selected_channel).dispatch();
		}

		deactivate();
	}
}
