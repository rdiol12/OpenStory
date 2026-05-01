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
#include "UIBuddyNickname.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Character/BuddyMemoStore.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		constexpr size_t MAX_NICK_LEN    = 13;
		constexpr size_t MAX_CONTENT_LEN = 80;

		// Layout in the 252×162 Nickname popup.
		// One row per field — the placeholder text and typed text both
		// render at the same y inside the textfield, so click→edit
		// flows in place with no jumps.
		constexpr int16_t NICK_FIELD_X   = 20;
		constexpr int16_t NICK_FIELD_Y   = 26;       // moved up
		constexpr int16_t NICK_FIELD_R   = 230;
		constexpr int16_t NICK_FIELD_H   = 14;

		constexpr int16_t MEMO_FIELD_X   = 20;
		constexpr int16_t MEMO_FIELD_R   = 230;
		constexpr int16_t MEMO_FIELD_Y   = 56;
		constexpr int16_t MEMO_FIELD_H   = 30;
	}

	UIBuddyNickname::UIBuddyNickname(int32_t cid, const std::string& real_name)
		: UIElement(Point<int16_t>(280, 200), Point<int16_t>(252, 162), true),
		  target_cid(cid),
		  target_name(real_name)
	{
		nl::node nick = nl::nx::ui["UIWindow2.img"]["UserList"]["Nickname"];

		sprites.emplace_back(nick["backgrnd"]);
		sprites.emplace_back(nick["backgrnd2"]);

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(nick["BtClose"]);
		buttons[BT_OK]    = std::make_unique<MapleButton>(nick["BtOK"]);

		title      = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE,    "",                   0);
		// Placeholder shown inside the empty nickname field — the
		// buddy's real name. Click to overwrite with a custom alias.
		// Drawn bold + BLACK so it reads strongly, not as faded.
		label_real = Text(Text::Font::A11B, Text::Alignment::LEFT,   Color::Name::BLACK,    real_name,            0);
		// Placeholder shown inside the empty memo box.
		label_memo = Text(Text::Font::A11B, Text::Alignment::LEFT,   Color::Name::BLACK,    "Click to add a memo…", 0);

		// Pre-fill from the store so editing persists across opens.
		const std::string& existing_nick    = BuddyMemoStore::get().get_memo(cid);
		const std::string& existing_content = BuddyMemoStore::get().get_content(cid);

		nickname_field = Textfield(
			Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(NICK_FIELD_X, NICK_FIELD_R,
				NICK_FIELD_Y, NICK_FIELD_Y + NICK_FIELD_H),
			MAX_NICK_LEN);
		if (!existing_nick.empty())
			nickname_field.change_text(existing_nick);

		memo_field = Textfield(
			Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(MEMO_FIELD_X, MEMO_FIELD_R,
				MEMO_FIELD_Y, MEMO_FIELD_Y + MEMO_FIELD_H),
			MAX_CONTENT_LEN);
		if (!existing_content.empty())
			memo_field.change_text(existing_content);

		// Tab-ish flow: Enter on nickname jumps to memo; Enter on memo
		// commits both fields.
		nickname_field.set_enter_callback([this](std::string)
			{
				nickname_field.set_state(Textfield::State::NORMAL);
				memo_field.set_state(Textfield::State::FOCUSED);
			});
		memo_field.set_enter_callback([this](std::string) { commit(); });

		auto esc = [this]() { deactivate(); };
		nickname_field.set_key_callback(KeyAction::Id::ESCAPE, esc);
		memo_field    .set_key_callback(KeyAction::Id::ESCAPE, esc);

		nickname_field.set_state(Textfield::State::FOCUSED);
	}

	void UIBuddyNickname::draw(float inter) const
	{
		UIElement::draw(inter);

		// Visible frame around the memo box so it reads as a textarea.
		ColorBox memo_frame(MEMO_FIELD_R - MEMO_FIELD_X, MEMO_FIELD_H + 4,
			Color::Name::WHITE, 0.4f);
		memo_frame.draw(DrawArgument(
			position + Point<int16_t>(MEMO_FIELD_X, MEMO_FIELD_Y - 2)));

		// Placeholder text — only visible when the field is empty AND
		// not focused. Click vanishes the placeholder and the caret +
		// typed text take its place at the same y.
		if (nickname_field.get_state() != Textfield::State::FOCUSED
			&& nickname_field.get_text().empty())
			label_real.draw(position + Point<int16_t>(NICK_FIELD_X, NICK_FIELD_Y));
		if (memo_field.get_state() != Textfield::State::FOCUSED
			&& memo_field.get_text().empty())
			label_memo.draw(position + Point<int16_t>(MEMO_FIELD_X, MEMO_FIELD_Y));

		nickname_field.draw(position);
		memo_field.draw(position);
	}

	void UIBuddyNickname::update()
	{
		UIElement::update();
		nickname_field.update(position);
		memo_field.update(position);
	}

	Cursor::State UIBuddyNickname::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Closest-y field routing inside the popup. Click inside the
		// nickname row band → focus nickname; below it → focus memo.
		if (clicked)
		{
			int16_t local_y = cursorpos.y() - position.y();
			int16_t local_x = cursorpos.x() - position.x();

			auto focus_only = [&](Textfield& f) {
				f.set_state(Textfield::State::FOCUSED);
				if (&f != &nickname_field) nickname_field.set_state(Textfield::State::NORMAL);
				if (&f != &memo_field)     memo_field.set_state(Textfield::State::NORMAL);
			};

			// Nickname row.
			if (local_x >= NICK_FIELD_X && local_x <= NICK_FIELD_R
				&& local_y >= NICK_FIELD_Y - 6 && local_y < MEMO_FIELD_Y - 2)
			{
				focus_only(nickname_field);
				return Cursor::State::CLICKING;
			}

			// Memo box (the entire green-ish frame).
			if (local_x >= MEMO_FIELD_X && local_x <= MEMO_FIELD_R
				&& local_y >= MEMO_FIELD_Y - 2 && local_y < MEMO_FIELD_Y + MEMO_FIELD_H + 4)
			{
				focus_only(memo_field);
				return Cursor::State::CLICKING;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIBuddyNickname::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed) return;
		if (escape)
		{
			deactivate();
			return;
		}
	}

	UIElement::Type UIBuddyNickname::get_type() const
	{
		return TYPE;
	}

	Button::State UIBuddyNickname::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_OK:    commit();     break;
		case BT_CLOSE: deactivate(); break;
		}
		return Button::State::NORMAL;
	}

	void UIBuddyNickname::commit()
	{
		BuddyMemoStore::get().set_both(target_cid,
			nickname_field.get_text(),
			memo_field.get_text());
		deactivate();
	}
}
