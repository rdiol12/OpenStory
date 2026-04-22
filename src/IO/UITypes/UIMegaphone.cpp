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
#include "UIMegaphone.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Graphics/Geometry.h"
#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMegaphone::UIMegaphone() : UIDragElement<PosMEGAPHONE>(Point<int16_t>(398, 24)), focused_idx(0), whisper_enabled(false), item_slot(0), item_id(0)
	{
		nl::node main = nl::nx::ui["UIWindow2.img"]["Megaphone"];
		nl::node backgrnd = main["backgrnd"];

		Point<int16_t> bg_dim = Texture(backgrnd).get_dimensions();
		sprites.emplace_back(backgrnd);

		// BtOK / BtCancle have NX origins of (-306,-78) and (-345,-78)
		// relative to the backgrnd, so passing (0,0) as the button position
		// places them at their proper pre-authored spots.
		buttons[Buttons::BT_SEND] = std::make_unique<MapleButton>(main["BtOK"]);
		buttons[Buttons::BT_SEND]->set_active(true);
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(main["BtCancle"]);
		buttons[Buttons::BT_CLOSE]->set_active(true);

		dimension = bg_dim;

		prompt_label = Text(Text::Font::A12B, Text::Alignment::LEFT,
			Color::Name::BLACK, "Megaphone", static_cast<uint16_t>(bg_dim.x() - 20));

		// Three fields so triple megaphone is ready; configure_item hides
		// the extra two when the item isn't 5077xxx.
		constexpr int16_t FIELD_LEFT = 16;
		constexpr int16_t FIELD_TOP = 32;
		constexpr int16_t FIELD_WIDTH = 280;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < 3; i++)
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

	void UIMegaphone::configure_item(int16_t slot, int32_t itemid)
	{
		item_slot = slot;
		item_id = itemid;
		whisper_enabled = false;
		for (auto& f : lines) f.change_text("");

		// Swap to the super variant for 5072 (Super Megaphone). The first
		// sprite in `sprites` is the background we installed at ctor time,
		// so replace it in-place.
		nl::node main = nl::nx::ui["UIWindow2.img"]["Megaphone"];
		nl::node bg = is_super() ? main["backgrnd_super"] : main["backgrnd"];
		if (!sprites.empty())
			sprites[0] = Sprite(bg);

		focus_field(0);
	}

	void UIMegaphone::focus_field(int idx)
	{
		int max_active = is_triple() ? 3 : 1;
		if (idx >= max_active) idx = 0;
		for (int i = 0; i < 3; i++)
			lines[i].set_state(i == idx ? Textfield::State::FOCUSED : Textfield::State::NORMAL);
		focused_idx = idx;
	}

	void UIMegaphone::draw(float inter) const
	{
		UIElement::draw(inter);

		prompt_label.draw(position + Point<int16_t>(16, 10));

		// Visible white boxes for each active input so users can see where
		// to click.
		int active = is_triple() ? 3 : 1;
		constexpr int16_t FIELD_LEFT = 16;
		constexpr int16_t FIELD_TOP = 32;
		constexpr int16_t FIELD_WIDTH = 280;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < active; i++)
		{
			int16_t y = FIELD_TOP + FIELD_STEP * i;
			ColorBox bg(FIELD_WIDTH, FIELD_HEIGHT, Color::Name::WHITE, 0.85f);
			bg.draw(DrawArgument(position + Point<int16_t>(FIELD_LEFT, y)));
		}

		for (int i = 0; i < active; i++)
			lines[i].draw(position);

		// Whisper-hear checkbox glyph pulled from NX (check bitmap has an
		// origin offset of (-16, -84) from the backgrnd, so drawing it at
		// `position` puts it at the right spot when enabled).
		if (whisper_enabled)
		{
			nl::node check = nl::nx::ui["UIWindow2.img"]["Megaphone"]["check"];
			Texture(check).draw(DrawArgument(position));
		}
	}

	void UIMegaphone::update()
	{
		UIElement::update();
		int active = is_triple() ? 3 : 1;
		for (int i = 0; i < active; i++)
			lines[i].update(position);
	}

	Cursor::State UIMegaphone::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> local = cursorpos - position;
		int active = is_triple() ? 3 : 1;

		for (int i = 0; i < active; i++)
		{
			Cursor::State st = lines[i].send_cursor(local, clicked);
			if (st != Cursor::State::IDLE)
			{
				if (clicked)
					focus_field(i);
				return st;
			}
		}

		// Whisper checkbox hit test — the check texture's origin means the
		// ACTIVE area is a ~12x12 box at position + (10, 80).
		Rectangle<int16_t> whisper_box(
			Point<int16_t>(10, 80), Point<int16_t>(22, 92));
		if (clicked && whisper_box.contains(local))
		{
			whisper_enabled = !whisper_enabled;
			return Cursor::State::CLICKING;
		}

		return UIDragElement<PosMEGAPHONE>::send_cursor(clicked, cursorpos);
	}

	void UIMegaphone::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIMegaphone::get_type() const
	{
		return TYPE;
	}

	Button::State UIMegaphone::button_pressed(uint16_t buttonid)
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

	void UIMegaphone::send_broadcast()
	{
		if (item_id == 0) { deactivate(); return; }

		if (is_triple())
		{
			std::vector<std::string> lns;
			for (int i = 0; i < 3; i++)
				lns.push_back(lines[i].get_text());
			while (!lns.empty() && lns.back().empty())
				lns.pop_back();
			if (lns.empty()) { deactivate(); return; }

			UseMegaphonePacket(item_slot, item_id, lns, whisper_enabled).dispatch();
		}
		else
		{
			std::string msg = lines[0].get_text();
			if (msg.empty()) { deactivate(); return; }

			UseMegaphonePacket(item_slot, item_id, msg, whisper_enabled).dispatch();
		}
		deactivate();
	}
}
