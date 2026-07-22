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

#include "UINotice.h"

#include "../Components/MapleButton.h"
#include "../UI.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIAlliance::UIAlliance() : UIDragElement<PosALLIANCE>(Point<int16_t>(W, 90)), rank(0), capacity(0)
	{
		nl::node src = nl::nx::ui["GuildUI.img"];
		nl::node uni = src["union"];

		sprites.emplace_back(src["backgrnd1"]);
		sprites.emplace_back(src["backgrnd2"]);

		no_union_tex = uni["unionOff"]["layer:noUnion"];
		head_u_tex = uni["unionOn"]["layer:head_u"];
		head_ut_tex = uni["unionOn"]["layer:head_ut"];
		row_tex = uni["unionOn"]["table"]["list"];
		cover_tex = src["guildInfo"]["layer:cover"];

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(src["top"]["guildMember"]["button:Min"]);
		buttons[BT_INVITE] = std::make_unique<MapleButton>(
			uni["unionOn"]["button:Invite"], Point<int16_t>(CONTENT_X, CONTENT_Y));
		buttons[BT_LEAVE] = std::make_unique<MapleButton>(
			uni["unionOn"]["button:LeaveUnion"], Point<int16_t>(CONTENT_X, CONTENT_Y));

		buttons[BT_INVITE]->set_active(false);
		buttons[BT_LEAVE]->set_active(false);

		name_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE, "No Alliance");
		notice_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "", 290);
		cap_text = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE);
		guild_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);

		dimension = Point<int16_t>(W, H);
	}

	void UIAlliance::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		name_text.draw(position + Point<int16_t>(95, 33));

		cover_tex.draw(DrawArgument(position));
		notice_text.draw(position + Point<int16_t>(112, 62));

		Point<int16_t> content = position + Point<int16_t>(CONTENT_X, CONTENT_Y);

		if (alliance_name.empty())
		{
			no_union_tex.draw(DrawArgument(content));
		}
		else
		{
			head_u_tex.draw(DrawArgument(content));
			head_ut_tex.draw(DrawArgument(content));

			cap_text.change_text(std::to_string(guilds.size()) + "/" + std::to_string(capacity) + " guilds");
			cap_text.draw(position + Point<int16_t>(510, 96));

			int16_t visible = static_cast<int16_t>(guilds.size());

			if (visible > MAX_VISIBLE_GUILDS)
				visible = MAX_VISIBLE_GUILDS;

			for (int16_t i = 0; i < visible; i++)
			{
				int16_t row_y = ROW_Y + i * ROW_H;

				row_tex.draw(DrawArgument(content + Point<int16_t>(14, row_y)));

				guild_label.change_text(guilds[i].name);
				guild_label.draw(content + Point<int16_t>(35, row_y + 4));
			}
		}

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
		case BT_CLOSE:
			deactivate();
			break;
		case BT_INVITE:
			UI::get().emplace<UIEnterText>(
				"Guild to invite:",
				[](const std::string& guild_name)
				{
					if (!guild_name.empty())
						AllianceInvitePacket(guild_name).dispatch();
				},
				12
			);
			break;
		case BT_LEAVE:
			UI::get().emplace<UIYesNo>(
				"Leave the alliance? (Guild Master only)",
				[](bool yes)
				{
					if (yes)
						AllianceLeavePacket().dispatch();
				}
			);
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

		name_text.change_text(name.empty() ? "No Alliance" : name);

		buttons[BT_INVITE]->set_active(!name.empty());
		buttons[BT_LEAVE]->set_active(!name.empty());
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
		notice_text.change_text(n);
	}
}
