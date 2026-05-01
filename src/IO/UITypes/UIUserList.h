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
#pragma once

#include "../UIDragElement.h"

#include "../Components/MapleFrame.h"
#include "../Components/Slider.h"
#include "../Components/Textfield.h"

#include "../../Character/Party.h"

namespace ms
{
	class UIUserList : public UIDragElement<PosUSERLIST>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::USERLIST;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIUserList(uint16_t tab);

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		void rightclick(Point<int16_t> cursorpos) override;
		void doubleclick(Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		enum Tab
		{
			FRIEND,
			PARTY,
			BOSS,
			BLACKLIST,
			PARTY_MINE,
			PARTY_SEARCH,
			FRIEND_ALL,
			FRIEND_ONLINE
		};

		void change_tab(uint8_t tabid);
		uint16_t get_tab();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_party_tab(uint8_t tabid);
		void change_friend_tab(uint8_t tabid);
		void change_blacklist_tab(uint8_t tabid);
		std::string get_cur_location();

		enum Buttons
		{
			BT_TAB_FRIEND,
			BT_TAB_PARTY,
			BT_TAB_BOSS,
			BT_TAB_BLACKLIST,
			BT_TAB_PARTY_MINE,
			BT_TAB_PARTY_SEARCH,
			BT_TAB_FRIEND_ALL,
			BT_TAB_FRIEND_ONLINE,
			BT_TAB_BLACKLIST_INDIVIDUAL,
			BT_TAB_BLACKLIST_GUILD,
			BT_CLOSE,
			BT_PARTY_CREATE,
			BT_PARTY_INVITE,
			BT_PARTY_LEAVE,
			BT_PARTY_SETTINGS,
			BT_PARTY_SEARCH_LEVEL,
			BT_PARTY_SEARCH_INVITE,
			BT_FRIEND_ADD,
			BT_FRIEND_ADD_GROUP,
			BT_FRIEND_EXPAND,
			BT_FRIEND_GROUP_0,
			BT_BOSS_0,
			BT_BOSS_1,
			BT_BOSS_2,
			BT_BOSS_3,
			BT_BOSS_4,
			BT_BOSS_L,
			BT_BOSS_R,
			BT_BOSS_DIFF_L,
			BT_BOSS_DIFF_R,
			BT_BOSS_GO,
			BT_BLACKLIST_ADD,
			BT_BLACKLIST_DELETE
		};

		uint16_t tab;
		nl::node UserList;
		Texture background;

		// Party tab
		uint16_t party_tab;
		Texture party_title;
		Texture party_mine_grid[5];
		Texture party_search_grid[3];
		Text party_mine_name;
		Slider party_slider;

		// Party member display
		static const size_t MAX_PARTY_MEMBERS = 6;
		Texture party_member_row[MAX_PARTY_MEMBERS];
		Texture party_icon_online;
		Texture party_icon_offline;
		Texture party_leader_star;          // 9x9 ★ stamped on the leader row

		// Party Search sub-tab — PartyInfo row sprites + per-row
		// Request button. Each visible row mirrors one pending party
		// invite from NotificationCenter (Cosmic doesn't ship a list
		// of recruiting parties, so we surface what we already have).
		Texture party_search_info_header;   // PartyInfo/0
		Texture party_search_info_row_a;    // PartyInfo/1
		Texture party_search_info_row_b;    // PartyInfo/2
		Texture party_search_info_sep;      // PartyInfo/3
		Texture party_search_request_tex;   // BtRequest normal sprite
		mutable Text party_search_row_label;
		struct SearchRowHit
		{
			int32_t notif_id;
			Rectangle<int16_t> request_hit;
		};
		mutable std::vector<SearchRowHit> party_search_hits;

		std::vector<PartyMember> party_members;
		Text party_member_names[MAX_PARTY_MEMBERS];
		Text party_member_levels[MAX_PARTY_MEMBERS];
		Text party_member_jobs[MAX_PARTY_MEMBERS];

		// Buddy tab
		uint16_t friend_tab;
		int friend_count = 0;
		int friend_total = 50;
		std::vector<Sprite> friend_sprites;
		Texture friend_grid[4];
		mutable Text friends_online_text;
		Text friends_cur_location;
		Text friends_name;
		Text friends_group_name;
		Slider friends_slider;

		// Reusable per-row labels for the buddy tab.
		mutable Text buddy_row_name;
		mutable Text buddy_row_status;
		mutable Text buddy_group_header;
		mutable Text buddy_account_label;

		// Group expansion state (group name → expanded?). Group folders
		// default expanded the first time they appear.
		mutable std::map<std::string, bool> buddy_group_expanded;

		// Hitboxes refreshed every draw() so cursor handling can map a
		// click to the right buddy / arrow.
		struct BuddyRowHit
		{
			int32_t cid;
			Rectangle<int16_t> rect;
		};
		mutable std::vector<BuddyRowHit> buddy_row_hits;
		mutable std::vector<std::pair<std::string, Rectangle<int16_t>>>
			buddy_arrow_hits;
		mutable int32_t hovered_buddy_cid = 0;

		// Party member row hits — refreshed each draw so right-click can
		// open UIPartyMemberMenu over the right row.
		struct PartyRowHit
		{
			int32_t cid;
			std::string name;
			Rectangle<int16_t> rect;
		};
		mutable std::vector<PartyRowHit> party_row_hits;

		Texture buddy_arrow_open;
		Texture buddy_arrow_closed;

		// Hover tooltip backdrop — UIToolTip.img/Item/Frame2 (the same
		// 9-slice frame TextTooltip uses for item hovers).
		MapleFrame tooltip_frame;
		Texture    tooltip_cover;
		// Pre-allocated tooltip lines reused across frames so hovering
		// doesn't churn 4-5 fresh Text objects on every draw call.
		mutable Text tt_channel;
		mutable Text tt_whisper_hint;
		mutable Text tt_rclick_hint;
		mutable Text tt_status;
		mutable Text tt_memo;

		// Per-group, per-side scroll offsets. Kept around so legacy
		// rect plumbing still compiles; the live behaviour is now a
		// single global scroll for the entire buddy list.
		struct GroupScroll
		{
			int16_t left  = 0;
			int16_t right = 0;
			Rectangle<int16_t> left_rect;
			Rectangle<int16_t> right_rect;
		};
		mutable std::map<std::string, GroupScroll> buddy_group_scroll;

		// Global scroll for the buddy list. Wheel anywhere over the
		// list area shifts everything — group headers, rows, dividers
		// — so content past the location-text floor becomes reachable
		// instead of being clipped.
		mutable int16_t buddy_list_scroll  = 0;
		mutable int16_t buddy_list_max     = 0;
		mutable Rectangle<int16_t> buddy_list_rect;

		// Boss tab
		std::vector<Sprite> boss_sprites;

		// Blacklist tab
		uint16_t blacklist_tab = 8; // BT_TAB_BLACKLIST_INDIVIDUAL — must
		                             // be a valid button id from the
		                             // start, otherwise tab re-pin reads
		                             // garbage memory and crashes when
		                             // the user enters the Blacklist tab.
		Texture blacklist_title;
		Texture blacklist_grid[3];
		Text blacklist_name;
	};
}