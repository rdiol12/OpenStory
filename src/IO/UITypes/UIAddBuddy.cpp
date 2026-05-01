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
#include "UIAddBuddy.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"
#include "../../Character/BuddyList.h"
#include "../../Character/BuddyMemoStore.h"
#include "../../Net/Packets/BuddyPackets.h"

#include <set>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		constexpr size_t MAX_NAME_LEN    = 13;
		constexpr size_t MAX_NICK_LEN    = 13;
		constexpr size_t MAX_CONTENT_LEN = 80;

		// Layout offsets within the 260×247 AddFriend backdrop.
		constexpr int16_t FIELD_X        = 22;
		constexpr int16_t FIELD_RIGHT    = 234;
		constexpr int16_t FIELD_H        = 14;

		// Original (working) layout — keep these visual positions.
		// Click→field mapping is handled by section hit zones in
		// send_cursor instead of relying on Textfield bounds, since
		// prompt labels and the green frame visually overlap previous
		// fields in this layout.
		constexpr int16_t RADIO_Y        = 56;
		constexpr int16_t RADIO_BOX_X    = 16;

		// One row = one y. The textfield's bounds sit at the prompt y
		// so the prompt placeholder, the typed text, and the caret all
		// render at the same line — no "below the field" gap.
		constexpr int16_t PROMPT_NAME_Y  = 72;
		constexpr int16_t NAME_FIELD_Y   = 72;

		constexpr int16_t PROMPT_NICK_Y  = 96;
		constexpr int16_t NICK_FIELD_Y   = 96;

		// Memo caret/indicator moved up further; green frame moves
		// with it so caret stays at its top edge.
		constexpr int16_t PROMPT_MEMO_Y  = 122;
		constexpr int16_t MEMO_FIELD_Y   = 122;
		constexpr int16_t MEMO_AREA_H        = 72;
		constexpr int16_t MEMO_FRAME_TOP_OFF = 2;

		constexpr int16_t DROPDOWN_W     = FIELD_RIGHT - FIELD_X;
		constexpr int16_t DROPDOWN_ROW_H = 16;
		constexpr int16_t DROPDOWN_MAX_ROWS = 6;
	}

	UIAddBuddy::UIAddBuddy() : UIAddBuddy("", "Default Group", Point<int16_t>(270, 180)) {}
	UIAddBuddy::UIAddBuddy(Point<int16_t> spawn) : UIAddBuddy("", "Default Group", spawn) {}
	UIAddBuddy::UIAddBuddy(const std::string& prefill_name, const std::string& prefill_group)
		: UIAddBuddy(prefill_name, prefill_group, Point<int16_t>(270, 180)) {}

	UIAddBuddy::UIAddBuddy(const std::string& prefill_name, const std::string& prefill_group, Point<int16_t> spawn)
		: UIDragElement<PosADDBUDDY>(Point<int16_t>(260, 22)),
		  selected_group(prefill_group.empty() ? std::string("Default Group") : prefill_group)
	{
		dimension = Point<int16_t>(260, 247);
		active    = true;
		if (position.x() == 0 && position.y() == 0)
			position = spawn;

		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"]["AddFriend"];

		sprites.emplace_back(src["backgrnd"]);
		sprites.emplace_back(src["bar"]);

		radio_box   = Texture(src["CheckBox"]["box"]);
		radio_check = Texture(src["CheckBox"]["check"]);

		buttons[BT_OK]     = std::make_unique<MapleButton>(src["BtOK"]);
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		prompt_name      = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Please enter a character name.", 0);
		prompt_nick      = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Please enter a nickname.",      0);
		label_memo       = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Memo (optional)",                0);
		typing_indicator = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLUE,  "> typing...",                    0);
		radio_label   = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "",                                0);
		dropdown_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "", 0);

		auto build_field = [&](const std::string& initial, int16_t y, size_t cap) {
			Textfield f(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
				Rectangle<int16_t>(FIELD_X, FIELD_RIGHT, y, y + FIELD_H), cap);
			if (!initial.empty()) f.change_text(initial);
			return f;
		};

		name_field = build_field(prefill_name, NAME_FIELD_Y, MAX_NAME_LEN);
		nick_field = build_field("",            NICK_FIELD_Y, MAX_NICK_LEN);
		memo_field = build_field("",            MEMO_FIELD_Y, MAX_CONTENT_LEN);

		name_field.set_enter_callback([this](std::string)
			{
				name_field.set_state(Textfield::State::NORMAL);
				nick_field.set_state(Textfield::State::FOCUSED);
			});
		nick_field.set_enter_callback([this](std::string)
			{
				nick_field.set_state(Textfield::State::NORMAL);
				memo_field.set_state(Textfield::State::FOCUSED);
			});
		memo_field.set_enter_callback([this](std::string) { commit(); });

		auto esc = [this]() { deactivate(); };
		name_field.set_key_callback(KeyAction::Id::ESCAPE, esc);
		nick_field.set_key_callback(KeyAction::Id::ESCAPE, esc);
		memo_field.set_key_callback(KeyAction::Id::ESCAPE, esc);

		if (prefill_name.empty())
			name_field.set_state(Textfield::State::FOCUSED);
		else
			nick_field.set_state(Textfield::State::FOCUSED);
	}

	void UIAddBuddy::rebuild_groups()
	{
		dropdown_rows.clear();

		std::set<std::string> names;
		names.insert("Default Group");

		const auto& entries = Stage::get().get_player()
			.get_buddylist().get_entries();
		for (const auto& kv : entries)
			if (!kv.second.group.empty())
				names.insert(kv.second.group);

		// Each row in the dropdown anchors below the radio toggle.
		int16_t y = RADIO_Y + DROPDOWN_ROW_H + 2;
		int rendered = 0;
		for (const auto& n : names)
		{
			DropdownRow r;
			r.name = n;
			r.hit = Rectangle<int16_t>(
				position + Point<int16_t>(FIELD_X, y),
				position + Point<int16_t>(FIELD_X + DROPDOWN_W, y + DROPDOWN_ROW_H));
			dropdown_rows.push_back(r);
			y += DROPDOWN_ROW_H;
			if (++rendered >= DROPDOWN_MAX_ROWS) break;
		}
	}

	void UIAddBuddy::draw(float inter) const
	{
		UIElement::draw(inter);

		const_cast<UIAddBuddy*>(this)->rebuild_groups();

		// Each row has the textfield's bounds aligned with the prompt's
		// y, so the prompt acts as placeholder text inside the field.
		// Show the prompt only when the field is empty AND not focused;
		// otherwise the textfield itself renders the typed content +
		// caret at the same line.
		auto draw_prompt = [&](const Text& prompt, Point<int16_t> tl, const Textfield& f) {
			if (f.get_state() != Textfield::State::FOCUSED && f.get_text().empty())
				prompt.draw(tl);
		};

		draw_prompt(prompt_name, position + Point<int16_t>(FIELD_X, PROMPT_NAME_Y), name_field);
		draw_prompt(prompt_nick, position + Point<int16_t>(FIELD_X, PROMPT_NICK_Y), nick_field);
		draw_prompt(label_memo,  position + Point<int16_t>(FIELD_X, PROMPT_MEMO_Y), memo_field);

		// Nudge the caret marker a few px to the right so it sits
		// inside the input row rather than flush against the left edge.
		Point<int16_t> caret_nudge(4, 0);
		name_field.draw(position, caret_nudge);
		nick_field.draw(position, caret_nudge);
		memo_field.draw(position, caret_nudge);

		// Group label — just the group name (no "Group:" prefix), with a
		// short hint after it when the dropdown is collapsed. The
		// checkbox indicator that used to sit to the left of this label
		// has been dropped per request — the label itself is the click
		// target.
		radio_label.change_text(selected_group +
			(dropdown_open ? "  (click to close)" : "  (click to choose)"));
		radio_label.draw(position + Point<int16_t>(RADIO_BOX_X + 6, RADIO_Y - 2));

		radio_hit = Rectangle<int16_t>(
			position + Point<int16_t>(RADIO_BOX_X, RADIO_Y),
			position + Point<int16_t>(FIELD_X + DROPDOWN_W, RADIO_Y + DROPDOWN_ROW_H));

		// Dropdown panel — only drawn when expanded. Overlays the
		// BtOK/BtCancel area below; click outside to close.
		// Semantics: the row currently chosen as the group (where the
		// buddy will appear in the friend list) gets a WHITE background
		// and a filled radio indicator. All other rows show the empty
		// radio box so the user sees this is a one-of-N picker.
		if (dropdown_open && !dropdown_rows.empty())
		{
			int16_t panel_h = static_cast<int16_t>(dropdown_rows.size()) * DROPDOWN_ROW_H + 2;
			ColorBox dropdown_bg(DROPDOWN_W, panel_h, Color::Name::JAMBALAYA, 0.94f);
			dropdown_bg.draw(DrawArgument(
				position + Point<int16_t>(FIELD_X, RADIO_Y + DROPDOWN_ROW_H + 1)));

			for (const auto& r : dropdown_rows)
			{
				bool selected = (r.name == selected_group);
				Point<int16_t> tl = r.hit.get_left_top();

				if (selected)
				{
					// White row highlight under the picked group.
					ColorBox sel_bg(DROPDOWN_W, DROPDOWN_ROW_H,
						Color::Name::WHITE, 0.95f);
					sel_bg.draw(DrawArgument(tl));
				}

				// Empty radio box on every row; fill it with the check
				// overlay only on the selected row.
				radio_box.draw(tl + Point<int16_t>(4, 3));
				if (selected)
					radio_check.draw(tl + Point<int16_t>(7, 6));

				dropdown_label.change_color(selected
					? Color::Name::BLACK : Color::Name::WHITE);
				dropdown_label.change_text(r.name);
				dropdown_label.draw(tl + Point<int16_t>(22, -1));
			}
		}
	}

	void UIAddBuddy::update()
	{
		UIElement::update();
		name_field.update(position);
		nick_field.update(position);
		memo_field.update(position);
	}

	Cursor::State UIAddBuddy::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Dropdown handling first: if open, eat all clicks.
		if (dropdown_open)
		{
			if (clicked)
			{
				for (const auto& r : dropdown_rows)
				{
					if (r.hit.contains(cursorpos))
					{
						selected_group = r.name;
						dropdown_open = false;
						return Cursor::State::CLICKING;
					}
				}
				if (radio_hit.contains(cursorpos))
				{
					dropdown_open = false;
					return Cursor::State::CLICKING;
				}
				// Click outside the dropdown closes it without changing
				// the selection (keeps current pick).
				dropdown_open = false;
			}
			return Cursor::State::CANCLICK;
		}

		// Radio toggle click → open dropdown.
		if (clicked && radio_hit.contains(cursorpos))
		{
			dropdown_open = true;
			return Cursor::State::CLICKING;
		}

		// Closest-field hit detection. Compute the user's click y
		// distance to each field's vertical center, then focus the
		// nearest one. Click x must still be inside the form column.
		if (clicked)
		{
			int16_t local_x = cursorpos.x() - position.x();
			int16_t local_y = cursorpos.y() - position.y();
			int16_t memo_box_bot = MEMO_FIELD_Y - MEMO_FRAME_TOP_OFF + MEMO_AREA_H;

			auto focus_only = [&](Textfield& f) {
				f.set_state(Textfield::State::FOCUSED);
				if (&f != &name_field) name_field.set_state(Textfield::State::NORMAL);
				if (&f != &nick_field) nick_field.set_state(Textfield::State::NORMAL);
				if (&f != &memo_field) memo_field.set_state(Textfield::State::NORMAL);
			};

			if (local_x >= FIELD_X && local_x <= FIELD_RIGHT
				&& local_y >= PROMPT_NAME_Y - 6 && local_y <= memo_box_bot)
			{
				// Each field's "center" is the midpoint between its
				// prompt label and its input strip — the visual middle
				// of the row from the user's perspective.
				int16_t name_c = (PROMPT_NAME_Y + NAME_FIELD_Y) / 2;
				int16_t nick_c = (PROMPT_NICK_Y + NICK_FIELD_Y) / 2;
				int16_t memo_c = (PROMPT_MEMO_Y + MEMO_FIELD_Y) / 2;

				int16_t dn = std::abs(local_y - name_c);
				int16_t di = std::abs(local_y - nick_c);
				int16_t dm = std::abs(local_y - memo_c);

				if (dn <= di && dn <= dm)
					focus_only(name_field);
				else if (di <= dm)
					focus_only(nick_field);
				else
					focus_only(memo_field);

				return Cursor::State::CLICKING;
			}
		}

		// Hover/non-click cursor still goes to whichever textfield is
		// active so its caret tracking works.
		if (name_field.get_state() == Textfield::State::FOCUSED)
			name_field.send_cursor(cursorpos, clicked);
		else if (nick_field.get_state() == Textfield::State::FOCUSED)
			nick_field.send_cursor(cursorpos, clicked);
		else if (memo_field.get_state() == Textfield::State::FOCUSED)
			memo_field.send_cursor(cursorpos, clicked);

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIAddBuddy::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (dropdown_open)
				dropdown_open = false;
			else
				deactivate();
		}
	}

	UIElement::Type UIAddBuddy::get_type() const
	{
		return TYPE;
	}

	Button::State UIAddBuddy::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_OK:     commit();    break;
		case BT_CANCEL: deactivate(); break;
		}
		return Button::State::NORMAL;
	}

	void UIAddBuddy::commit()
	{
		std::string nm = name_field.get_text();
		std::string gp = selected_group.empty() ? std::string("Default Group") : selected_group;
		if (nm.empty()) return;

		AddBuddyPacket(nm, gp).dispatch();

		std::string nick = nick_field.get_text();
		std::string memo = memo_field.get_text();
		if (!nick.empty() || !memo.empty())
		{
			const auto& entries = Stage::get().get_player()
				.get_buddylist().get_entries();
			for (const auto& kv : entries)
			{
				if (kv.second.name == nm)
				{
					BuddyMemoStore::get().set_both(kv.first, nick, memo);
					break;
				}
			}
		}

		deactivate();
	}
}
