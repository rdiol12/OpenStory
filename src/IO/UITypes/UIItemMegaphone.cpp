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
#include "UIItemMegaphone.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Graphics/Geometry.h"
#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIItemMegaphone::UIItemMegaphone() : UIDragElement<PosITEMMEGAPHONE>(Point<int16_t>(236, 24)), whisper_enabled(false), item_slot(0), item_id(0)
	{
		nl::node main = nl::nx::ui["UIWindow2.img"]["ItemMegaphone"];
		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dim = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// BtOK / BtCancle in this node carry NX origins of (-149,-153) and
		// (-188,-153) so placing them at (0,0) lands them at the correct
		// bottom-right slot inside the 236x182 backdrop.
		buttons[Buttons::BT_SEND] = std::make_unique<MapleButton>(main["BtOK"]);
		buttons[Buttons::BT_SEND]->set_active(true);
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(main["BtCancle"]);
		buttons[Buttons::BT_CLOSE]->set_active(true);

		check_icon = Texture(main["check"]);

		dimension = bg_dim;

		prompt_label = Text(Text::Font::A12B, Text::Alignment::LEFT,
			Color::Name::BLACK, "Item Megaphone", static_cast<uint16_t>(bg_dim.x() - 20));

		const int16_t FIELD_LEFT   = 16;
		const int16_t FIELD_TOP    = 60;
		const int16_t FIELD_WIDTH  = bg_dim.x() - 32; // ~204
		const int16_t FIELD_HEIGHT = 18;

		msg_field = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(FIELD_LEFT, FIELD_TOP),
				Point<int16_t>(FIELD_LEFT + FIELD_WIDTH, FIELD_TOP + FIELD_HEIGHT)),
			60
		);
		msg_field.set_enter_callback([this](std::string) { send_broadcast(); });
		msg_field.set_state(Textfield::State::FOCUSED);
	}

	void UIItemMegaphone::configure_item(int16_t slot, int32_t itemid)
	{
		item_slot = slot;
		item_id = itemid;
		whisper_enabled = false;
		msg_field.change_text("");
		msg_field.set_state(Textfield::State::FOCUSED);
	}

	void UIItemMegaphone::draw(float inter) const
	{
		UIElement::draw(inter);
		prompt_label.draw(position + Point<int16_t>(16, 14));

		// Visible white box behind the input so users can see where to click.
		ColorBox bg(dimension.x() - 32, 18, Color::Name::WHITE, 0.85f);
		bg.draw(DrawArgument(position + Point<int16_t>(16, 60)));

		msg_field.draw(position);

		// Whisper checkbox glyph (NX origin anchors it at the canonical
		// slot inside the backdrop — drawing at `position` is enough).
		if (whisper_enabled && check_icon.is_valid())
			check_icon.draw(DrawArgument(position));
	}

	void UIItemMegaphone::update()
	{
		UIElement::update();
		msg_field.update(position);
	}

	Cursor::State UIItemMegaphone::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> local = cursorpos - position;

		Cursor::State st = msg_field.send_cursor(local, clicked);
		if (st != Cursor::State::IDLE)
		{
			if (clicked && msg_field.get_state() != Textfield::State::FOCUSED)
				msg_field.set_state(Textfield::State::FOCUSED);
			return st;
		}

		// Whisper toggle hit area (rough 12x12 around the check sprite's
		// anchored position).
		Rectangle<int16_t> whisper_box(
			Point<int16_t>(10, 154), Point<int16_t>(22, 166));
		if (clicked && whisper_box.contains(local))
		{
			whisper_enabled = !whisper_enabled;
			return Cursor::State::CLICKING;
		}

		return UIDragElement<PosITEMMEGAPHONE>::send_cursor(clicked, cursorpos);
	}

	void UIItemMegaphone::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIItemMegaphone::get_type() const
	{
		return TYPE;
	}

	Button::State UIItemMegaphone::button_pressed(uint16_t buttonid)
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

	void UIItemMegaphone::send_broadcast()
	{
		if (item_id == 0) { deactivate(); return; }

		std::string msg = msg_field.get_text();
		if (msg.empty()) { deactivate(); return; }

		// Item attachment picker is not wired yet — send with no attached
		// item (src_tab = 0, src_slot = 0). TODO: add drag-drop slot from
		// the inventory so players can preview any item.
		UseMegaphonePacket(item_slot, item_id, msg, whisper_enabled, 0, 0).dispatch();
		deactivate();
	}
}
