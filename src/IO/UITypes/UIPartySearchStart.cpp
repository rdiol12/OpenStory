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
#include "UIPartySearchStart.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Net/Packets/SocialPackets.h"
#include "../../Gameplay/Stage.h"

#include <algorithm>
#include <cstdlib>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		// Layout inside the 260×103 PopupSettings backdrop.
		constexpr int16_t MIN_FIELD_X = 30;
		constexpr int16_t MIN_FIELD_R = 110;
		constexpr int16_t MIN_FIELD_Y = 38;
		constexpr int16_t MIN_FIELD_H = 16;

		constexpr int16_t MAX_FIELD_X = 145;
		constexpr int16_t MAX_FIELD_R = 230;
		constexpr int16_t MAX_FIELD_Y = 38;
		constexpr int16_t MAX_FIELD_H = 16;

		// Bits 0..25 cover every job category Cosmic recognises in
		// PartySearchTask. Sending the full mask says "any class".
		constexpr int32_t ALL_JOBS_MASK = 0x3FFFFFF;
	}

	UIPartySearchStart::UIPartySearchStart()
		: UIDragElement<PosPARTYSETTINGS>(Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::DRAGAREA_H))
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"]["PopupSettings"];

		sprites.emplace_back(src["backgrnd"]);
		chrome.load(src);

		buttons[BT_OK]     = std::make_unique<MapleButton>(src["BtSave"]);
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		// Pre-fill with the ±10 window around the local player's
		// level so the leader can hit Start immediately.
		int16_t my_level =
			static_cast<int16_t>(Stage::get().get_player().get_level());
		int16_t default_min = std::max<int16_t>(1, my_level - 10);
		int16_t default_max = std::min<int16_t>(200, my_level + 10);

		min_field = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(MIN_FIELD_X, MIN_FIELD_Y),
				Point<int16_t>(MIN_FIELD_R, MIN_FIELD_Y + MIN_FIELD_H)),
			3);
		min_field.change_text(std::to_string(default_min));

		max_field = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(MAX_FIELD_X, MAX_FIELD_Y),
				Point<int16_t>(MAX_FIELD_R, MAX_FIELD_Y + MAX_FIELD_H)),
			3);
		max_field.change_text(std::to_string(default_max));

		min_label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, "Min:", 0);
		max_label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, "Max:", 0);
		hint_label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::DUSTYGRAY,
			"Range max-min must be 30 or fewer levels.", 0);

		dimension = Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::HEIGHT);
		dragarea  = Point<int16_t>(PopupSettingsChrome::WIDTH, PopupSettingsChrome::DRAGAREA_H);
	}

	void UIPartySearchStart::draw(float inter) const
	{
		UIElement::draw(inter);

		// Recruiting / "make" title strip — same chrome the Settings
		// dialog uses, just locked to the make-mode swap.
		chrome.draw_title(position, /*make_mode=*/true);

		min_label.draw(position + Point<int16_t>(8, MIN_FIELD_Y));
		max_label.draw(position + Point<int16_t>(120, MAX_FIELD_Y));

		min_field.draw(position, Point<int16_t>(2, -2));
		max_field.draw(position, Point<int16_t>(2, -2));

		hint_label.draw(position + Point<int16_t>(8, MIN_FIELD_Y + MIN_FIELD_H + 6));
	}

	void UIPartySearchStart::update()
	{
		UIElement::update();
		min_field.update(position);
		max_field.update(position);
	}

	Cursor::State UIPartySearchStart::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State ms = min_field.send_cursor(cursorpos, clicked);
		if (ms != Cursor::State::IDLE) return ms;
		Cursor::State xs = max_field.send_cursor(cursorpos, clicked);
		if (xs != Cursor::State::IDLE) return xs;

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIPartySearchStart::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed) return;
		if (escape)
		{
			deactivate();
			return;
		}
		if (keycode == 13 /* enter */)
			dispatch_start();
	}

	UIElement::Type UIPartySearchStart::get_type() const
	{
		return TYPE;
	}

	Button::State UIPartySearchStart::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_OK:     dispatch_start(); break;
		case BT_CANCEL: deactivate();     break;
		}
		return Button::State::NORMAL;
	}

	void UIPartySearchStart::dispatch_start()
	{
		std::string min_str = min_field.get_text();
		std::string max_str = max_field.get_text();
		if (min_str.empty() || max_str.empty()) return;

		int min_lvl = std::atoi(min_str.c_str());
		int max_lvl = std::atoi(max_str.c_str());
		if (min_lvl <= 0 || max_lvl <= 0 || max_lvl < min_lvl) return;

		// Cosmic enforces max-min ≤ 30 server-side; sanitise client
		// side so we don't get an "error popup" round-trip.
		if (max_lvl - min_lvl > 30)
			max_lvl = min_lvl + 30;

		PartySearchStartPacket(min_lvl, max_lvl, 6, ALL_JOBS_MASK).dispatch();
		deactivate();
	}
}
