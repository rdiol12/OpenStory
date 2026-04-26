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
#include "UIUserList.h"

#include "UIAddBuddy.h"
#include "UIBuddyContextMenu.h"
#include "UIBuddyGroup.h"
#include "UIPartyInvite.h"
#include "UIPartyMemberMenu.h"
#include "UIPartySettings.h"
#include "UINotice.h"
#include "UIWhisper.h"
#include "../UI.h"
#include "../Components/Tooltip.h"
#include "../../IO/Components/MapleButton.h"
#include "../../Gameplay/Stage.h"
#include "../../Character/Job.h"
#include "../../Character/BuddyList.h"
#include "../../Character/BuddyMemoStore.h"
#include "../../Character/AccountCharStore.h"
#include "../../Net/Packets/BuddyPackets.h"
#include "../../Net/Packets/GameplayPackets.h"
#include "../../Configuration.h"

#include <algorithm>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIUserList::UIUserList(uint16_t t) : UIDragElement<PosUSERLIST>(Point<int16_t>(260, 20)), tab(t)
	{
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		UserList = nl::nx::ui["UIWindow2.img"]["UserList"];
		nl::node Main = UserList["Main"];

		sprites.emplace_back(Main["backgrnd"]);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(244, 7));

		nl::node taben = Main["Tab"]["enabled"];
		nl::node tabdis = Main["Tab"]["disabled"];

		buttons[Buttons::BT_TAB_FRIEND] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
		buttons[Buttons::BT_TAB_PARTY] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
		buttons[Buttons::BT_TAB_BOSS] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
		buttons[Buttons::BT_TAB_BLACKLIST] = std::make_unique<TwoSpriteButton>(tabdis["3"], taben["3"]);

		// Party Tab
		nl::node Party = Main["Party"];
		nl::node PartySearch = Party["PartySearch"];

		party_tab = Tab::PARTY_MINE;
		party_title = Party["title"];

		for (size_t i = 0; i <= 4; i++)
			party_mine_grid[i] = UserList["Sheet2"][i];

		party_mine_name = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, "none", 0);

		nl::node party_taben = Party["Tab"]["enabled"];
		nl::node party_tabdis = Party["Tab"]["disabled"];

		buttons[Buttons::BT_PARTY_CREATE] = std::make_unique<MapleButton>(Party["BtPartyMake"]);
		buttons[Buttons::BT_PARTY_INVITE] = std::make_unique<MapleButton>(Party["BtPartyInvite"]);
		buttons[Buttons::BT_PARTY_LEAVE] = std::make_unique<MapleButton>(Party["BtPartyOut"]);
		buttons[Buttons::BT_PARTY_SETTINGS] = std::make_unique<MapleButton>(Party["BtPartySettings"]);
		buttons[Buttons::BT_PARTY_CREATE]->set_active(false);
		buttons[Buttons::BT_PARTY_INVITE]->set_active(false);
		buttons[Buttons::BT_PARTY_LEAVE]->set_active(false);
		buttons[Buttons::BT_PARTY_SETTINGS]->set_active(false);

		buttons[Buttons::BT_TAB_PARTY_MINE] = std::make_unique<TwoSpriteButton>(party_tabdis["0"], party_taben["0"]);
		buttons[Buttons::BT_TAB_PARTY_SEARCH] = std::make_unique<TwoSpriteButton>(party_tabdis["1"], party_taben["1"]);
		buttons[Buttons::BT_TAB_PARTY_MINE]->set_active(false);
		buttons[Buttons::BT_TAB_PARTY_SEARCH]->set_active(false);

		party_search_grid[0] = PartySearch["partyName"];
		party_search_grid[1] = PartySearch["request"];
		party_search_grid[2] = PartySearch["table"];

		buttons[Buttons::BT_PARTY_SEARCH_LEVEL] = std::make_unique<MapleButton>(PartySearch["BtPartyLevel"]);
		buttons[Buttons::BT_PARTY_SEARCH_LEVEL]->set_active(false);

		int16_t party_x = 243;
		int16_t party_y = 114;
		int16_t party_height = party_y + 168;
		int16_t party_unitrows = 6;
		int16_t party_rowmax = 6;
		party_slider = Slider(Slider::Type::DEFAULT_SILVER, Range<int16_t>(party_y, party_height), party_x, party_unitrows, party_rowmax, [](bool) {});

		// Party member row backgrounds and icons
		for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
		{
			party_member_row[i] = Party["party" + std::to_string(i)];
			party_member_names[i] = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, "", 0);
			party_member_levels[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "", 0);
			party_member_jobs[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, "", 0);
		}

		party_icon_online = Party["icon0"];
		party_icon_offline = Party["icon1"];

		// Buddy Tab
		nl::node Friend = Main["Friend"];

		friend_tab = Tab::FRIEND_ALL;
		friend_sprites.emplace_back(Friend["title"]);
		friend_sprites.emplace_back(Friend["CbCondition"]["text"]);
		friend_sprites.emplace_back(UserList["line"], DrawArgument(Point<int16_t>(132, 115), Point<int16_t>(230, 0)));

		buttons[Buttons::BT_FRIEND_GROUP_0] = std::make_unique<MapleButton>(UserList["BtSheetIClose"], Point<int16_t>(13, 118));
		buttons[Buttons::BT_FRIEND_GROUP_0]->set_active(false);

		for (size_t i = 0; i <= 3; i++)
			friend_grid[i] = UserList["Sheet1"][i];

		// Refilled every draw with current online / total / capacity.
		friends_online_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "(0/0/0)", 0);

		friends_cur_location = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, "My Location - " + get_cur_location(), 0);
		friends_name = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "none", 0);
		friends_group_name = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Default Group (0/0)", 0);

		buddy_row_name      = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "");
		buddy_row_status    = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "");
		buddy_group_header  = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "");
		buddy_account_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "");

		// 3-piece SideMenu uses BtSheetIOpen/Close as ▼ / ▶ arrows.
		buddy_arrow_open   = Texture(UserList["BtSheetIOpen"]["normal"]["0"]);
		buddy_arrow_closed = Texture(UserList["BtSheetIClose"]["normal"]["0"]);

		// Hover-tooltip backdrop sourced from the same frame UI item
		// hovers use (UIToolTip.img/Item/Frame2 — 9-slice + cover).
		nl::node tip = nl::nx::ui["UIToolTip.img"]["Item"]["Frame2"];
		tooltip_frame = MapleFrame(tip);
		tooltip_cover = Texture(tip["cover"]);

		buttons[Buttons::BT_FRIEND_ADD] = std::make_unique<MapleButton>(Friend["BtAddFriend"]);
		buttons[Buttons::BT_FRIEND_ADD_GROUP] = std::make_unique<MapleButton>(Friend["BtAddGroup"]);
		buttons[Buttons::BT_FRIEND_EXPAND] = std::make_unique<MapleButton>(Friend["BtPlusFriend"]);
		buttons[Buttons::BT_FRIEND_ADD]->set_active(false);
		buttons[Buttons::BT_FRIEND_ADD_GROUP]->set_active(false);
		buttons[Buttons::BT_FRIEND_EXPAND]->set_active(false);

		buttons[Buttons::BT_TAB_FRIEND_ALL] = std::make_unique<MapleButton>(Friend["TapShowAll"]);
		buttons[Buttons::BT_TAB_FRIEND_ONLINE] = std::make_unique<MapleButton>(Friend["TapShowOnline"]);
		buttons[Buttons::BT_TAB_FRIEND_ALL]->set_active(false);
		buttons[Buttons::BT_TAB_FRIEND_ONLINE]->set_active(false);

		int16_t friends_x = 243;
		int16_t friends_y = 115;
		int16_t friends_height = friends_y + 148;
		int16_t friends_unitrows = 6;
		int16_t friends_rowmax = 6;
		friends_slider = Slider(Slider::Type::DEFAULT_SILVER, Range<int16_t>(friends_y, friends_height), friends_x, friends_unitrows, friends_rowmax, [](bool) {});

		// Boss tab
		nl::node Boss = Main["Boss"];

		boss_sprites.emplace_back(Boss["base"]);
		boss_sprites.emplace_back(Boss["base3"]);
		boss_sprites.emplace_back(Boss["base2"]);

		buttons[Buttons::BT_BOSS_0] = std::make_unique<TwoSpriteButton>(Boss["BossList"]["0"]["icon"]["disabled"]["0"], Boss["BossList"]["0"]["icon"]["normal"]["0"]);
		buttons[Buttons::BT_BOSS_1] = std::make_unique<TwoSpriteButton>(Boss["BossList"]["1"]["icon"]["disabled"]["0"], Boss["BossList"]["1"]["icon"]["normal"]["0"]);
		buttons[Buttons::BT_BOSS_2] = std::make_unique<TwoSpriteButton>(Boss["BossList"]["2"]["icon"]["disabled"]["0"], Boss["BossList"]["2"]["icon"]["normal"]["0"]);
		buttons[Buttons::BT_BOSS_3] = std::make_unique<TwoSpriteButton>(Boss["BossList"]["3"]["icon"]["disabled"]["0"], Boss["BossList"]["3"]["icon"]["normal"]["0"]);
		buttons[Buttons::BT_BOSS_4] = std::make_unique<TwoSpriteButton>(Boss["BossList"]["4"]["icon"]["disabled"]["0"], Boss["BossList"]["4"]["icon"]["normal"]["0"]);
		buttons[Buttons::BT_BOSS_L] = std::make_unique<MapleButton>(Boss["BtArrow"]["Left"]);
		buttons[Buttons::BT_BOSS_R] = std::make_unique<MapleButton>(Boss["BtArrow"]["Right"]);
		buttons[Buttons::BT_BOSS_DIFF_L] = std::make_unique<MapleButton>(Boss["BtArrow2"]["Left"]);
		buttons[Buttons::BT_BOSS_DIFF_R] = std::make_unique<MapleButton>(Boss["BtArrow2"]["Right"]);
		buttons[Buttons::BT_BOSS_GO] = std::make_unique<MapleButton>(Boss["BtEntry"]);
		buttons[Buttons::BT_BOSS_0]->set_active(false);
		buttons[Buttons::BT_BOSS_1]->set_active(false);
		buttons[Buttons::BT_BOSS_2]->set_active(false);
		buttons[Buttons::BT_BOSS_3]->set_active(false);
		buttons[Buttons::BT_BOSS_4]->set_active(false);
		buttons[Buttons::BT_BOSS_L]->set_active(false);
		buttons[Buttons::BT_BOSS_R]->set_active(false);
		buttons[Buttons::BT_BOSS_DIFF_L]->set_active(false);
		buttons[Buttons::BT_BOSS_DIFF_R]->set_active(false);
		buttons[Buttons::BT_BOSS_GO]->set_active(false);

		// Blacklist tab
		nl::node BlackList = Main["BlackList"];

		blacklist_title = BlackList["base"];

		for (size_t i = 0; i < 3; i++)
			blacklist_grid[i] = UserList["Sheet6"][i];

		blacklist_name = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, "none", 0);

		nl::node blacklist_taben = BlackList["Tab"]["enabled"];
		nl::node blacklist_tabdis = BlackList["Tab"]["disabled"];

		buttons[Buttons::BT_BLACKLIST_ADD] = std::make_unique<MapleButton>(BlackList["BtAdd"]);
		buttons[Buttons::BT_BLACKLIST_DELETE] = std::make_unique<MapleButton>(BlackList["BtDelete"]);
		buttons[Buttons::BT_TAB_BLACKLIST_INDIVIDUAL] = std::make_unique<MapleButton>(BlackList["TapShowIndividual"]);
		buttons[Buttons::BT_TAB_BLACKLIST_GUILD] = std::make_unique<MapleButton>(BlackList["TapShowGuild"]);
		buttons[Buttons::BT_BLACKLIST_ADD]->set_active(false);
		buttons[Buttons::BT_BLACKLIST_DELETE]->set_active(false);
		buttons[Buttons::BT_TAB_BLACKLIST_INDIVIDUAL]->set_active(false);
		buttons[Buttons::BT_TAB_BLACKLIST_GUILD]->set_active(false);

		change_tab(tab);

		dimension = Point<int16_t>(276, 390);
	}

	void UIUserList::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		background.draw(position);

		if (tab == Buttons::BT_TAB_PARTY)
		{
			party_title.draw(position);

			if (party_tab == Buttons::BT_TAB_PARTY_MINE)
			{
				// TODO: finish up party tab — leader/member glyphs from
				// PartySearch/PartyInfo/leader, optional HP bar overlay
				// (Main/Party/PartyHP 9-slice + GaugeBar), and wiring
				// the Search sub-tab to actual server results.
				//
				// Always paint the existing NX layout (Sheet2 header at
				// the top, then 6 row backdrops) regardless of party
				// state. The previous code swapped Sheet2 out for
				// `Party["partyN"]` (which doesn't exist on Cosmic v83)
				// the moment a member showed up — that's why the panel
				// went blank as soon as a party formed.
				party_mine_grid[0].draw(position + Point<int16_t>(10, 115));

				constexpr int16_t party_row0_y = 133;
				constexpr int16_t party_row_h  = 18;
				constexpr int16_t party_row_x  = 10;
				constexpr int16_t party_row_w  = 235;

				party_row_hits.clear();

				for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
				{
					int16_t row_y = party_row0_y + static_cast<int16_t>(i) * party_row_h;
					Point<int16_t> row_pos = position + Point<int16_t>(party_row_x, row_y);

					party_mine_grid[4].draw(row_pos);

					if (i < party_members.size())
					{
						const PartyMember& m = party_members[i];

						(m.online ? party_icon_online : party_icon_offline)
							.draw(row_pos + Point<int16_t>(2, 2));

						// Columns: name | job | level. Text is top-anchored
						// so a negative y offset nudges it visually up.
						party_member_names[i].draw(row_pos + Point<int16_t>(17, -1));
						party_member_jobs[i].draw(row_pos + Point<int16_t>(120, -1));
						party_member_levels[i].draw(row_pos + Point<int16_t>(190, -1));

						party_row_hits.push_back({
							m.cid, m.name,
							Rectangle<int16_t>(
								row_pos,
								row_pos + Point<int16_t>(party_row_w, party_row_h))
						});
					}
				}

				if (party_members.empty())
					party_mine_name.draw(position + Point<int16_t>(27, 130));
			}
			else if (party_tab == Buttons::BT_TAB_PARTY_SEARCH)
			{
				party_search_grid[0].draw(position);
				party_search_grid[1].draw(position);
				party_search_grid[2].draw(position);
				party_slider.draw(position);
			}
		}
		else if (tab == Buttons::BT_TAB_FRIEND)
		{
			for (auto sprite : friend_sprites)
				sprite.draw(position, alpha);

			// Refresh online / total / capacity counter from live data.
			{
				const auto& blist = Stage::get().get_player().get_buddylist();
				const auto& blist_entries = blist.get_entries();
				int total = static_cast<int>(blist_entries.size());
				int online = 0;
				for (const auto& kv : blist_entries)
					if (kv.second.online()) online++;
				int cap = static_cast<int>(blist.get_capacity());

				friends_online_text.change_text(
					"(" + std::to_string(online) + "/"
						+ std::to_string(total) + "/"
						+ std::to_string(cap) + ")");
			}
			friends_online_text.draw(position + Point<int16_t>(211, 62));
			friends_cur_location.draw(position + Point<int16_t>(9, 279));

			// Per-group two-pane layout:
			//   Group header (full width, always visible at the top of
			//   its group section).
			//   When expanded, the body splits into:
			//     left sub-pane  (x=10..125)  = account characters
			//     vertical divider (x=128)
			//     right sub-pane (x=132..245) = buddies in this group
			//   Each sub-pane caps visible rows at 6; if its content
			//   has more, that sub-pane scrolls independently via
			//   mouse-wheel when the cursor is over it.
			constexpr int16_t list_y_start = 116;
			constexpr int16_t list_y_max   = 275;          // hard floor — "My Location" line lives at y=279
			constexpr int16_t row_h        = 18;
			constexpr int16_t row_gap      = 2;            // visual gap between buddy rows
			constexpr int16_t row_stride   = row_h + row_gap;
			constexpr int16_t pane_capacity = 6;
			constexpr int16_t pane_max_h   = pane_capacity * row_stride;

			constexpr int16_t panel_x      = 10;
			constexpr int16_t panel_w      = 235;
			constexpr int16_t left_sub_x   = 10;
			constexpr int16_t left_sub_w   = 115;
			constexpr int16_t right_sub_x  = 132;
			constexpr int16_t right_sub_w  = 113;

			Player& me = Stage::get().get_player();
			int32_t my_cid = me.get_oid();
			const auto& account_chars = AccountCharStore::get().list();
			int16_t left_n = static_cast<int16_t>(account_chars.size());

			const auto& entries = me.get_buddylist().get_entries();
			std::map<std::string, std::vector<const BuddyEntry*>> grouped;
			for (const auto& kv : entries)
			{
				const BuddyEntry& e = kv.second;
				if (friend_tab == Tab::FRIEND_ONLINE && !e.online())
					continue;
				std::string g = e.group.empty() ? std::string("Default Group") : e.group;
				grouped[g].push_back(&e);
			}
			if (grouped.find("Default Group") == grouped.end())
				grouped["Default Group"] = {};

			// Within each group: online buddies first (alphabetical),
			// then offline (alphabetical).
			for (auto& kv : grouped)
				std::sort(kv.second.begin(), kv.second.end(),
					[](const BuddyEntry* a, const BuddyEntry* b)
					{
						if (a->online() != b->online())
							return a->online();
						return a->name < b->name;
					});

			buddy_row_hits.clear();
			buddy_arrow_hits.clear();

			// Global-scroll layout: lay out every group back-to-back,
			// shifted by buddy_list_scroll, and clip vertically to the
			// band [list_y_start, list_y_max). Anything that falls
			// outside that band is suppressed but still contributes to
			// the total content height so the wheel can reach it.
			const int16_t clip_top    = list_y_start;
			const int16_t clip_bottom = list_y_max;

			int16_t y = list_y_start - buddy_list_scroll;
			int16_t total_h = 0;

			buddy_list_rect = Rectangle<int16_t>(
				position + Point<int16_t>(panel_x, clip_top),
				position + Point<int16_t>(panel_x + panel_w, clip_bottom));

			for (auto& kv : grouped)
			{
				const std::string& gname = kv.first;
				const auto& list = kv.second;
				int16_t right_n = static_cast<int16_t>(list.size());

				auto it = buddy_group_expanded.find(gname);
				if (it == buddy_group_expanded.end())
				{
					buddy_group_expanded[gname] = true;
					it = buddy_group_expanded.find(gname);
				}
				bool expanded = it->second;

				int online = 0;
				for (auto* e : list) if (e->online()) online++;

				// === Group header (drawn only when within the clip band) ===
				if (y + row_h > clip_top && y < clip_bottom)
				{
					friend_grid[0].draw(position + Point<int16_t>(panel_x, y));

					// Per-group expand/collapse arrow. ▼ when expanded,
					// ▶ when collapsed. Drawn over Sheet1[0] so every
					// group gets the indicator, not just the first one
					// where BT_FRIEND_GROUP_0 happens to sit.
					const Texture& arrow = expanded
						? buddy_arrow_open
						: buddy_arrow_closed;
					arrow.draw(position + Point<int16_t>(panel_x + 3, y + 3));

					buddy_group_header.change_text(
						gname + " (" + std::to_string(online)
						+ "/" + std::to_string(list.size()) + ")");
					buddy_group_header.change_color(Color::Name::WHITE);
					buddy_group_header.draw(position + Point<int16_t>(panel_x + 24, y - 1));

					buddy_arrow_hits.push_back({ gname,
						Rectangle<int16_t>(
							position + Point<int16_t>(panel_x, y),
							position + Point<int16_t>(panel_x + panel_w, y + row_h)) });
				}

				y += row_h;
				total_h += row_h;
				if (!expanded) continue;

				// Sheet1[3] divider cap directly under the header.
				if (y + row_h > clip_top && y < clip_bottom)
					friend_grid[3].draw(position + Point<int16_t>(panel_x, y));

				int16_t body_top = y;

				// === Row backdrops + buddy rows, clipped per row.
				for (int16_t r = 0; r < right_n; ++r)
				{
					int16_t ry = body_top + r * row_stride;
					Rectangle<int16_t> rect(
						position + Point<int16_t>(panel_x, ry),
						position + Point<int16_t>(panel_x + panel_w, ry + row_h));

					const BuddyEntry* e = list[r];
					buddy_row_hits.push_back({ e->cid, rect });

					if (ry + row_h > clip_top && ry < clip_bottom)
					{
						const Texture& bg = (r % 2 == 0)
							? friend_grid[1]
							: friend_grid[2];
						bg.draw(position + Point<int16_t>(panel_x, ry));

						const std::string& memo = BuddyMemoStore::get().get_memo(e->cid);
						std::string display = memo.empty() ? e->name : memo;

						Color::Name col = e->online()
							? Color::Name::BLUE
							: Color::Name::BLACK;
						float row_alpha = e->online() ? 1.0f : 0.5f;
						buddy_row_name.change_color(col);
						buddy_row_name.change_text(display);
						buddy_row_name.draw(DrawArgument(
							position + Point<int16_t>(panel_x + 24, ry - 1),
							row_alpha));

						if (e->online())
						{
							buddy_row_status.change_text(
								"Ch " + std::to_string(e->channel + 1));
							buddy_row_status.draw(
								position + Point<int16_t>(panel_x + 180, ry - 1));
						}
					}
				}

				int16_t group_body_h = right_n * row_stride;
				if (right_n == 0) group_body_h = row_stride;
				y += group_body_h + 4;
				total_h += group_body_h + 4;
			}

			// Clamp the scroll so the user can't push past the last row.
			int16_t visible_h = clip_bottom - clip_top;
			buddy_list_max = std::max<int16_t>(0, total_h - visible_h);
			if (buddy_list_scroll > buddy_list_max)
				buddy_list_scroll = buddy_list_max;
			if (buddy_list_scroll < 0)
				buddy_list_scroll = 0;

		}
		else if (tab == Buttons::BT_TAB_BOSS)
		{
			for (auto sprite : boss_sprites)
				sprite.draw(position, alpha);
		}
		else if (tab == Buttons::BT_TAB_BLACKLIST)
		{
			blacklist_title.draw(position + Point<int16_t>(24, 104));
			blacklist_grid[0].draw(position + Point<int16_t>(24, 134));
			blacklist_name.draw(position + Point<int16_t>(24, 134));
		}

		UIElement::draw_buttons(alpha);

		// TODO: fix hover animation — the per-row yellow glow used to
		// highlight the buddy under the cursor was removed because of
		// y-alignment drift; revisit once the row text top-anchor +
		// scroll math has been re-verified.
		//
		// === Hover tooltip for the focused buddy row (drawn LAST) ===
		// Has to render after every sprite + button on this UI so it
		// can never be covered by row backdrops, the location text, or
		// footer buttons. Only relevant on the Friend tab.
		if (tab == Buttons::BT_TAB_FRIEND && hovered_buddy_cid != 0)
		{
			const auto& entries =
				Stage::get().get_player().get_buddylist().get_entries();
			auto it = entries.find(hovered_buddy_cid);
			if (it != entries.end())
			{
				const BuddyEntry& e = it->second;
				int32_t my_ch = Configuration::get().get_channelid();
				std::string ch_line;
				if (e.online())
				{
					ch_line = (e.channel == my_ch)
						? std::string("Same Channel")
						: "Channel " + std::to_string(e.channel + 1);
				}
				const std::string& memo_content =
					BuddyMemoStore::get().get_content(hovered_buddy_cid);

				Point<int16_t> tip_pos =
					UI::get().get_cursor_position() + Point<int16_t>(14, 8);

				constexpr int16_t TIP_W = 220;
				int16_t line_h = 14;
				int16_t lines = e.online() ? 4 : 3;
				int16_t tip_h = line_h * lines + 10 + line_h;
				if (!memo_content.empty()) tip_h += 10 + line_h;

				tooltip_frame.draw(tip_pos + Point<int16_t>(TIP_W / 2, tip_h - 6),
					TIP_W - 19, tip_h - 17);
				tooltip_cover.draw(tip_pos + Point<int16_t>(-5, -2));

				int16_t ty = tip_pos.y() + 4;

				if (e.online())
				{
					Text(Text::Font::A11M, Text::Alignment::LEFT,
						Color::Name::WHITE, ch_line, 0)
						.draw(Point<int16_t>(tip_pos.x() + 6, ty));
					ty += line_h;
				}
				Text(Text::Font::A11M, Text::Alignment::LEFT,
					Color::Name::WHITE, "Double-click to send a Whisper", 0)
					.draw(Point<int16_t>(tip_pos.x() + 6, ty));
				ty += line_h * 2;
				Text(Text::Font::A11M, Text::Alignment::LEFT,
					Color::Name::WHITE, "Right-Click to open your buddy menu", 0)
					.draw(Point<int16_t>(tip_pos.x() + 6, ty));
				ty += line_h + 5;

				std::string status_line =
					(e.online() ? std::string("online") : std::string("offline"))
					+ ": " + e.name;
				Text(Text::Font::A11B, Text::Alignment::LEFT,
					Color::Name::ORANGE, status_line, 0)
					.draw(Point<int16_t>(tip_pos.x() + 6, ty));
				ty += line_h;

				if (!memo_content.empty())
				{
					ty += 10;
					Text(Text::Font::A11M, Text::Alignment::LEFT,
						Color::Name::WHITE, memo_content, 0)
						.draw(Point<int16_t>(tip_pos.x() + 6, ty));
				}
			}
		}
	}

	void UIUserList::update()
	{
		UIElement::update();

		if (tab == Buttons::BT_TAB_FRIEND)
			for (auto sprite : friend_sprites)
				sprite.update();

		if (tab == Buttons::BT_TAB_BOSS)
			for (auto sprite : boss_sprites)
				sprite.update();

		// Refresh party member data
		if (tab == Buttons::BT_TAB_PARTY && party_tab == Buttons::BT_TAB_PARTY_MINE)
		{
			Party& party = Stage::get().get_player().get_party();
			const auto& members = party.get_members();

			party_members.clear();
			party_members.assign(members.begin(), members.end());

			for (size_t i = 0; i < MAX_PARTY_MEMBERS; i++)
			{
				if (i < party_members.size())
				{
					const PartyMember& member = party_members[i];
					party_member_names[i].change_text(member.name);
					party_member_levels[i].change_text(std::to_string(member.level));
					party_member_jobs[i].change_text(Job(member.job).get_name());
				}
				else
				{
					party_member_names[i].change_text("");
					party_member_levels[i].change_text("");
					party_member_jobs[i].change_text("");
				}
			}

			// Update party button states based on whether we're in a party
			if (party.is_in_party())
			{
				buttons[Buttons::BT_PARTY_CREATE]->set_state(Button::State::DISABLED);
				buttons[Buttons::BT_PARTY_INVITE]->set_state(Button::State::NORMAL);
				buttons[Buttons::BT_PARTY_LEAVE]->set_active(true);
				buttons[Buttons::BT_PARTY_SETTINGS]->set_active(true);
			}
			else
			{
				buttons[Buttons::BT_PARTY_CREATE]->set_state(Button::State::NORMAL);
				buttons[Buttons::BT_PARTY_INVITE]->set_state(Button::State::DISABLED);
				buttons[Buttons::BT_PARTY_LEAVE]->set_active(false);
				buttons[Buttons::BT_PARTY_SETTINGS]->set_active(false);
			}
		}
	}

	Cursor::State UIUserList::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);
		if (dragged)
			return dstate;

		hovered_buddy_cid = 0;
		if (tab == Buttons::BT_TAB_FRIEND)
		{
			for (const auto& gh : buddy_arrow_hits)
				if (gh.second.contains(cursorpos))
				{
					if (clicked)
						buddy_group_expanded[gh.first] = !buddy_group_expanded[gh.first];
					return clicked ? Cursor::State::CLICKING : Cursor::State::CANCLICK;
				}

			for (const auto& bh : buddy_row_hits)
				if (bh.rect.contains(cursorpos))
				{
					hovered_buddy_cid = bh.cid;
					return Cursor::State::CANCLICK;
				}
		}

		// Re-pin the active main tab and sub-tab to PRESSED — the cursor
		// loop in UIElement::send_cursor flips MOUSEOVER → NORMAL on
		// exit, which would clobber the "I'm the selected tab" highlight
		// every time the cursor moved away.
		buttons[Buttons::BT_TAB_FRIEND + tab]->set_state(Button::State::PRESSED);
		if (tab == Buttons::BT_TAB_FRIEND)
			buttons[Buttons::BT_TAB_FRIEND + friend_tab]->set_state(Button::State::PRESSED);
		else if (tab == Buttons::BT_TAB_PARTY)
			buttons[Buttons::BT_TAB_FRIEND + party_tab]->set_state(Button::State::PRESSED);
		else if (tab == Buttons::BT_TAB_BLACKLIST)
			buttons[Buttons::BT_TAB_FRIEND + blacklist_tab]->set_state(Button::State::PRESSED);

		// UIDragElement::send_cursor above already ran the button loop
		// via its own tail-call to UIElement::send_cursor. Calling it
		// again would fire button_pressed twice per click — for any
		// TOGGLED popup the second call would immediately re-toggle it
		// back to inactive, so the user never sees anything open.
		return dstate;
	}

	void UIUserList::send_scroll(double yoffset)
	{
		// Global scroll: wheel anywhere over the buddy list area moves
		// the entire stack (group headers + rows). Clamp happens in the
		// draw pass once we know the total height.
		if (tab != Buttons::BT_TAB_FRIEND) return;

		Point<int16_t> cursorpos = UI::get().get_cursor_position();
		if (!buddy_list_rect.contains(cursorpos)) return;

		int16_t step  = 18;
		int16_t delta = static_cast<int16_t>(yoffset * step);
		int16_t next  = static_cast<int16_t>(buddy_list_scroll - delta);

		if (next < 0) next = 0;
		if (next > buddy_list_max) next = buddy_list_max;
		buddy_list_scroll = next;
	}

	void UIUserList::doubleclick(Point<int16_t> cursorpos)
	{
		// Double-click a buddy row → open a whisper window pre-targeted
		// at that buddy. Mirrors the right-click "Whisper" action and
		// the tooltip's "Double-click to send a Whisper" hint.
		if (tab != Buttons::BT_TAB_FRIEND) return;

		const auto& entries = Stage::get().get_player().get_buddylist().get_entries();
		for (const auto& bh : buddy_row_hits)
		{
			if (bh.rect.contains(cursorpos))
			{
				auto it = entries.find(bh.cid);
				if (it == entries.end()) break;
				UI::get().emplace<UIWhisper>(it->second.name);
				break;
			}
		}
	}

	void UIUserList::rightclick(Point<int16_t> cursorpos)
	{
		if (tab == Buttons::BT_TAB_PARTY)
		{
			Player& me = Stage::get().get_player();
			Party& party = me.get_party();
			int32_t my_cid = me.get_oid();
			bool is_leader = party.is_in_party() && party.get_leader() == my_cid;

			for (const auto& ph : party_row_hits)
			{
				if (ph.rect.contains(cursorpos))
				{
					Point<int16_t> row_tl = ph.rect.get_left_top();
					Point<int16_t> spawn(row_tl.x() + 60, row_tl.y() - 4);

					UI::get().emplace<UIPartyMemberMenu>(
						ph.cid, ph.name, spawn,
						is_leader, ph.cid == my_cid);
					break;
				}
			}
			return;
		}

		if (tab != Buttons::BT_TAB_FRIEND) return;

		const auto& entries = Stage::get().get_player().get_buddylist().get_entries();
		for (const auto& bh : buddy_row_hits)
		{
			if (bh.rect.contains(cursorpos))
			{
				hovered_buddy_cid = bh.cid;

				auto it = entries.find(bh.cid);
				if (it == entries.end()) break;

				// Measure the displayed name (memo if set, else real
				// name) so the menu spawns just past the text instead
				// of under the cursor.
				const std::string& memo = BuddyMemoStore::get().get_memo(bh.cid);
				std::string display = memo.empty() ? it->second.name : memo;
				buddy_row_name.change_text(display);
				int16_t name_w = buddy_row_name.width();

				// Buddy name is drawn at bh.rect.left + 8 (right_sub_x+8).
				// Menu spawn = name_end + small gap, row top y.
				Point<int16_t> row_tl = bh.rect.get_left_top();
				Point<int16_t> spawn(
					row_tl.x() + 8 + name_w + 4,
					row_tl.y() - 4);

				// All sub-tab → trim the menu to just Memo + Delete.
				// Online sub-tab → full action menu.
				bool online_actions = (friend_tab == Tab::FRIEND_ONLINE);

				UI::get().emplace<UIBuddyContextMenu>(
					bh.cid, it->second.name, spawn, online_actions);
				break;
			}
		}
	}

	void UIUserList::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				deactivate();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				uint16_t new_tab = tab;

				if (new_tab < Buttons::BT_TAB_BLACKLIST)
					new_tab++;
				else
					new_tab = Buttons::BT_TAB_FRIEND;

				change_tab(new_tab);
			}
		}
	}

	UIElement::Type UIUserList::get_type() const
	{
		return TYPE;
	}

	Button::State UIUserList::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::BT_CLOSE:
				deactivate();
				break;
			case Buttons::BT_TAB_FRIEND:
			case Buttons::BT_TAB_PARTY:
			case Buttons::BT_TAB_BOSS:
			case Buttons::BT_TAB_BLACKLIST:
				change_tab(buttonid);
				return Button::State::PRESSED;
			case Buttons::BT_TAB_PARTY_MINE:
			case Buttons::BT_TAB_PARTY_SEARCH:
				change_party_tab(buttonid);
				return Button::State::PRESSED;
			case Buttons::BT_TAB_FRIEND_ALL:
			case Buttons::BT_TAB_FRIEND_ONLINE:
				change_friend_tab(buttonid);
				return Button::State::PRESSED;
			case Buttons::BT_TAB_BLACKLIST_INDIVIDUAL:
			case Buttons::BT_TAB_BLACKLIST_GUILD:
				change_blacklist_tab(buttonid);
				return Button::State::PRESSED;

			// === Friend tab footer ===
			case Buttons::BT_FRIEND_ADD:
				UI::get().emplace<UIAddBuddy>(
					position + Point<int16_t>(dimension.x() + 4, 0));
				break;
			case Buttons::BT_FRIEND_ADD_GROUP:
				UI::get().emplace<UIBuddyGroup>(
					position + Point<int16_t>(dimension.x() + 4, 0));
				break;
			case Buttons::BT_FRIEND_EXPAND:
				// Slot expansion is a Cash item on retail; not exposed
				// in Cosmic v83.
				UI::get().emplace<UIOk>(
					"Buddy list expansion is not available on this server.",
					[](bool) {});
				break;

			// === Party tab footer ===
			case Buttons::BT_PARTY_CREATE:
				CreatePartyPacket().dispatch();
				break;
			case Buttons::BT_PARTY_INVITE:
				UI::get().emplace<UIPartyInvite>();
				break;
			case Buttons::BT_PARTY_LEAVE:
				UI::get().emplace<UIYesNo>(
					"Are you sure you want to leave the party?",
					[](bool yes)
					{
						if (yes)
							LeavePartyPacket().dispatch();
					});
				break;
			case Buttons::BT_PARTY_SETTINGS:
				UI::get().emplace<UIPartySettings>(false /* settings, not make */);
				break;

			// === Blacklist tab footer ===
			case Buttons::BT_BLACKLIST_ADD:
			case Buttons::BT_BLACKLIST_DELETE:
				UI::get().emplace<UIOk>(
					"Block list is not supported on this server.",
					[](bool) {});
				break;

			default:
				return Button::State::NORMAL;
		}

		return Button::State::NORMAL;
	}

	void UIUserList::change_tab(uint8_t tabid)
	{
		uint8_t oldtab = tab;
		tab = tabid;

		background = tabid == Buttons::BT_TAB_BOSS ? UserList["Main"]["Boss"]["backgrnd3"] : UserList["Main"]["backgrnd2"];

		if (oldtab != tab)
			buttons[Buttons::BT_TAB_FRIEND + oldtab]->set_state(Button::State::NORMAL);

		buttons[Buttons::BT_TAB_FRIEND + tab]->set_state(Button::State::PRESSED);

		if (tab == Buttons::BT_TAB_PARTY)
		{
			buttons[Buttons::BT_PARTY_CREATE]->set_active(true);
			buttons[Buttons::BT_PARTY_INVITE]->set_active(true);
			buttons[Buttons::BT_TAB_PARTY_MINE]->set_active(true);
			buttons[Buttons::BT_TAB_PARTY_SEARCH]->set_active(true);

			change_party_tab(Tab::PARTY_MINE);
		}
		else
		{
			buttons[Buttons::BT_PARTY_CREATE]->set_active(false);
			buttons[Buttons::BT_PARTY_INVITE]->set_active(false);
			buttons[Buttons::BT_PARTY_LEAVE]->set_active(false);
			buttons[Buttons::BT_PARTY_SETTINGS]->set_active(false);
			buttons[Buttons::BT_TAB_PARTY_MINE]->set_active(false);
			buttons[Buttons::BT_TAB_PARTY_SEARCH]->set_active(false);
			buttons[Buttons::BT_PARTY_SEARCH_LEVEL]->set_active(false);
		}

		if (tab == Buttons::BT_TAB_FRIEND)
		{
			buttons[Buttons::BT_FRIEND_ADD]->set_active(true);
			buttons[Buttons::BT_FRIEND_ADD_GROUP]->set_active(true);
			// Slot expansion is a Cash item on retail; not exposed in
			// Cosmic v83, so the BtPlusFriend button stays hidden.
			buttons[Buttons::BT_FRIEND_EXPAND]->set_active(false);
			buttons[Buttons::BT_TAB_FRIEND_ALL]->set_active(true);
			buttons[Buttons::BT_TAB_FRIEND_ONLINE]->set_active(true);
			// BT_FRIEND_GROUP_0 is the legacy fixed-position arrow that
			// only ever covered the first group — replaced by per-group
			// arrows drawn in the buddy-list loop, so keep it inactive.
			buttons[Buttons::BT_FRIEND_GROUP_0]->set_active(false);

			change_friend_tab(Tab::FRIEND_ALL);
		}
		else
		{
			buttons[Buttons::BT_FRIEND_ADD]->set_active(false);
			buttons[Buttons::BT_FRIEND_ADD_GROUP]->set_active(false);
			buttons[Buttons::BT_FRIEND_EXPAND]->set_active(false);
			buttons[Buttons::BT_TAB_FRIEND_ALL]->set_active(false);
			buttons[Buttons::BT_TAB_FRIEND_ONLINE]->set_active(false);
			buttons[Buttons::BT_FRIEND_GROUP_0]->set_active(false);
		}

		if (tab == Buttons::BT_TAB_BOSS)
		{
			buttons[Buttons::BT_BOSS_0]->set_active(true);
			buttons[Buttons::BT_BOSS_1]->set_active(true);
			buttons[Buttons::BT_BOSS_2]->set_active(true);
			buttons[Buttons::BT_BOSS_3]->set_active(true);
			buttons[Buttons::BT_BOSS_4]->set_active(true);
			buttons[Buttons::BT_BOSS_L]->set_active(true);
			buttons[Buttons::BT_BOSS_R]->set_active(true);
			buttons[Buttons::BT_BOSS_DIFF_L]->set_active(true);
			buttons[Buttons::BT_BOSS_DIFF_R]->set_active(true);
			buttons[Buttons::BT_BOSS_GO]->set_active(true);
			buttons[Buttons::BT_BOSS_L]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_BOSS_R]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_BOSS_GO]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_BOSS_DIFF_L]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_BOSS_DIFF_R]->set_state(Button::State::DISABLED);
		}
		else
		{
			buttons[Buttons::BT_BOSS_0]->set_active(false);
			buttons[Buttons::BT_BOSS_1]->set_active(false);
			buttons[Buttons::BT_BOSS_2]->set_active(false);
			buttons[Buttons::BT_BOSS_3]->set_active(false);
			buttons[Buttons::BT_BOSS_4]->set_active(false);
			buttons[Buttons::BT_BOSS_L]->set_active(false);
			buttons[Buttons::BT_BOSS_R]->set_active(false);
			buttons[Buttons::BT_BOSS_DIFF_L]->set_active(false);
			buttons[Buttons::BT_BOSS_DIFF_R]->set_active(false);
			buttons[Buttons::BT_BOSS_GO]->set_active(false);
		}

		if (tab == Buttons::BT_TAB_BLACKLIST)
		{
			buttons[Buttons::BT_BLACKLIST_ADD]->set_active(true);
			buttons[Buttons::BT_BLACKLIST_DELETE]->set_active(true);
			buttons[Buttons::BT_TAB_BLACKLIST_INDIVIDUAL]->set_active(true);
			buttons[Buttons::BT_TAB_BLACKLIST_GUILD]->set_active(true);

			change_blacklist_tab(Buttons::BT_TAB_BLACKLIST_INDIVIDUAL);
		}
		else
		{
			buttons[Buttons::BT_BLACKLIST_ADD]->set_active(false);
			buttons[Buttons::BT_BLACKLIST_DELETE]->set_active(false);
			buttons[Buttons::BT_TAB_BLACKLIST_INDIVIDUAL]->set_active(false);
			buttons[Buttons::BT_TAB_BLACKLIST_GUILD]->set_active(false);
		}
	}

	uint16_t UIUserList::get_tab()
	{
		return tab;
	}

	void UIUserList::change_party_tab(uint8_t tabid)
	{
		uint8_t oldtab = party_tab;
		party_tab = tabid;

		if (oldtab != party_tab)
			buttons[Buttons::BT_TAB_FRIEND + oldtab]->set_state(Button::State::NORMAL);

		buttons[Buttons::BT_TAB_FRIEND + party_tab]->set_state(Button::State::PRESSED);

		if (party_tab == Buttons::BT_TAB_PARTY_SEARCH)
			buttons[Buttons::BT_PARTY_SEARCH_LEVEL]->set_active(true);
		else
			buttons[Buttons::BT_PARTY_SEARCH_LEVEL]->set_active(false);
	}

	void UIUserList::change_friend_tab(uint8_t tabid)
	{
		uint8_t oldtab = friend_tab;
		friend_tab = tabid;

		if (oldtab != friend_tab)
			buttons[Buttons::BT_TAB_FRIEND + oldtab]->set_state(Button::State::NORMAL);

		buttons[Buttons::BT_TAB_FRIEND + friend_tab]->set_state(Button::State::PRESSED);
	}

	void UIUserList::change_blacklist_tab(uint8_t tabid)
	{
		uint8_t oldtab = blacklist_tab;
		blacklist_tab = tabid;

		if (oldtab != blacklist_tab)
			buttons[Buttons::BT_TAB_FRIEND + oldtab]->set_state(Button::State::NORMAL);

		buttons[Buttons::BT_TAB_FRIEND + blacklist_tab]->set_state(Button::State::PRESSED);
	}

	std::string UIUserList::get_cur_location()
	{
		return "Henesys Market";
	}
}