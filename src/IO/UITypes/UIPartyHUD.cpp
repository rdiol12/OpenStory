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
//////////////////////////////////////////////////////////////////////////////////
#include "UIPartyHUD.h"

#include "UINotice.h"

#include "../UI.h"

#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/GameplayPackets.h"

#include "../../Graphics/Geometry.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartyHUD::UIPartyHUD() : UIDragElement<PosPARTYHUD>(Point<int16_t>(WIDTH, 20))
	{
		nl::node PartyHP = nl::nx::ui["UIWindow.img"]["UserList"]["Party"]["PartyHP"];
		nl::node Gauge = PartyHP["GaugeBar"];

		// panel backdrop = the item-tooltip 9-slice frame
		frame = MapleFrame(nl::nx::ui["UIToolTip.img"]["Item"]["Frame2"]);

		gauge_bar = Gauge["bar"];
		gauge_fill = Gauge["gauge"];
		gauge_grad = Gauge["graduation"];

		leader_star = nl::nx::ui["UIWindow2.img"]["UserList"]["Main"]["Party"]["PartySearch"]["PartyInfo"]["leader"];

		// same X as the quest helper's per-quest close button
		nl::node btdel = nl::nx::ui["UIWindow2.img"]["QuestAlarm"]["BtDelete"];
		if (!btdel)
			btdel = nl::nx::ui["UIWindow.img"]["QuestAlarm"]["BtDelete"];
		kick_btn = Texture(btdel["normal"]["0"]);

		title = Text(Text::Font::A13B, Text::Alignment::LEFT, Color::Name::WHITE);
		member_name = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::WHITE);
		member_hp_text = Text(Text::Font::A11B, Text::Alignment::RIGHT, Color::Name::WHITE);

		dimension = Point<int16_t>(WIDTH, TITLE_H + 14);
	}

	void UIPartyHUD::draw(float inter) const
	{
		const Party& party = Stage::get().get_player().get_party();

		if (!party.is_in_party())
			return;

		const auto& members = party.get_members();
		int16_t rows = static_cast<int16_t>(members.size());
		int16_t inner_h = static_cast<int16_t>(TITLE_H + rows * ROW_H);

		int16_t top_h = 7;
		int16_t panel_h = static_cast<int16_t>(top_h + inner_h + 10);

		Point<int16_t> tl = position;

		frame.draw(tl + Point<int16_t>(WIDTH / 2, panel_h - 6), WIDTH - 19, panel_h - 17);

		// title: the leader's party
		std::string leader_name;
		for (const auto& m : members)
			if (m.cid == party.get_leader())
				leader_name = m.name;

		title.change_text(leader_name.empty() ? "Party" : leader_name + "'s Party");
		title.draw(tl + Point<int16_t>(10, top_h + 1));

		// white divider under the title, before the gauges
		static const ColorBox divider(WIDTH - 10, 1, Color::Name::WHITE, 0.7f);
		divider.draw(DrawArgument(tl + Point<int16_t>(5, top_h + TITLE_H + 3)));

		int32_t my_cid = Stage::get().get_player().get_oid();
		bool i_lead = (my_cid == party.get_leader());
		kick_hits.clear();

		for (int16_t i = 0; i < rows; i++)
		{
			const PartyMember& m = members[i];
			int16_t row_y = static_cast<int16_t>(top_h + TITLE_H + 11 + i * ROW_H);

			if (m.cid == party.get_leader())
				leader_star.draw(DrawArgument(tl + Point<int16_t>(4, row_y + 7)));

			member_name.change_text(m.name);
			// row order: gauge on top, 2px, then the name
			member_name.draw(tl + Point<int16_t>(15, row_y + 6));

			// HP for self comes live from our stats; others from party updates
			int32_t hp = m.hp, maxhp = m.maxhp;
			if (m.cid == my_cid)
			{
				const CharStats& stats = Stage::get().get_player().get_stats();
				hp = stats.get_stat(MapleStat::Id::HP);
				maxhp = stats.get_total(EquipStat::Id::HP);
			}

			// gauge: bar frame + fill scaled by hp ratio (bar origin -3,-3)
			Point<int16_t> bar_at = tl + Point<int16_t>(15, row_y - 3);
			gauge_bar.draw(DrawArgument(bar_at));

			if (maxhp > 0)
			{
				float ratio = hp > 0 ? static_cast<float>(hp) / static_cast<float>(maxhp) : 0.0f;
				if (ratio > 1.0f)
					ratio = 1.0f;
				// graduation interior track: x 4..64, y 3..9 from its corner
				int16_t fill_w = static_cast<int16_t>(61 * ratio);
				if (fill_w > 0)
					gauge_fill.draw(DrawArgument(
						bar_at + Point<int16_t>(4, 3),
						Point<int16_t>(fill_w, 0)));

				member_hp_text.change_text(std::to_string(hp) + " / " + std::to_string(maxhp));
			}
			else
			{
				member_hp_text.change_text(m.online ? "-" : "offline");
			}

			member_hp_text.draw(tl + Point<int16_t>(WIDTH - 8, row_y - 3));
			gauge_grad.draw(DrawArgument(bar_at));

			// leader-only kick X beside the member's name
			if (i_lead && m.cid != my_cid)
			{
				// right after the gauge bar (bar spans x 15..84)
				Point<int16_t> kx = tl + Point<int16_t>(84, row_y - 5);
				kick_btn.draw(DrawArgument(kx));
				kick_hits.push_back({ m.cid, m.name,
					Rectangle<int16_t>(kx, kx + Point<int16_t>(14, 14)) });
			}
		}

		UIElement::draw(inter);
	}

	void UIPartyHUD::update()
	{
		UIElement::update();

		const Party& party = Stage::get().get_player().get_party();
		Player& player = Stage::get().get_player();
		MapChars& chars = Stage::get().get_chars();

		// overhead gauges: stamp current members, clear anyone who left
		std::vector<int32_t> now;

		if (party.is_in_party())
		{
			int32_t my_cid = player.get_oid();

			for (const auto& m : party.get_members())
			{
				// no overhead bar on yourself — only the rest of the party
				if (m.cid == my_cid)
					continue;

				now.push_back(m.cid);

				if (auto chr = chars.get_char(m.cid))
					chr->set_party_hp(m.hp, m.maxhp);
			}

			int16_t rows = static_cast<int16_t>(party.get_members().size());
			dimension = Point<int16_t>(WIDTH, static_cast<int16_t>(TITLE_H + rows * ROW_H + 14));
		}

		for (int32_t cid : stamped_cids)
		{
			if (std::find(now.begin(), now.end(), cid) != now.end())
				continue;

			if (cid == player.get_oid())
				player.clear_party_hp();
			else if (auto chr = chars.get_char(cid))
				chr->clear_party_hp();
		}

		stamped_cids = std::move(now);
	}

	Cursor::State UIPartyHUD::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		for (const auto& hit : kick_hits)
		{
			if (hit.rect.contains(cursorpos))
			{
				if (clicked)
				{
					int32_t cid = hit.cid;
					UI::get().emplace<UIYesNo>(
						"Kick " + hit.name + " from the party?",
						[cid](bool yes)
						{
							if (yes)
								ExpelFromPartyPacket(cid).dispatch();
						});

					return Cursor::State::CLICKING;
				}

				return Cursor::State::CANCLICK;
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIPartyHUD::get_type() const
	{
		return TYPE;
	}
}
