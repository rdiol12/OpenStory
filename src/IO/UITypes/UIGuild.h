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
#include "../Components/Charset.h"

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <vector>

namespace ms
{
	// Guild window on the GuildUI.img art: flag + name top bar, tabs
	// (Members / Profile / Skills / Board / Alliance), member table
	class UIGuild : public UIDragElement<PosGUILD>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::GUILD;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIGuild();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;

		UIElement::Type get_type() const override;

		// Called from guild packet handler
		void set_guild_info(const std::string& name, const std::string& notice, int16_t level, int16_t capacity);
		void set_notice(const std::string& notice);
		void set_capacity(int16_t capacity);
		void set_gp(int32_t gp);
		void set_rank_titles(const std::string titles[5]);
		void add_member(int32_t cid, const std::string& name, int32_t rank, int16_t level, int16_t job, bool online);
		void remove_member(int32_t cid);
		void update_member_stats(int32_t cid, int16_t level, int16_t job);
		void set_member_online(int32_t cid, bool online);
		void set_member_rank(int32_t cid, int32_t rank);
		std::string get_member_name(int32_t cid) const;
		void clear_members();
		void set_guild_emblem(int16_t bg, int8_t bgcolor, int16_t logo, int8_t logocolor);

		// Guild "level" derived from GP (approximate v83 formula)
		static int16_t level_for_gp(int32_t gp);

		// True once the server delivered real guild info
		bool has_guild() const { return capacity > 0; }

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void change_tab(uint16_t tabid);

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_LEAVE,
			BT_TAB0,
			BT_TAB1,
			BT_TAB2,
			BT_TAB3,
			BT_TAB4
		};

		enum Tabs : uint16_t
		{
			TAB_MEMBERS,
			TAB_PROFILE,
			TAB_SKILLS,
			TAB_BOARD,
			TAB_ALLIANCE
		};

		static constexpr int16_t W = 535;
		static constexpr int16_t H = 391;

		// Tab-page textures carry origins relative to this content anchor
		static constexpr int16_t CONTENT_X = 12;
		static constexpr int16_t CONTENT_Y = 116;

		static constexpr int16_t MAX_VISIBLE_MEMBERS = 10;
		static constexpr int16_t MEMBER_ROW_HEIGHT = 22;
		static constexpr int16_t MEMBER_LIST_Y = 152;

		uint16_t tab;
		int16_t guildlevel;
		int32_t gp_value;

		Texture flags[5];
		Texture nomark;
		Texture cover;
		Texture head_tex;
		Texture row_tex;
		Texture on_tex;
		Texture off_tex;
		Texture bginfo_tex;
		Texture charframe_tex;
		Texture emblem_bg;
		Texture emblem_mark;
		Charset lvnum;

		mutable Text guild_name;
		mutable Text guild_notice;
		mutable Text member_count_text;
		mutable Text cell_text;
		mutable Text value_text;

		struct MemberEntry
		{
			int32_t cid;
			std::string name;
			int32_t rank;
			int16_t level;
			int16_t job;
			bool online;
		};

		MemberEntry* find_member(int32_t cid);
		const std::string& rank_title(int32_t rank) const;
		void refresh_member_count();

		std::vector<MemberEntry> members;
		std::string rank_titles[5];
		int16_t capacity;
		int16_t scroll_offset;
	};
}
