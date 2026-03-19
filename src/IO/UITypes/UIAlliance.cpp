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
#include "UIAlliance.h"

#include "../UI.h"
#include "../Components/AreaButton.h"
#include "UIChatBar.h"

#include "../../Graphics/Geometry.h"

namespace ms
{
	UIAlliance::UIAlliance() : UIDragElement<PosALLIANCE>(), rank(0), capacity(0)
	{
		dimension = Point<int16_t>(300, 300);
		dragarea = Point<int16_t>(300, 20);

		// Close button at top-right
		buttons[Buttons::BT_CLOSE] = std::make_unique<AreaButton>(Point<int16_t>(275, 3), Point<int16_t>(18, 15));

		// Invite button near the bottom
		buttons[Buttons::BT_INVITE] = std::make_unique<AreaButton>(Point<int16_t>(30, 265), Point<int16_t>(100, 20));

		// Leave button near the bottom
		buttons[Buttons::BT_LEAVE] = std::make_unique<AreaButton>(Point<int16_t>(170, 265), Point<int16_t>(100, 20));

		title_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Alliance");
		notice_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "");
		guild_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);

		alliance_name = "";
		notice = "";
	}

	void UIAlliance::draw(float inter) const
	{
		// Draw background
		ColorBox bg_box(300, 300, Color::Name::JAMBALAYA, 0.85f);
		bg_box.draw(DrawArgument(position));

		// Title bar
		ColorBox title_bar(300, 20, Color::Name::EMPEROR, 1.0f);
		title_bar.draw(DrawArgument(position));

		// Title text
		title_label.draw(position + Point<int16_t>(150, 3));

		// Close button "X" indicator
		Text close_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "X");
		close_text.draw(position + Point<int16_t>(284, 3));

		// Alliance name
		if (!alliance_name.empty())
		{
			Text name_text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, alliance_name);
			name_text.draw(position + Point<int16_t>(150, 30));

			// Capacity info
			std::string cap_str = "Guilds: " + std::to_string(guilds.size()) + "/" + std::to_string(capacity);
			Text cap_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, cap_str);
			cap_text.draw(position + Point<int16_t>(15, 50));
		}
		else
		{
			Text no_alliance(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "No Alliance");
			no_alliance.draw(position + Point<int16_t>(150, 30));
		}

		// Notice section
		if (!notice.empty())
		{
			Text notice_header(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Notice:");
			notice_header.draw(position + Point<int16_t>(15, 70));

			notice_label.draw(position + Point<int16_t>(15, 88));
		}

		// Guild list separator line
		ColorLine sep_line(270, Color::Name::DUSTYGRAY, 0.7f, false);
		sep_line.draw(DrawArgument(position + Point<int16_t>(15, 110)));

		// Guild list header
		Text guild_header(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Member Guilds:");
		guild_header.draw(position + Point<int16_t>(15, 115));

		// Draw guild entries
		for (size_t i = 0; i < guilds.size(); i++)
		{
			int16_t row_y = static_cast<int16_t>(135 + i * 22);

			if (row_y > 250)
				break;

			guild_label.change_text(guilds[i].name + " (ID: " + std::to_string(guilds[i].guild_id) + ")");
			guild_label.draw(position + Point<int16_t>(25, row_y));
		}

		// Invite button area
		ColorBox invite_box(100, 20, Color::Name::EMPEROR, 0.8f);
		invite_box.draw(DrawArgument(position + Point<int16_t>(30, 265)));

		Text invite_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Invite");
		invite_text.draw(position + Point<int16_t>(80, 267));

		// Leave button area
		ColorBox leave_box(100, 20, Color::Name::EMPEROR, 0.8f);
		leave_box.draw(DrawArgument(position + Point<int16_t>(170, 265)));

		Text leave_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Leave");
		leave_text.draw(position + Point<int16_t>(220, 267));

		UIElement::draw_buttons(inter);
	}

	void UIAlliance::update()
	{
		UIElement::update();
	}

	void UIAlliance::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIAlliance::get_type() const
	{
		return TYPE;
	}

	Button::State UIAlliance::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_INVITE:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Use @allianceinvite <guild_name> to invite a guild.", UIChatBar::LineType::YELLOW);
			break;
		case Buttons::BT_LEAVE:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Use @allianceleave to leave the alliance.", UIChatBar::LineType::YELLOW);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIAlliance::set_alliance_info(const std::string& name, int8_t r, int8_t cap)
	{
		alliance_name = name;
		rank = r;
		capacity = cap;
		title_label.change_text("Alliance - " + name);
	}

	void UIAlliance::add_guild(const std::string& guild_name, int32_t guild_id)
	{
		guilds.push_back({ guild_name, guild_id });
	}

	void UIAlliance::clear_guilds()
	{
		guilds.clear();
	}

	void UIAlliance::set_notice(const std::string& n)
	{
		notice = n;
		notice_label.change_text(n);
	}
}
