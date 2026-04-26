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
#include "UIBuddyGroup.h"

#include "UINotice.h"
#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"
#include "../../Character/BuddyList.h"
#include "../../Net/Packets/BuddyPackets.h"

#include <map>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		constexpr size_t MAX_LEN = 13;

		// Layout matched to the reference screenshot.
		// Title bar at top y=0..28 (baked into backgrnd).
		// "CREATE NEW GROUP" header at y=32..52 (the `base` strip).
		// Textfield for new group name at y=58..76.
		// "CURRENT BUDDIES" header at y=86..104.
		// Column-header row at y=106..120 ("NAME" | "GROUP").
		// Buddy list y=122..302.
		// Caption "Selected buddies…" at y=308..320.
		// BtOK / BtCancle at y=325 (NX-baked).

		constexpr int16_t FIELD_X        = 18;
		constexpr int16_t FIELD_RIGHT    = 240;
		constexpr int16_t FIELD_Y        = 60;
		constexpr int16_t FIELD_H        = 16;

		constexpr int16_t COL_HEADER_Y   = 108;
		constexpr int16_t LIST_Y_START   = 124;
		constexpr int16_t ROW_H          = 18;
		constexpr int16_t ROW_GAP        = 3;        // visual gap between rows
		constexpr int16_t ROW_STRIDE     = ROW_H + ROW_GAP;
		constexpr int16_t VISIBLE_ROWS   = 7;        // 7 rows visible, rest scrolls
		constexpr int16_t LIST_Y_END     = LIST_Y_START + VISIBLE_ROWS * ROW_STRIDE;

		constexpr int16_t CHECK_X        = 22;
		constexpr int16_t NAME_COL_X     = 36;
		constexpr int16_t GROUP_COL_X    = 130;
	}

	UIBuddyGroup::UIBuddyGroup() : UIBuddyGroup(Point<int16_t>(170, 100)) {}

	UIBuddyGroup::UIBuddyGroup(Point<int16_t> spawn)
		: UIDragElement<PosBUDDYGROUP>(Point<int16_t>(264, 28))
	{
		dimension = Point<int16_t>(264, 382);
		active    = true;
		if (position.x() == 0 && position.y() == 0)
			position = spawn;

		// `UIWindow.img/UserList/Friend/GroupMod` is the older 268×364
		// panel with the full layout (title bar, blue/light-blue header
		// strips, NAME/GROUP column header, caption) all baked into a
		// single backgrnd bitmap. Popup overlays + button bitmap come
		// from the same Friend node.
		nl::node src = nl::nx::ui["UIWindow.img"]["UserList"]["Friend"];
		nl::node mod = src["GroupMod"];

		sprites.emplace_back(mod["backgrnd"]);

		// BtOk / BtCancel have no baked origin — position them manually
		// at the bottom of the 268×364 panel, side by side.
		buttons[BT_OK]     = std::make_unique<MapleButton>(mod["BtOk"],
			Point<int16_t>(70, 335));
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(mod["BtCancel"],
			Point<int16_t>(140, 335));

		// Overlay sub-bitmaps live alongside under Friend/Popup.
		nl::node pop = src["Popup"];
		popup_addfriend     = Texture(pop["AddFriend"]);
		popup_base          = Texture(pop["Base"]);
		popup_groupdel      = Texture(pop["GroupDel"]);
		popup_groupdeldeny  = Texture(pop["GroupDelDeny"]);
		popup_groupwhisper  = Texture(pop["GroupWhisper"]);
		popup_needmessage   = Texture(pop["NeedMessage"]);

		// Per-row checkbox sprites — reuse AddFriend's CheckBox. The
		// baked origins (box: -22,-215 / check: -25,-218) pin them to
		// the bottom-left of any panel; shift() zeroes the origin so
		// the bitmaps render where we explicitly draw them.
		nl::node addfriend_node = nl::nx::ui["UIWindow2.img"]["UserList"]["AddFriend"];
		checkbox_off = Texture(addfriend_node["CheckBox"]["box"]);
		checkbox_on  = Texture(addfriend_node["CheckBox"]["check"]);
		checkbox_off.shift(Point<int16_t>(-22, -215));
		checkbox_on .shift(Point<int16_t>(-25, -218));

		buddy_dot    = Texture(nl::nx::ui["UIWindow2.img"]["UserList"]
			["Main"]["Party"]["icon0"]);

		// Alternating row backdrops (zebra stripes) so each buddy row
		// is visually separated from the next.
		nl::node sheets = nl::nx::ui["UIWindow2.img"]["UserList"]["Sheet1"];
		row_bg_even = Texture(sheets["1"]);
		row_bg_odd  = Texture(sheets["2"]);

		// v83's Group bitmap is a generic frame — render the text
		// content (header bands, column titles, caption) ourselves.
		caption          = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::BLACK,    "Selected buddies will be moved to the group.", 0);
		col_header_name  = Text(Text::Font::A11B, Text::Alignment::LEFT,   Color::Name::BLACK,    "NAME",  0);
		col_header_group = Text(Text::Font::A11B, Text::Alignment::LEFT,   Color::Name::BLACK,    "GROUP", 0);
		hint             = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::DUSTYGRAY, "",     0);
		// Clamp the row labels to their column widths so long names
		// don't bleed into the GROUP column or off the panel.
		row_label        = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::DUSTYGRAY, "",
			GROUP_COL_X - NAME_COL_X - 4);                  // ~90 px
		row_group_label  = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::DUSTYGRAY, "",
			FIELD_RIGHT - GROUP_COL_X - 4);                 // ~106 px

		new_group_field = Textfield(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::BLACK,
			Rectangle<int16_t>(FIELD_X, FIELD_RIGHT, FIELD_Y, FIELD_Y + FIELD_H),
			MAX_LEN);
		new_group_field.set_key_callback(KeyAction::Id::ESCAPE,
			[this]() { deactivate(); });
	}

	void UIBuddyGroup::rebuild_layout()
	{
		buddy_rows.clear();
		group_rows.clear();   // unused in new layout but keep field for compatibility

		const auto& entries = Stage::get().get_player()
			.get_buddylist().get_entries();

		// Clamp the scroll so we can't push past the last entry.
		int16_t total_rows = static_cast<int16_t>(entries.size());
		int16_t visible_rows = (LIST_Y_END - LIST_Y_START) / ROW_STRIDE;
		int16_t max_scroll = std::max<int16_t>(0,
			(total_rows - visible_rows) * ROW_STRIDE);
		if (buddy_scroll > max_scroll) buddy_scroll = max_scroll;
		if (buddy_scroll < 0)          buddy_scroll = 0;

		int16_t ly = LIST_Y_START - buddy_scroll;
		for (const auto& kv : entries)
		{
			BuddyRow b;
			b.cid  = kv.first;
			b.name = kv.second.name;
			b.current_group = kv.second.group.empty()
				? std::string("Default Group") : kv.second.group;
			b.hit = Rectangle<int16_t>(
				position + Point<int16_t>(FIELD_X, ly),
				position + Point<int16_t>(GROUP_COL_X - 4, ly + ROW_H));
			b.checkbox = Rectangle<int16_t>(
				position + Point<int16_t>(CHECK_X,      ly + 3),
				position + Point<int16_t>(CHECK_X + 13, ly + 14));
			b.group_cell = Rectangle<int16_t>(
				position + Point<int16_t>(GROUP_COL_X, ly),
				position + Point<int16_t>(FIELD_RIGHT, ly + ROW_H));
			buddy_rows.push_back(b);
			ly += ROW_STRIDE;
		}
	}

	void UIBuddyGroup::draw(float inter) const
	{
		UIElement::draw(inter);

		const_cast<UIBuddyGroup*>(this)->rebuild_layout();

		// All static frame content (title bar, CREATE NEW GROUP /
		// CURRENT BUDDIES header strips, NAME/GROUP column row,
		// caption) is baked into the GroupMod backgrnd bitmap — we
		// draw only the dynamic input + rows on top.

		// New-group textfield sits in the white slot under the
		// "CREATE NEW GROUP" header.
		new_group_field.draw(position);

		// Buddy list — checkbox + name + that buddy's current group.
		// Rows are 18 px tall with a 5 px gap between them. Anything
		// scrolled outside [LIST_Y_START, LIST_Y_END] is skipped so
		// nothing leaks past the panel.
		const auto& entries = Stage::get().get_player()
			.get_buddylist().get_entries();
		for (const auto& b : buddy_rows)
		{
			Point<int16_t> tl = b.hit.get_left_top();
			int16_t local_y = tl.y() - position.y();
			if (local_y + ROW_H <= LIST_Y_START || local_y >= LIST_Y_END)
				continue;

			// Hover highlight aligned with the text band only (skipping
			// the gap above each row) so the glow doesn't appear to
			// float above the group name.
			if (!hovered_group.empty() && b.current_group == hovered_group)
			{
				ColorBox glow(FIELD_RIGHT - GROUP_COL_X, ROW_H - 4,
					Color::Name::YELLOW, 0.25f);
				glow.draw(DrawArgument(
					position + Point<int16_t>(GROUP_COL_X, local_y + 6)));
			}

			// Checkbox nudged 4 px left and 3 px down so it lines up
			// with the buddy name baseline.
			checkbox_off.draw(tl + Point<int16_t>(CHECK_X - FIELD_X - 4, 10));
			if (checked_buddies.count(b.cid))
				checkbox_on .draw(tl + Point<int16_t>(CHECK_X - FIELD_X - 1, 13));

			row_label.change_color(Color::Name::DUSTYGRAY);
			row_label.change_text(b.name);
			row_label.draw(tl + Point<int16_t>(NAME_COL_X - FIELD_X, 4));

			auto it = entries.find(b.cid);
			std::string g = (it != entries.end() && !it->second.group.empty())
				? it->second.group : std::string("Default Group");
			row_group_label.change_color(Color::Name::DUSTYGRAY);
			row_group_label.change_text(g);
			row_group_label.draw(tl + Point<int16_t>(GROUP_COL_X - FIELD_X, 4));
		}

		// (Caption "Selected buddies will be moved to the group." is
		// part of the baked GroupMod backgrnd — no extra draw needed.)

		// Active overlay (if any).
		const Texture* overlay = nullptr;
		switch (popup_state)
		{
		case Popup::NEED_MESSAGE:    overlay = &popup_needmessage;  break;
		case Popup::GROUP_DEL:       overlay = &popup_groupdel;     break;
		case Popup::GROUP_DEL_DENY:  overlay = &popup_groupdeldeny; break;
		case Popup::ADD_FRIEND:      overlay = &popup_addfriend;    break;
		case Popup::GROUP_WHISPER:   overlay = &popup_groupwhisper; break;
		default: break;
		}
		if (overlay && overlay->is_valid())
			overlay->draw(position);
	}

	void UIBuddyGroup::update()
	{
		UIElement::update();
		new_group_field.update(position);
	}

	Cursor::State UIBuddyGroup::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Hover tracking for GROUP cells — track which unique group
		// name the cursor is over (used for highlight + doubleclick).
		hovered_group.clear();
		for (const auto& b : buddy_rows)
		{
			int16_t ly = b.hit.get_left_top().y() - position.y();
			if (ly + ROW_H <= LIST_Y_START || ly >= LIST_Y_END) continue;
			if (b.group_cell.contains(cursorpos))
			{
				hovered_group = b.current_group;
				break;
			}
		}

		if (clicked && popup_state == Popup::NONE)
		{
			// Toggle a buddy's checkbox (or row click anywhere outside
			// the box, in the LEFT column only).
			for (const auto& b : buddy_rows)
			{
				if (b.hit.contains(cursorpos))
				{
					if (checked_buddies.count(b.cid))
						checked_buddies.erase(b.cid);
					else
						checked_buddies.insert(b.cid);
					return Cursor::State::CLICKING;
				}
			}
		}

		if (new_group_field.get_state() == Textfield::State::NORMAL)
		{
			Cursor::State s = new_group_field.send_cursor(cursorpos, clicked);
			if (s != Cursor::State::IDLE) return s;
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIBuddyGroup::doubleclick(Point<int16_t> cursorpos)
	{
		// Double-click on a GROUP cell → confirm-and-move all checked
		// buddies into that group. Only proceeds if at least one buddy
		// is checked; otherwise the click is a no-op.
		if (popup_state != Popup::NONE) return;
		if (checked_buddies.empty()) return;

		for (const auto& b : buddy_rows)
		{
			int16_t ly = b.hit.get_left_top().y() - position.y();
			if (ly + ROW_H <= LIST_Y_START || ly >= LIST_Y_END) continue;
			if (!b.group_cell.contains(cursorpos)) continue;

			std::string target = b.current_group;
			int count = static_cast<int>(checked_buddies.size());
			std::string msg = "Move " + std::to_string(count)
				+ " buddy"
				+ (count == 1 ? std::string("") : std::string("s"))
				+ " to '" + target + "'?";
			UI::get().emplace<UIYesNo>(msg,
				[this, target](bool yes)
				{
					if (yes)
						const_cast<UIBuddyGroup*>(this)
							->apply_move_checked(target);
				});
			break;
		}
	}

	void UIBuddyGroup::send_scroll(double yoffset)
	{
		// One wheel tick = one row stride. yoffset > 0 = wheel-up =
		// scroll content earlier, so subtract from buddy_scroll.
		buddy_scroll = static_cast<int16_t>(
			buddy_scroll - yoffset * ROW_STRIDE);
		// Clamping happens in rebuild_layout each draw.
	}

	void UIBuddyGroup::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (popup_state != Popup::NONE)
			{
				popup_state = Popup::NONE;
				popup_target.clear();
			}
			else
				deactivate();
		}
	}

	UIElement::Type UIBuddyGroup::get_type() const
	{
		return TYPE;
	}

	Button::State UIBuddyGroup::button_pressed(uint16_t buttonid)
	{
		// Overlay-mode short-circuit.
		if (popup_state != Popup::NONE)
		{
			popup_state = Popup::NONE;
			popup_target.clear();
			return Button::State::NORMAL;
		}

		switch (buttonid)
		{
		case BT_OK:
		{
			std::string target = new_group_field.get_text();
			if (target.empty())
			{
				popup_state = Popup::NEED_MESSAGE;
				return Button::State::NORMAL;
			}
			if (checked_buddies.empty())
			{
				deactivate();
				break;
			}
			apply_move_checked(target);
			deactivate();
			break;
		}
		case BT_CANCEL:
			deactivate();
			break;
		}
		return Button::State::NORMAL;
	}

	void UIBuddyGroup::apply_rename(const std::string&, const std::string&) {}

	void UIBuddyGroup::apply_delete(const std::string& old_name)
	{
		const auto& entries = Stage::get().get_player().get_buddylist().get_entries();
		for (const auto& kv : entries)
		{
			if (kv.second.group != old_name) continue;
			AddBuddyPacket(kv.second.name, "Default Group").dispatch();
		}
	}

	void UIBuddyGroup::apply_move_checked(const std::string& target_group)
	{
		const auto& entries = Stage::get().get_player().get_buddylist().get_entries();
		for (int32_t cid : checked_buddies)
		{
			auto it = entries.find(cid);
			if (it == entries.end()) continue;
			AddBuddyPacket(it->second.name, target_group).dispatch();
		}
		checked_buddies.clear();
	}
}
