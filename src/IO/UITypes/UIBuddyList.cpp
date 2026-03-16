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
#include "UIBuddyList.h"

#include <algorithm>

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UINotice.h"

#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/BuddyPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIBuddyList::UIBuddyList() : UIDragElement<PosBUDDYLIST>(Point<int16_t>(260, 20)), selected_buddy(-1), scroll_offset(0)
	{
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node UserList = nl::nx::ui["UIWindow2.img"]["UserList"];
		nl::node Main = UserList["Main"];
		nl::node Friend = Main["Friend"];

		// Background sprite
		nl::node backgrnd = Main["backgrnd"];
		sprites.emplace_back(backgrnd);
		sprites.emplace_back(Main["backgrnd2"]);

		// Title area from Friend node
		sprites.emplace_back(Friend["title"]);

		Point<int16_t> backgrnd_dim = Texture(backgrnd).get_dimensions();

		// Close button positioned at top-right of the window
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(backgrnd_dim.x() - 18, 7));

		// Friend action buttons
		buttons[Buttons::BT_ADD] = std::make_unique<MapleButton>(Friend["BtAddFriend"]);
		buttons[Buttons::BT_DELETE] = std::make_unique<MapleButton>(Friend["BtAddGroup"]);

		// Load row backgrounds (friend0 through friend6)
		nl::node FriendNode = UserList["Friend"];
		for (size_t i = 0; i < 7; i++)
		{
			std::string row_name = "friend" + std::to_string(i);
			row_textures[i] = FriendNode[row_name];
		}

		// Load status icons (icon0 through icon5)
		for (size_t i = 0; i < 6; i++)
		{
			std::string icon_name = "icon" + std::to_string(i);
			status_icons[i] = FriendNode[icon_name];
		}

		// Load separator line and button background
		line = FriendNode["line"];
		button_bg = FriendNode["buttonbg"];

		// Initialize text labels for drawing buddy rows
		name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		status_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
		group_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);
		title_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Friends List");

		dimension = backgrnd_dim;
		dragarea = Point<int16_t>(dimension.x(), 20);

		// Initial refresh of buddy data
		refresh_buddies();
	}

	void UIBuddyList::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw title text
		title_text.draw(position + Point<int16_t>(25, 62));

		// Draw the buddy rows
		int16_t row_start_y = 115;
		int16_t name_x = 40;
		int16_t status_x = 160;

		int16_t visible_count = static_cast<int16_t>(cached_buddies.size());
		if (visible_count > MAX_VISIBLE)
			visible_count = MAX_VISIBLE;

		for (int16_t i = 0; i < visible_count; i++)
		{
			int16_t idx = i + scroll_offset;
			if (idx >= static_cast<int16_t>(cached_buddies.size()))
				break;

			const auto& buddy = cached_buddies[idx];
			int16_t row_y = row_start_y + i * ROW_HEIGHT;
			Point<int16_t> row_pos = position + Point<int16_t>(10, row_y);

			// Draw row background (cycle through friend0-friend6)
			// Use a different texture index for selected buddy
			int16_t tex_idx = (idx == selected_buddy) ? 0 : (i % 7);
			row_textures[tex_idx].draw(row_pos);

			// Draw the separator line between rows
			if (i > 0)
				line.draw(row_pos + Point<int16_t>(0, -1));

			// Draw status icon: icon0 = online, icon1 = offline
			int16_t icon_idx = buddy.is_online ? 0 : 1;
			status_icons[icon_idx].draw(row_pos + Point<int16_t>(3, 3));

			// Draw friend name
			name_label.change_text(buddy.name);
			name_label.draw(position + Point<int16_t>(name_x, row_y + 4));

			// Draw online status / channel info
			std::string status_str;
			if (buddy.is_online)
				status_str = "Ch." + std::to_string(buddy.channel + 1);
			else
				status_str = "Offline";

			status_label.change_text(status_str);
			status_label.draw(position + Point<int16_t>(status_x, row_y + 4));
		}

		// Draw button area background at the bottom
		button_bg.draw(position + Point<int16_t>(0, row_start_y + visible_count * ROW_HEIGHT + 5));

		UIElement::draw_buttons(inter);
	}

	void UIBuddyList::update()
	{
		UIElement::update();

		// Periodically refresh buddy data
		refresh_buddies();
	}

	Cursor::State UIBuddyList::send_cursor(bool pressed, Point<int16_t> cursorpos)
	{
		if (UIDragElement::send_cursor(pressed, cursorpos) == Cursor::State::CLICKING)
			return Cursor::State::CLICKING;

		Point<int16_t> cursoroffset = cursorpos - position;

		if (pressed)
		{
			int16_t row_start_y = 115;
			int16_t row_y = cursoroffset.y() - row_start_y;

			if (row_y >= 0 && cursoroffset.x() >= 10 && cursoroffset.x() <= dimension.x() - 10)
			{
				int16_t row_index = row_y / ROW_HEIGHT + scroll_offset;

				if (row_index >= 0 && row_index < static_cast<int16_t>(cached_buddies.size()))
					selected_buddy = row_index;
			}
		}

		return UIElement::send_cursor(pressed, cursorpos);
	}

	Button::State UIBuddyList::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			return Button::State::NORMAL;
		case Buttons::BT_ADD:
		{
			// Add buddy using the last chat target or the selected character
			// In v83, the Add button opens a name input — use chat /buddy <name> for now
			// If a character is targeted (via clicking their name in-game), add them directly
			int32_t target_cid = Stage::get().get_player().get_oid();

			// Show instruction message
			UI::get().emplace<UIOk>("Use the chat command to add a buddy:\n/buddy <character name>",
				[](bool) {});

			return Button::State::NORMAL;
		}
		case Buttons::BT_DELETE:
		{
			if (selected_buddy >= 0 && (size_t)selected_buddy < cached_buddies.size())
			{
				const auto& buddy = cached_buddies[selected_buddy];
				int32_t cid = buddy.cid;
				std::string name = buddy.name;

				UI::get().emplace<UIYesNo>("Remove " + name + " from your buddy list?",
					[cid](bool yes)
					{
						if (yes)
							DeleteBuddyPacket(cid).dispatch();
					});
			}
			return Button::State::NORMAL;
		}
		case Buttons::BT_WHISPER:
		{
			if (selected_buddy >= 0 && (size_t)selected_buddy < cached_buddies.size())
			{
				const auto& buddy = cached_buddies[selected_buddy];

				if (buddy.is_online)
					FindPlayerPacket(buddy.name).dispatch();
			}
			return Button::State::NORMAL;
		}
		default:
			break;
		}

		return Button::State::DISABLED;
	}

	void UIBuddyList::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIBuddyList::get_type() const
	{
		return TYPE;
	}

	void UIBuddyList::refresh_buddies()
	{
		cached_buddies.clear();

		const auto& entries = Stage::get().get_player().get_buddylist().get_entries();

		for (const auto& [cid, entry] : entries)
		{
			BuddyDisplay display;
			display.cid = cid;
			display.name = entry.name;
			display.group = entry.group;
			display.channel = entry.channel;
			display.is_online = entry.online();

			cached_buddies.push_back(display);
		}

		// Sort: online friends first, then alphabetically
		std::sort(cached_buddies.begin(), cached_buddies.end(),
			[](const BuddyDisplay& a, const BuddyDisplay& b)
			{
				if (a.is_online != b.is_online)
					return a.is_online > b.is_online;
				return a.name < b.name;
			});

		// Update title with count
		int online_count = 0;
		for (const auto& b : cached_buddies)
			if (b.is_online)
				online_count++;

		int8_t capacity = Stage::get().get_player().get_buddylist().get_capacity();
		std::string title = "Friends (" + std::to_string(online_count) + "/" + std::to_string(cached_buddies.size()) + ")";
		title_text.change_text(title);
	}
}
