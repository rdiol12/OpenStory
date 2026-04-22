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
#include "UIAvatarMegaphone.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Graphics/Geometry.h"
#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIAvatarMegaphone::UIAvatarMegaphone() : UIDragElement<PosAVATARMEGAPHONE>(Point<int16_t>(196, 24)), focused_idx(0), whisper_enabled(false), item_slot(0), item_id(0)
	{
		nl::node main = nl::nx::ui["UIWindow.img"]["AvatarMegaphone"];
		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dim = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// BtOk / BtCancel are the Basic.img/BtOK2 / BtCancel2 buttons
		// (47x18). Position them at the bottom of the 196x193 form.
		buttons[Buttons::BT_SEND] = std::make_unique<MapleButton>(main["BtOk"],
			Point<int16_t>(bg_dim.x() - 104, bg_dim.y() - 28));
		buttons[Buttons::BT_SEND]->set_active(true);
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(main["BtCancel"],
			Point<int16_t>(bg_dim.x() - 52, bg_dim.y() - 28));
		buttons[Buttons::BT_CLOSE]->set_active(true);

		dimension = bg_dim;

		prompt_label = Text(Text::Font::A12B, Text::Alignment::LEFT,
			Color::Name::BLACK, "Avatar Megaphone", static_cast<uint16_t>(bg_dim.x() - 20));

		constexpr int16_t FIELD_LEFT = 14;
		constexpr int16_t FIELD_TOP = 70;
		constexpr int16_t FIELD_WIDTH = 168;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < 4; i++)
		{
			int16_t y = FIELD_TOP + FIELD_STEP * i;
			lines[i] = Textfield(
				Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
				Rectangle<int16_t>(
					Point<int16_t>(FIELD_LEFT, y),
					Point<int16_t>(FIELD_LEFT + FIELD_WIDTH, y + FIELD_HEIGHT)),
				60
			);
			lines[i].set_enter_callback([this](std::string) { send_broadcast(); });
		}

		focus_field(0);
	}

	void UIAvatarMegaphone::configure_item(int16_t slot, int32_t itemid)
	{
		item_slot = slot;
		item_id = itemid;
		whisper_enabled = false;
		for (auto& f : lines) f.change_text("");
		focus_field(0);
	}

	void UIAvatarMegaphone::focus_field(int idx)
	{
		for (int i = 0; i < 4; i++)
			lines[i].set_state(i == idx ? Textfield::State::FOCUSED : Textfield::State::NORMAL);
		focused_idx = idx;
	}

	void UIAvatarMegaphone::draw(float inter) const
	{
		UIElement::draw(inter);

		prompt_label.draw(position + Point<int16_t>(14, 44));

		constexpr int16_t FIELD_LEFT = 14;
		constexpr int16_t FIELD_TOP = 70;
		constexpr int16_t FIELD_WIDTH = 168;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < 4; i++)
		{
			int16_t y = FIELD_TOP + FIELD_STEP * i;
			ColorBox bg(FIELD_WIDTH, FIELD_HEIGHT, Color::Name::WHITE, 0.85f);
			bg.draw(DrawArgument(position + Point<int16_t>(FIELD_LEFT, y)));
		}

		for (const auto& f : lines)
			f.draw(position);
	}

	void UIAvatarMegaphone::update()
	{
		UIElement::update();
		for (auto& f : lines)
			f.update(position);
	}

	Cursor::State UIAvatarMegaphone::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> local = cursorpos - position;

		for (int i = 0; i < 4; i++)
		{
			Cursor::State st = lines[i].send_cursor(local, clicked);
			if (st != Cursor::State::IDLE)
			{
				if (clicked)
					focus_field(i);
				return st;
			}
		}

		return UIDragElement<PosAVATARMEGAPHONE>::send_cursor(clicked, cursorpos);
	}

	void UIAvatarMegaphone::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIAvatarMegaphone::get_type() const
	{
		return TYPE;
	}

	Button::State UIAvatarMegaphone::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_SEND:
			send_broadcast();
			break;
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		default:
			break;
		}
		return Button::State::NORMAL;
	}

	void UIAvatarMegaphone::send_broadcast()
	{
		if (item_id == 0) { deactivate(); return; }

		std::vector<std::string> payload;
		payload.reserve(4);
		for (const auto& f : lines)
			payload.push_back(f.get_text());

		while (!payload.empty() && payload.back().empty())
			payload.pop_back();

		if (payload.empty()) { deactivate(); return; }

		UseAvatarMegaphonePacket(item_slot, item_id, payload, whisper_enabled).dispatch();
		deactivate();
	}
}
