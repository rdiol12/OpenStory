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

#include "../../Graphics/Text.h"

#include <vector>

namespace ms
{
	// Guild bulletin board (GuildBBS.img/GuildBBS). Two views:
	// the thread list, and an opened thread with its replies.
	// Wire protocol matches Cosmic's BBSOperationHandler / GuildPackets.
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
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_WRITE,
			BT_DELETE,
			BT_REPLY
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

		void request_list() const;
		void open_write_dialog();
		void open_reply_dialog();

		std::vector<BBSPost> posts;

		// Opened-thread state; viewing == true switches draw to thread view.
		bool viewing = false;
		int32_t view_id = 0;
		std::string view_author;
		std::string view_title;
		std::string view_content;
		std::string view_date;
		std::vector<BBSReply> view_replies;

		// Pagination
		int16_t current_page;
		int16_t total_pages;

		// Text labels
		mutable Text title_text;
		mutable Text page_text;
		mutable Text post_title_label;
		mutable Text post_author_label;
		mutable Text post_date_label;
		mutable Text body_text;
		mutable Text empty_text;

		// Post list area
		Texture list_backgrnd;

		// List display constants
		static constexpr int16_t MAX_VISIBLE_POSTS = 10;
		static constexpr int16_t POST_ROW_HEIGHT = 20;
		static constexpr int16_t POST_LIST_Y = 60;
	};
}
