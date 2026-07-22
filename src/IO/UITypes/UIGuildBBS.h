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
#include "../Components/Textfield.h"

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <vector>

namespace ms
{
	// Guild bulletin board (GuildBBS.img/GuildBBS): thread list on the
	// left, the opened thread + its replies on the right, write-post
	// popup (backgrnd3) drawn centered. Wire protocol matches Cosmic's
	// BBSOperationHandler / GuildPackets.
	class UIGuildBBS : public UIDragElement<PosGUILDBBS>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::GUILDBBS;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIGuildBBS();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void start_post_list(int32_t thread_count);
		void set_total_threads(int32_t thread_count);
		void add_post(int32_t id, const std::string& author, const std::string& title,
			int64_t timestamp, int32_t reply_count, bool is_notice);
		void clear_posts();

		void show_thread(int32_t id, const std::string& author, const std::string& title,
			const std::string& content, int64_t timestamp);
		void add_thread_reply(int32_t reply_id, const std::string& author,
			const std::string& content, int64_t timestamp);

		// Formats a Cosmic FILETIME-style long as YYYY-MM-DD.
		static std::string format_date(int64_t timestamp);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		bool indragrange(Point<int16_t> cursorpos) const override;

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_WRITE,
			BT_NOTICE,
			BT_DELETE,
			BT_REPLY,
			BT_REGISTER,
			BT_CANCEL
		};

		struct BBSPost
		{
			int32_t id;
			std::string author;
			std::string title;
			std::string date;
			int32_t reply_count;
			bool is_notice;
		};

		struct BBSReply
		{
			int32_t id;
			std::string author;
			std::string content;
			std::string date;
		};

		static constexpr int16_t W = 731;
		static constexpr int16_t H = 526;

		// Thread list table
		static constexpr int16_t LIST_Y = 85;
		static constexpr int16_t ROW_H = 30;
		static constexpr int16_t MAX_VISIBLE_POSTS = 12;

		// Right-hand reading pane
		static constexpr int16_t PANE_X = 400;
		static constexpr int16_t PANE_Y = 15;

		// Replies panel
		static constexpr int16_t REPLY_X = 403;
		static constexpr int16_t REPLY_Y = 330;
		static constexpr int16_t REPLY_ROW_H = 30;
		static constexpr int16_t MAX_VISIBLE_REPLIES = 4;

		// Write popup (backgrnd3, 328x254, centered)
		static constexpr int16_t POPUP_X = 202;
		static constexpr int16_t POPUP_Y = 119;

		void request_list() const;
		void set_write_mode(bool on, bool notice);
		void submit_post();

		std::vector<BBSPost> posts;

		// Opened-thread state shown in the right pane
		bool viewing = false;
		int32_t view_id = 0;
		std::string view_author;
		std::string view_title;
		std::string view_content;
		std::string view_date;
		std::vector<BBSReply> view_replies;

		// Write-popup state
		bool writing = false;
		bool writing_notice = false;
		Texture popup_tex;
		Textfield subject_field;
		Textfield content_field;
		Textfield reply_field;

		// Pagination
		int16_t current_page;
		int16_t total_pages;

		mutable Text page_text;
		mutable Text pane_hint;
		mutable Text notice_hint;
		mutable Text post_title_label;
		mutable Text post_author_label;
		mutable Text post_date_label;
		mutable Text body_text;
		mutable Text empty_text;
	};
}
