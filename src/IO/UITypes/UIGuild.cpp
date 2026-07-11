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

#include "UIGuildBBS.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../UI.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuild::UIGuild() : UIDragElement<PosGUILD>(Point<int16_t>(300, 20)), tab(0), capacity(0), scroll_offset(0)
	{
		// Default v83 rank titles — replaced by the titles the server
		// sends with the full guild info block (sub-op 0x1A).
		static const char* default_titles[5] = { "Master", "Jr. Master", "Member", "Member", "Member" };

		for (int i = 0; i < 5; i++)
			rank_titles[i] = default_titles[i];

		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"];
		nl::node main = src["Main"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = main["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);

		nl::node taben = main["Tab"]["enabled"];
		nl::node tabdis = main["Tab"]["disabled"];

		if (taben.size() > 0)
		{
			for (uint16_t i = 0; i < 4; i++)
			{
				std::string idx = std::to_string(i);

				if (taben[idx] && tabdis[idx])
					buttons[Buttons::BT_TAB0 + i] = std::make_unique<TwoSpriteButton>(tabdis[idx], taben[idx]);
			}
		}

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg.get_dimensions().x() - 19, 7));

		guild_name = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "No Guild");
		guild_notice = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "");
		guild_level = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Lv. 1");
		guild_capacity = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "0/100");
		member_count_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Members: 0");
		member_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		member_info_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);

		change_tab(0);
	}

	void UIGuild::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		guild_name.draw(position + Point<int16_t>(dimension.x() / 2, 30));

		if (tab == 0)
		{
			guild_notice.draw(position + Point<int16_t>(20, 65));
			member_count_text.draw(position + Point<int16_t>(15, 98));
			guild_level.draw(position + Point<int16_t>(dimension.x() - 60, 98));

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

				member_name_label.change_text(member.name);
				member_name_label.draw(position + Point<int16_t>(20, row_y + 2));

				std::string info = "Lv." + std::to_string(member.level) + " " + rank_title(member.rank);
				info += member.online ? " [ON]" : " [OFF]";

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

	void UIGuild::set_guild_info(const std::string& name, const std::string& notice, int16_t level, int16_t cap)
	{
		guild_name.change_text(name);
		guild_notice.change_text(notice);
		guild_level.change_text("Lv. " + std::to_string(level));
		capacity = cap;

		refresh_member_count();
	}

	void UIGuild::set_notice(const std::string& notice)
	{
		guild_notice.change_text(notice);
	}

	void UIGuild::set_capacity(int16_t cap)
	{
		capacity = cap;

		refresh_member_count();
	}

	void UIGuild::set_gp(int32_t gp)
	{
		guild_level.change_text("Lv. " + std::to_string(level_for_gp(gp)));
	}

	int16_t UIGuild::level_for_gp(int32_t gp)
	{
		if (gp >= 15000)
			return 5;
		else if (gp >= 5000)
			return 4;
		else if (gp >= 2000)
			return 3;
		else if (gp >= 500)
			return 2;

		return 1;
	}

	void UIGuild::set_rank_titles(const std::string titles[5])
	{
		// Member rows resolve their rank string at draw time, so the
		// list picks the new titles up automatically.
		for (int i = 0; i < 5; i++)
			rank_titles[i] = titles[i];
	}

	void UIGuild::add_member(int32_t cid, const std::string& name, int32_t rank, int16_t level, int16_t job, bool online)
	{
		members.push_back({ cid, name, rank, level, job, online });

		refresh_member_count();
	}

	void UIGuild::remove_member(int32_t cid)
	{
		for (auto it = members.begin(); it != members.end(); ++it)
		{
			if (it->cid == cid)
			{
				members.erase(it);
				break;
			}
		}

		refresh_member_count();
	}

	void UIGuild::update_member_stats(int32_t cid, int16_t level, int16_t job)
	{
		if (MemberEntry* member = find_member(cid))
		{
			member->level = level;
			member->job = job;
		}
	}

	void UIGuild::set_member_online(int32_t cid, bool online)
	{
		if (MemberEntry* member = find_member(cid))
			member->online = online;
	}

	void UIGuild::set_member_rank(int32_t cid, int32_t rank)
	{
		if (MemberEntry* member = find_member(cid))
			member->rank = rank;
	}

	std::string UIGuild::get_member_name(int32_t cid) const
	{
		for (const auto& member : members)
			if (member.cid == cid)
				return member.name;

		return "";
	}

	void UIGuild::clear_members()
	{
		members.clear();
		scroll_offset = 0;

		refresh_member_count();
	}

	UIGuild::MemberEntry* UIGuild::find_member(int32_t cid)
	{
		for (auto& member : members)
			if (member.cid == cid)
				return &member;

		return nullptr;
	}

	const std::string& UIGuild::rank_title(int32_t rank) const
	{
		return rank_titles[(rank >= 1 && rank <= 5) ? rank - 1 : 2];
	}

	void UIGuild::refresh_member_count()
	{
		std::string count = std::to_string(members.size());
		guild_capacity.change_text(count + "/" + std::to_string(capacity));

		if (capacity > 0)
			member_count_text.change_text("Members: " + count + "/" + std::to_string(capacity));
		else
			member_count_text.change_text("Members: " + count);
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
			change_tab(buttonid - Buttons::BT_TAB0);
			return Button::State::IDENTITY;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIGuild::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			auto old_it = buttons.find(Buttons::BT_TAB0 + oldtab);

			if (old_it != buttons.end() && old_it->second)
				old_it->second->set_state(Button::State::NORMAL);
		}

		auto new_it = buttons.find(Buttons::BT_TAB0 + tab);

		if (new_it != buttons.end() && new_it->second)
			new_it->second->set_state(Button::State::PRESSED);

		scroll_offset = 0;

		// Tab 1 is the bulletin board in the v83 layout. The board lives
		// in its own window; opening it requests the thread list from the
		// server (BBS_OPERATION mode 2).
		if (tab == 1)
		{
			if (auto bbs = UI::get().get_element<UIGuildBBS>())
			{
				bbs->makeactive();
				GuildBBSListPacket(0).dispatch();
			}
			else
			{
				UI::get().emplace<UIGuildBBS>();
			}
		}
	}
}
