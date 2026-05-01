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
	UIAlliance::UIAlliance() : UIDragElement<PosALLIANCE>(Point<int16_t>(300, 20)), rank(0), capacity(0)
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

		// Pre-allocate draw objects
		bg_box = ColorBox(300, 300, Color::Name::JAMBALAYA, 0.85f);
		title_bar = ColorBox(300, 20, Color::Name::EMPEROR, 1.0f);
		close_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "X");
		name_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE);
		cap_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);
		no_alliance_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "No Alliance");
		notice_header = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Notice:");
		guild_header = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Member Guilds:");
		invite_box = ColorBox(100, 20, Color::Name::EMPEROR, 0.8f);
		invite_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Invite");
		leave_box = ColorBox(100, 20, Color::Name::EMPEROR, 0.8f);
		leave_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Leave");
	}

	void UIAlliance::draw(float inter) const
	{
		bg_box.draw(DrawArgument(position));
		title_bar.draw(DrawArgument(position));
		title_label.draw(position + Point<int16_t>(150, 3));
		close_text.draw(position + Point<int16_t>(284, 3));

		if (!alliance_name.empty())
		{
			name_text.draw(position + Point<int16_t>(150, 30));
			cap_text.draw(position + Point<int16_t>(15, 50));
		}
		else
		{
			no_alliance_text.draw(position + Point<int16_t>(150, 30));
		}

		if (!notice.empty())
		{
			notice_header.draw(position + Point<int16_t>(15, 70));
			notice_label.draw(position + Point<int16_t>(15, 88));
		}

		guild_header.draw(position + Point<int16_t>(15, 115));

		for (size_t i = 0; i < guilds.size(); i++)
		{
			int16_t row_y = static_cast<int16_t>(135 + i * 22);

			if (row_y > 250)
				break;

			guild_label.change_text(guilds[i].name + " (ID: " + std::to_string(guilds[i].guild_id) + ")");
			guild_label.draw(position + Point<int16_t>(25, row_y));
		}

		invite_box.draw(DrawArgument(position + Point<int16_t>(30, 265)));
		invite_text.draw(position + Point<int16_t>(80, 267));
		leave_box.draw(DrawArgument(position + Point<int16_t>(170, 265)));
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
			chat::log("[Alliance] Use @allianceinvite <guild_name> to invite a guild.", chat::LineType::YELLOW);
			break;
		case Buttons::BT_LEAVE:
			chat::log("[Alliance] Use @allianceleave to leave the alliance.", chat::LineType::YELLOW);
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
		name_text.change_text(name);
		cap_text.change_text("Guilds: " + std::to_string(guilds.size()) + "/" + std::to_string(cap));
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
