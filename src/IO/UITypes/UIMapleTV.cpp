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
#include "UIMapleTV.h"

#include "UIChatBar.h"
#include "UINotice.h"
#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Graphics/Geometry.h"
#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMapleTV::UIMapleTV() : UIDragElement<PosMAPLETV>(Point<int16_t>(212, 30)), focused_idx(0), item_slot(0), item_id(0), is_heart_tv(false)
	{
		nl::node main = nl::nx::ui["UIWindow.img"]["MapleTV"];
		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		// OK / Cancel near the bottom-right.
		buttons[Buttons::BT_SEND] = std::make_unique<MapleButton>(main["BtOk"],
			Point<int16_t>(bg_dimensions.x() - 100, bg_dimensions.y() - 60));
		buttons[Buttons::BT_SEND]->set_active(true);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(main["BtCancel"],
			Point<int16_t>(bg_dimensions.x() - 52, bg_dimensions.y() - 60));
		buttons[Buttons::BT_CLOSE]->set_active(true);

		// BtTo is the "pick target player" button (Heart-TV only). It's
		// created here but only activated when the item is a Heart-TV.
		buttons[Buttons::BT_TO] = std::make_unique<MapleButton>(main["BtTo"],
			Point<int16_t>(14, bg_dimensions.y() - 90));
		buttons[Buttons::BT_TO]->set_active(false);

		dimension = bg_dimensions;

		prompt_label = Text(Text::Font::A12B, Text::Alignment::LEFT,
			Color::Name::WHITE, "MapleTV Message", static_cast<uint16_t>(bg_dimensions.x() - 20));
		victim_label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, "", static_cast<uint16_t>(bg_dimensions.x() - 20));

		// Five single-line input fields stacked in the form area.
		constexpr int16_t FIELD_LEFT = 14;
		constexpr int16_t FIELD_TOP = 120;
		constexpr int16_t FIELD_WIDTH = 184;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < 5; i++)
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

		// Victim field for Heart-TV (hidden for other sub-types).
		victim_field = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(FIELD_LEFT, bg_dimensions.y() - 110),
				Point<int16_t>(FIELD_LEFT + FIELD_WIDTH, bg_dimensions.y() - 90)),
			12
		);

		focus_field(0);
	}

	void UIMapleTV::configure_item(int16_t slot, int32_t itemid)
	{
		item_slot = slot;
		item_id = itemid;
		for (auto& f : lines) f.change_text("");
		victim_field.change_text("");

		// Heart TV is 5075001 in v83 (the romantic variant that lets you
		// display a message with a chosen partner). Only then do we expose
		// the "To:" picker and victim field.
		is_heart_tv = (itemid == 5075001);
		buttons[Buttons::BT_TO]->set_active(is_heart_tv);
		victim_label.change_text(is_heart_tv ? "To:" : "");

		focus_field(0);
	}

	void UIMapleTV::focus_field(int idx)
	{
		// Textfield::set_state(FOCUSED) calls UI::focus_textfield(this)
		// internally so key input routes to the field automatically.
		for (int i = 0; i < 5; i++)
			lines[i].set_state(i == idx ? Textfield::State::FOCUSED : Textfield::State::NORMAL);
		victim_field.set_state(Textfield::State::NORMAL);
		focused_idx = idx;
	}

	void UIMapleTV::prompt_victim()
	{
		// Pop a small dialog that takes the target player name and stores
		// it in the victim field. Called by the BtTo button.
		UI::get().emplace<UIEnterText>("Target player name:",
			[this](const std::string& name)
			{
				victim_field.change_text(name);
			}, 12);
	}

	void UIMapleTV::draw(float inter) const
	{
		UIElement::draw(inter);

		prompt_label.draw(position + Point<int16_t>(14, 70));

		if (is_heart_tv)
		{
			victim_label.draw(position + Point<int16_t>(14, dimension.y() - 126));
			victim_field.draw(position);
		}

		// Draw a faint white rectangle under each input field so users can
		// see where to click even when the field is empty / unfocused. The
		// backgrnd sprite already has the TV form art but without visible
		// cutouts where the text lines go.
		constexpr int16_t FIELD_LEFT = 14;
		constexpr int16_t FIELD_TOP = 120;
		constexpr int16_t FIELD_WIDTH = 184;
		constexpr int16_t FIELD_HEIGHT = 18;
		constexpr int16_t FIELD_STEP = 22;
		for (int i = 0; i < 5; i++)
		{
			int16_t y = FIELD_TOP + FIELD_STEP * i;
			ColorBox bg(FIELD_WIDTH, FIELD_HEIGHT, Color::Name::WHITE, 0.85f);
			bg.draw(DrawArgument(position + Point<int16_t>(FIELD_LEFT, y)));
		}

		for (const auto& f : lines)
			f.draw(position);
	}

	void UIMapleTV::update()
	{
		UIElement::update();
		for (auto& f : lines)
			f.update(position);
		victim_field.update(position);
	}

	Cursor::State UIMapleTV::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> local = cursorpos - position;

		for (int i = 0; i < 5; i++)
		{
			Cursor::State st = lines[i].send_cursor(local, clicked);
			if (st != Cursor::State::IDLE)
			{
				if (clicked)
					focus_field(i);
				return st;
			}
		}

		if (is_heart_tv)
		{
			Cursor::State st = victim_field.send_cursor(local, clicked);
			if (st != Cursor::State::IDLE)
				return st;
		}

		return UIDragElement<PosMAPLETV>::send_cursor(clicked, cursorpos);
	}

	void UIMapleTV::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIMapleTV::get_type() const
	{
		return TYPE;
	}

	Button::State UIMapleTV::button_pressed(uint16_t buttonid)
	{
		if (auto chat = UI::get().get_element<UIChatBar>())
			chat->send_chatline("[TV] button_pressed id=" + std::to_string(buttonid),
				UIChatBar::LineType::YELLOW);
		switch (buttonid)
		{
		case Buttons::BT_SEND:
			send_broadcast();
			break;
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_TO:
			prompt_victim();
			break;
		default:
			break;
		}
		return Button::State::NORMAL;
	}

	void UIMapleTV::send_broadcast()
	{
		auto chat = UI::get().get_element<UIChatBar>();
		if (item_id == 0)
		{
			if (chat)
				chat->send_chatline("[TV] send aborted: item_id=0 (configure_item wasn't called?)",
					UIChatBar::LineType::RED);
			deactivate();
			return;
		}

		std::vector<std::string> payload;
		payload.reserve(5);
		for (const auto& f : lines)
			payload.push_back(f.get_text());

		while (!payload.empty() && payload.back().empty())
			payload.pop_back();

		if (payload.empty())
		{
			if (chat)
				chat->send_chatline("[TV] send aborted: all lines empty",
					UIChatBar::LineType::RED);
			deactivate();
			return;
		}

		std::string victim = is_heart_tv ? victim_field.get_text() : std::string();

		if (chat)
			chat->send_chatline("[TV] dispatching UseMapleTVPacket slot=" + std::to_string(item_slot)
				+ " id=" + std::to_string(item_id) + " lines=" + std::to_string(payload.size()),
				UIChatBar::LineType::YELLOW);

		UseMapleTVPacket(item_slot, item_id, victim, payload, false).dispatch();
		deactivate();
	}
}
