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
#include "UIAlliance.h"
#include "UINotice.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../UI.h"

#include "../../Character/Job.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuild::UIGuild() : UIDragElement<PosGUILD>(Point<int16_t>(W, 90)), tab(TAB_MEMBERS), guildlevel(1), gp_value(0), capacity(0), scroll_offset(0)
	{
		// Default v83 rank titles — replaced by the titles the server
		// sends with the full guild info block (sub-op 0x1A).
		static const char* default_titles[5] = { "Master", "Jr. Master", "Member", "Member", "Member" };

		for (int i = 0; i < 5; i++)
			rank_titles[i] = default_titles[i];

		nl::node src = nl::nx::ui["GuildUI.img"];
		nl::node top = src["top"];
		nl::node member = src["member"];

		sprites.emplace_back(src["backgrnd1"]);
		sprites.emplace_back(src["backgrnd2"]);

		for (int i = 0; i < 5; i++)
			flags[i] = top["flag"][std::to_string(i)];

		lvnum = Charset(top["flag"]["lvNumber"], Charset::Alignment::CENTER);
		nomark = top["noGuildMark"];
		cover = src["guildInfo"]["layer:cover"];

		head_tex = member["layer:head"];
		row_tex = member["table"]["list"];
		on_tex = member["userOn"];
		off_tex = member["userOff"];

		bginfo_tex = src["guildInfo"]["layer:bgInfo"];
		charframe_tex = src["guildInfo"]["layer:char"];

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(top["guildMember"]["button:Min"]);

		nl::node taben = top["tab"]["enabled"];
		nl::node tabdis = top["tab"]["disabled"];

		// No Skills tab — Cosmic (v83) has no guild skills. Board and
		// Alliance shift one slot left to close the gap.
		for (uint16_t i = 0; i < 5; i++)
		{
			if (i == TAB_SKILLS)
				continue;

			std::string idx = std::to_string(i);

			if (taben[idx] && tabdis[idx])
			{
				Point<int16_t> shift(i > TAB_SKILLS ? -69 : 0, 0);
				buttons[BT_TAB0 + i] = std::make_unique<TwoSpriteButton>(tabdis[idx], taben[idx], shift);
			}
		}

		buttons[BT_LEAVE] = std::make_unique<MapleButton>(
			src["guildInfo"]["button:LeaveGuild"],
			Point<int16_t>(CONTENT_X, CONTENT_Y));
		buttons[BT_LEAVE]->set_active(false);

		guild_name = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE, "No Guild");
		guild_notice = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "", 290);
		member_count_text = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, "0/0");
		cell_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);
		value_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);

		dimension = Point<int16_t>(W, H);

		change_tab(TAB_MEMBERS);
	}

	void UIGuild::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Flag with the guild level, mark placeholder and name
		int16_t flag_idx = guildlevel - 1;

		if (flag_idx < 0)
			flag_idx = 0;

		if (flag_idx > 4)
			flag_idx = 4;

		flags[flag_idx].draw(DrawArgument(position));
		lvnum.draw(std::to_string(guildlevel), DrawArgument(position + Point<int16_t>(40, 48)));

		// Real guild emblem when the server sent one, placeholder otherwise
		if (emblem_bg.is_valid() || emblem_mark.is_valid())
		{
			Point<int16_t> mark_pos = position + Point<int16_t>(69, 30);

			if (emblem_bg.is_valid())
				emblem_bg.draw(DrawArgument(mark_pos));

			if (emblem_mark.is_valid())
				emblem_mark.draw(DrawArgument(mark_pos));
		}
		else
		{
			nomark.draw(DrawArgument(position));
		}

		guild_name.draw(position + Point<int16_t>(95, 33));

		cover.draw(DrawArgument(position));
		guild_notice.draw(position + Point<int16_t>(112, 62));

		Point<int16_t> content = position + Point<int16_t>(CONTENT_X, CONTENT_Y);

		if (tab == TAB_MEMBERS)
		{
			head_tex.draw(DrawArgument(content));
			member_count_text.draw(position + Point<int16_t>(510, 96));

			int16_t visible_count = static_cast<int16_t>(members.size()) - scroll_offset;

			if (visible_count > MAX_VISIBLE_MEMBERS)
				visible_count = MAX_VISIBLE_MEMBERS;

			for (int16_t i = 0; i < visible_count; i++)
			{
				int16_t idx = i + scroll_offset;

				if (idx >= static_cast<int16_t>(members.size()))
					break;

				const auto& m = members[idx];
				int16_t row_y = MEMBER_LIST_Y + i * MEMBER_ROW_HEIGHT;
				Point<int16_t> row_pos = position + Point<int16_t>(25, row_y);

				row_tex.draw(DrawArgument(row_pos));

				cell_text.change_text(m.name);
				cell_text.draw(position + Point<int16_t>(78, row_y + 4));

				cell_text.change_text(Job(m.job).get_name());
				cell_text.draw(position + Point<int16_t>(168, row_y + 4));

				cell_text.change_text(std::to_string(m.level));
				cell_text.draw(position + Point<int16_t>(250, row_y + 4));

				cell_text.change_text(rank_title(m.rank));
				cell_text.draw(position + Point<int16_t>(325, row_y + 4));

				const Texture& status = m.online ? on_tex : off_tex;
				status.draw(DrawArgument(position + Point<int16_t>(455, row_y + 8)));
			}
		}
		else if (tab == TAB_PROFILE)
		{
			bginfo_tex.draw(DrawArgument(content));
			charframe_tex.draw(DrawArgument(content));

			// Only Cosmic-backed values are drawn; labels without a
			// server-side counterpart (Honor EXP, Ranking, Contribution,
			// IGP) stay empty
			std::string master;

			for (const auto& m : members)
				if (m.rank == 1)
					master = m.name;

			int16_t left_x = 145;
			int16_t row0 = 255;
			int16_t step = 19;

			value_text.change_text(master);
			value_text.draw(position + Point<int16_t>(left_x, row0));

			value_text.change_text(std::to_string(members.size()) + "/" + std::to_string(capacity));
			value_text.draw(position + Point<int16_t>(left_x, row0 + step));

			value_text.change_text(std::to_string(gp_value));
			value_text.draw(position + Point<int16_t>(left_x, row0 + step * 3));

			// My Profile: rank is the only Cosmic-backed value
			std::string myrank;
			int32_t mycid = Stage::get().get_player().get_oid();

			for (const auto& m : members)
				if (m.cid == mycid)
					myrank = rank_title(m.rank);

			value_text.change_text(myrank);
			value_text.draw(position + Point<int16_t>(390, row0));
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
		guildlevel = level;
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
		gp_value = gp;
		guildlevel = level_for_gp(gp);
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

	void UIGuild::set_guild_emblem(int16_t bg, int8_t bgcolor, int16_t logo, int8_t logocolor)
	{
		emblem_bg = Texture();
		emblem_mark = Texture();

		nl::node markimg = nl::nx::ui["GuildMark.img"];

		auto pad = [](int16_t id)
		{
			std::string s = std::to_string(id);
			return std::string(8 - s.size(), '0') + s;
		};

		if (bg > 0)
		{
			nl::node n = markimg["BackGround"][pad(bg)][std::to_string(bgcolor)];

			if (n)
				emblem_bg = n;
		}

		if (logo > 0)
		{
			for (const char* cat : { "Animal", "Etc", "Letter", "Pattern", "Plant" })
			{
				nl::node n = markimg["Mark"][cat][pad(logo)];

				if (n)
				{
					nl::node c = n[std::to_string(logocolor)];

					if (c)
						emblem_mark = c;

					break;
				}
			}
		}
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

		if (capacity > 0)
			member_count_text.change_text(count + "/" + std::to_string(capacity));
		else
			member_count_text.change_text(count);
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

	void UIGuild::send_scroll(double yoffset)
	{
		if (tab != TAB_MEMBERS)
			return;

		int16_t max_offset = static_cast<int16_t>(members.size()) - MAX_VISIBLE_MEMBERS;

		if (max_offset < 0)
			max_offset = 0;

		scroll_offset -= static_cast<int16_t>(yoffset);

		if (scroll_offset < 0)
			scroll_offset = 0;

		if (scroll_offset > max_offset)
			scroll_offset = max_offset;
	}

	UIElement::Type UIGuild::get_type() const
	{
		return TYPE;
	}

	Button::State UIGuild::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			deactivate();
			break;
		case BT_LEAVE:
		{
			std::string myname = Stage::get().get_player().get_name();
			int32_t mycid = Stage::get().get_player().get_oid();

			UI::get().emplace<UIYesNo>(
				"Leave the guild?",
				[mycid, myname](bool yes)
				{
					if (yes)
						GuildLeavePacket(mycid, myname).dispatch();
				}
			);

			return Button::State::NORMAL;
		}
		case BT_TAB0:
		case BT_TAB1:
		case BT_TAB2:
		case BT_TAB3:
		case BT_TAB4:
			change_tab(buttonid - BT_TAB0);
			return Button::State::IDENTITY;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIGuild::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;

		// The board lives in its own window; the alliance too. Opening
		// them doesn't switch the page under this window.
		if (tabid == TAB_BOARD || tabid == TAB_ALLIANCE)
		{
			auto it = buttons.find(BT_TAB0 + tabid);

			if (it != buttons.end() && it->second)
				it->second->set_state(Button::State::NORMAL);

			if (!has_guild())
			{
				UI::get().emplace<UIOk>("You are not in a guild.", [](bool) {});
				return;
			}

			if (tabid == TAB_BOARD)
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
			else
			{
				if (auto alliance = UI::get().get_element<UIAlliance>())
					alliance->makeactive();
				else
					UI::get().emplace<UIAlliance>();
			}

			return;
		}

		tab = tabid;

		if (oldtab != tab)
		{
			auto old_it = buttons.find(BT_TAB0 + oldtab);

			if (old_it != buttons.end() && old_it->second)
				old_it->second->set_state(Button::State::NORMAL);
		}

		auto new_it = buttons.find(BT_TAB0 + tab);

		if (new_it != buttons.end() && new_it->second)
			new_it->second->set_state(Button::State::PRESSED);

		scroll_offset = 0;

		buttons[BT_LEAVE]->set_active(tab == TAB_PROFILE);
	}
}
