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
#include "UIGuild.h"

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuild::UIGuild() : UIDragElement<PosGUILD>(), tab(0), scroll_offset(0)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["GuildWindow"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(src["backgrnd2"]);

		tabbar = src["tabbar"];

		// Load per-tab background overlays (tab0 through tab5)
		for (size_t i = 0; i <= 5; i++)
		{
			nl::node tab_node = src["tab" + std::to_string(i)];

			if (tab_node && tab_node["backgrnd"])
				backgrounds.emplace_back(tab_node["backgrnd"]);
			else if (tab_node)
				backgrounds.emplace_back(tab_node);
			else
				backgrounds.emplace_back(Texture());
		}

		// Close button at top-right
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg.get_dimensions().x() - 19, 5));

		// Tab buttons (Members, Skills, Rank, Board, Mark, Search)
		buttons[Buttons::BT_TAB0] = std::make_unique<MapleButton>(src["BtTab"]["0"]);
		buttons[Buttons::BT_TAB1] = std::make_unique<MapleButton>(src["BtTab"]["1"]);
		buttons[Buttons::BT_TAB2] = std::make_unique<MapleButton>(src["BtTab"]["2"]);
		buttons[Buttons::BT_TAB3] = std::make_unique<MapleButton>(src["BtTab"]["3"]);
		buttons[Buttons::BT_TAB4] = std::make_unique<MapleButton>(src["BtTab"]["4"]);
		buttons[Buttons::BT_TAB5] = std::make_unique<MapleButton>(src["BtTab"]["5"]);

		// Members tab action buttons (only create if nodes exist)
		nl::node tab0_node = src["tab0"];
		if (tab0_node)
		{
			if (tab0_node["BtInvite"])
				buttons[Buttons::BT_INVITE] = std::make_unique<MapleButton>(tab0_node["BtInvite"]);

			if (tab0_node["BtExpel"])
				buttons[Buttons::BT_EXPEL] = std::make_unique<MapleButton>(tab0_node["BtExpel"]);

			if (tab0_node["BtChangeName"])
				buttons[Buttons::BT_CHANGENAME] = std::make_unique<MapleButton>(tab0_node["BtChangeName"]);

			if (tab0_node["BtChangeNotice"])
				buttons[Buttons::BT_CHANGENOTICE] = std::make_unique<MapleButton>(tab0_node["BtChangeNotice"]);

			if (tab0_node["BtBBS"])
				buttons[Buttons::BT_BBS] = std::make_unique<MapleButton>(tab0_node["BtBBS"]);

			if (tab0_node["BtDisband"])
				buttons[Buttons::BT_DISBAND] = std::make_unique<MapleButton>(tab0_node["BtDisband"]);

			// Notice background texture
			if (tab0_node["notice"])
				notice_backgrnd = tab0_node["notice"];
		}

		// Initialize text labels
		guild_name = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "No Guild");
		guild_notice = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "");
		guild_level = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Lv. 1");
		guild_capacity = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "0/100");
		member_count_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Members: 0");
		member_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		member_info_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);

		// Start on the Members tab
		change_tab(0);
	}

	void UIGuild::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw the tab bar
		tabbar.draw(DrawArgument(position));

		// Draw the active tab's background overlay
		if (tab < backgrounds.size())
			backgrounds[tab].draw(DrawArgument(position));

		// Draw guild name centered near the top
		guild_name.draw(position + Point<int16_t>(dimension.x() / 2, 30));

		// Draw tab-specific content
		if (tab == 0)
		{
			// Members tab: show notice and member list area

			// Draw notice area
			notice_backgrnd.draw(position + Point<int16_t>(0, 0));
			guild_notice.draw(position + Point<int16_t>(20, 65));

			// Draw member count
			member_count_text.draw(position + Point<int16_t>(15, 98));

			// Draw member list entries
			int16_t visible_count = static_cast<int16_t>(members.size()) - scroll_offset;
			if (visible_count > MAX_VISIBLE_MEMBERS)
				visible_count = MAX_VISIBLE_MEMBERS;

			for (int16_t i = 0; i < visible_count; i++)
			{
				int16_t idx = i + scroll_offset;
				if (idx >= static_cast<int16_t>(members.size()))
					break;

				const auto& member = members[idx];
				int16_t row_y = MEMBER_LIST_Y + i * MEMBER_ROW_HEIGHT;

				// Draw member name
				member_name_label.change_text(member.name);
				member_name_label.draw(position + Point<int16_t>(20, row_y + 2));

				// Draw member info (level/rank/online status)
				std::string info = "Lv." + std::to_string(member.level) + " " + member.rank;

				if (member.online)
					info += " [ON]";
				else
					info += " [OFF]";

				member_info_label.change_text(info);
				member_info_label.draw(position + Point<int16_t>(130, row_y + 2));
			}
		}

		UIElement::draw_buttons(inter);
	}

	void UIGuild::update()
	{
		UIElement::update();
	}

	void UIGuild::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UIGuild::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIGuild::get_type() const
	{
		return TYPE;
	}

	Button::State UIGuild::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
		case Buttons::BT_TAB5:
			change_tab(buttonid - Buttons::BT_TAB0);
			return Button::State::IDENTITY;
		case Buttons::BT_INVITE:
			// TODO: Open invite dialog (requires server packet)
			break;
		case Buttons::BT_EXPEL:
			// TODO: Expel selected member (requires server packet)
			break;
		case Buttons::BT_CHANGENAME:
			// TODO: Change guild name (requires server packet)
			break;
		case Buttons::BT_CHANGENOTICE:
			// TODO: Change notice (requires server packet)
			break;
		case Buttons::BT_BBS:
			// TODO: Open UIGuildBBS
			break;
		case Buttons::BT_DISBAND:
			// TODO: Disband guild (requires server packet)
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIGuild::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;
		tab = tabid;

		// Reset old tab button to normal state
		if (oldtab != tab)
		{
			auto old_it = buttons.find(Buttons::BT_TAB0 + oldtab);
			if (old_it != buttons.end() && old_it->second)
				old_it->second->set_state(Button::State::NORMAL);
		}

		// Set the new tab button to pressed state
		auto new_it = buttons.find(Buttons::BT_TAB0 + tab);
		if (new_it != buttons.end() && new_it->second)
			new_it->second->set_state(Button::State::PRESSED);

		// Show/hide members tab action buttons based on active tab
		bool on_members = (tab == 0);

		auto set_active = [&](uint16_t id, bool active)
		{
			auto it = buttons.find(id);
			if (it != buttons.end() && it->second)
				it->second->set_active(active);
		};

		set_active(Buttons::BT_INVITE, on_members);
		set_active(Buttons::BT_EXPEL, on_members);
		set_active(Buttons::BT_CHANGENAME, on_members);
		set_active(Buttons::BT_CHANGENOTICE, on_members);
		set_active(Buttons::BT_BBS, on_members);
		set_active(Buttons::BT_DISBAND, on_members);

		// Reset scroll
		scroll_offset = 0;
	}
}
