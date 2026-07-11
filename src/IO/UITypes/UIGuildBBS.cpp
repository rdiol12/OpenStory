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
#include "UIGuildBBS.h"

#include "UINotice.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#include "../../Configuration.h"
#include "../../Net/Packets/SocialPackets.h"

#include <ctime>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuildBBS::UIGuildBBS() : UIDragElement<PosGUILDBBS>(Point<int16_t>(300, 20)), current_page(0), total_pages(1)
	{
		nl::node src = nl::nx::ui["GuildBBS.img"]["GuildBBS"];

		nl::node backgrnd = src["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(src["backgrnd2"]);
		sprites.emplace_back(src["backgrnd3"]);

		// Close button (BtQuit in NX)
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtQuit"]);

		// Action buttons
		buttons[Buttons::BT_WRITE] = std::make_unique<MapleButton>(src["BtWrite"]);
		buttons[Buttons::BT_DELETE] = std::make_unique<MapleButton>(src["BtDelete"]);
		buttons[Buttons::BT_REPLY] = std::make_unique<MapleButton>(src["BtReply"]);

		// Initialize text labels
		title_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Guild Board");
		page_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "1 / 1");
		post_title_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		post_author_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
		post_date_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::DUSTYGRAY);
		body_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "", 250);
		empty_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DUSTYGRAY, "No posts yet.");

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);

		request_list();
	}

	void UIGuildBBS::request_list() const
	{
		GuildBBSListPacket(current_page).dispatch();
	}

	std::string UIGuildBBS::format_date(int64_t timestamp)
	{
		// Cosmic sends unix_ms * 10000 + FILETIME offset (see
		// PacketCreator.getTime). Undo that to get a calendar date.
		constexpr int64_t FT_UT_OFFSET = 116444736010800000LL;

		if (timestamp <= FT_UT_OFFSET)
			return "";

		time_t unix_s = static_cast<time_t>((timestamp - FT_UT_OFFSET) / 10000 / 1000);

		struct tm timeinfo;
		if (localtime_s(&timeinfo, &unix_s) != 0)
			return "";

		char buf[16];
		strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);

		return buf;
	}

	void UIGuildBBS::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		title_text.draw(position + Point<int16_t>(dimension.x() / 2, 28));

		list_backgrnd.draw(DrawArgument(position));

		if (viewing)
		{
			// --- Thread view ---
			post_title_label.change_text(view_title);
			post_title_label.draw(position + Point<int16_t>(15, POST_LIST_Y));

			post_author_label.change_text(view_author);
			post_author_label.draw(position + Point<int16_t>(15, POST_LIST_Y + 18));

			post_date_label.change_text(view_date);
			post_date_label.draw(position + Point<int16_t>(dimension.x() - 15, POST_LIST_Y + 18));

			body_text.change_text(view_content);
			body_text.draw(position + Point<int16_t>(15, POST_LIST_Y + 40));

			// Replies below the body.
			int16_t reply_y = POST_LIST_Y + 48 + body_text.height();
			for (const auto& reply : view_replies)
			{
				if (reply_y > dimension.y() - 60)
					break;

				post_author_label.change_text(reply.author + ":");
				post_author_label.draw(position + Point<int16_t>(20, reply_y));

				post_date_label.change_text(reply.date);
				post_date_label.draw(position + Point<int16_t>(dimension.x() - 15, reply_y));

				body_text.change_text(reply.content);
				body_text.draw(position + Point<int16_t>(30, reply_y + 16));

				reply_y += 20 + body_text.height();
			}

			page_text.change_text("Press ESC for the post list");
			page_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 30));
		}
		else
		{
			// --- Thread list ---
			if (posts.empty())
			{
				empty_text.draw(position + Point<int16_t>(dimension.x() / 2, POST_LIST_Y + 60));
			}
			else
			{
				int16_t visible_count = static_cast<int16_t>(posts.size());
				if (visible_count > MAX_VISIBLE_POSTS)
					visible_count = MAX_VISIBLE_POSTS;

				for (int16_t i = 0; i < visible_count; i++)
				{
					const auto& post = posts[i];
					int16_t row_y = POST_LIST_Y + i * POST_ROW_HEIGHT;

					post_title_label.change_text(
						post.is_notice ? "[Notice] " + post.title : post.title);
					post_title_label.draw(position + Point<int16_t>(15, row_y + 2));

					post_author_label.change_text(post.author);
					post_author_label.draw(position + Point<int16_t>(180, row_y + 2));

					std::string right = post.date;
					if (post.reply_count > 0)
						right += " (" + std::to_string(post.reply_count) + ")";
					post_date_label.change_text(right);
					post_date_label.draw(position + Point<int16_t>(dimension.x() - 15, row_y + 2));
				}
			}

			page_text.change_text(std::to_string(current_page + 1) + " / " + std::to_string(total_pages));
			page_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 30));
		}

		UIElement::draw_buttons(inter);
	}

	void UIGuildBBS::update()
	{
		UIElement::update();
	}

	void UIGuildBBS::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (viewing)
			{
				// Back out of the opened thread to the list.
				viewing = false;
				request_list();
			}
			else
			{
				deactivate();
			}
		}
	}

	Cursor::State UIGuildBBS::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		// Clicking a row in list view opens the thread.
		if (!viewing && clicked)
		{
			Point<int16_t> local = cursorpos - position;

			if (local.x() >= 10 && local.x() <= dimension.x() - 10)
			{
				int16_t row = (local.y() - POST_LIST_Y) / POST_ROW_HEIGHT;

				if (local.y() >= POST_LIST_Y && row >= 0
					&& row < static_cast<int16_t>(posts.size())
					&& row < MAX_VISIBLE_POSTS)
				{
					GuildBBSViewPacket(posts[row].id).dispatch();
					return Cursor::State::CLICKING;
				}
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UIGuildBBS::get_type() const
	{
		return TYPE;
	}

	void UIGuildBBS::open_write_dialog()
	{
		// Title first, then content — chained UIEnterText prompts.
		// Cosmic caps title at 25 chars and body at 600.
		auto title_handler = [](const std::string& title)
		{
			if (title.empty())
				return;

			std::string title_copy = title;
			auto content_handler = [title_copy](const std::string& content)
			{
				if (content.empty())
					return;

				GuildBBSWritePacket(false, 0, false, title_copy, content).dispatch();

				if (auto bbs = UI::get().get_element<UIGuildBBS>())
					GuildBBSListPacket(0).dispatch();
			};

			UI::get().emplace<UIEnterText>("Post content:", content_handler, 600);
		};

		UI::get().emplace<UIEnterText>("Post title:", title_handler, 25);
	}

	void UIGuildBBS::open_reply_dialog()
	{
		int32_t thread_id = view_id;

		auto reply_handler = [thread_id](const std::string& content)
		{
			if (content.empty())
				return;

			GuildBBSReplyPacket(thread_id, content).dispatch();

			// The server responds with a fresh showThread for this id.
		};

		// Cosmic caps replies at 25 characters.
		UI::get().emplace<UIEnterText>("Reply:", reply_handler, 25);
	}

	Button::State UIGuildBBS::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_WRITE:
			open_write_dialog();
			break;
		case Buttons::BT_DELETE:
			if (viewing)
			{
				GuildBBSDeletePacket(view_id).dispatch();
				viewing = false;
				request_list();
			}
			else
			{
				chat::log("Open a post first, then press Delete to remove it.", chat::LineType::YELLOW);
			}
			break;
		case Buttons::BT_REPLY:
			if (viewing)
				open_reply_dialog();
			else
				chat::log("Open a post first, then press Reply.", chat::LineType::YELLOW);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIGuildBBS::start_post_list(int32_t thread_count)
	{
		posts.clear();
		viewing = false;

		set_total_threads(thread_count);
	}

	void UIGuildBBS::set_total_threads(int32_t thread_count)
	{
		total_pages = static_cast<int16_t>((thread_count + MAX_VISIBLE_POSTS - 1) / MAX_VISIBLE_POSTS);
		if (total_pages < 1)
			total_pages = 1;
	}

	void UIGuildBBS::add_post(int32_t id, const std::string& author, const std::string& title,
		int64_t timestamp, int32_t reply_count, bool is_notice)
	{
		posts.push_back({ id, author, title, format_date(timestamp), reply_count, is_notice });
	}

	void UIGuildBBS::clear_posts()
	{
		posts.clear();
		current_page = 0;
	}

	void UIGuildBBS::show_thread(int32_t id, const std::string& author, const std::string& title,
		const std::string& content, int64_t timestamp)
	{
		view_id = id;
		view_author = author;
		view_title = title;
		view_content = content;
		view_date = format_date(timestamp);
		view_replies.clear();
		viewing = true;
	}

	void UIGuildBBS::add_thread_reply(int32_t reply_id, const std::string& author,
		const std::string& content, int64_t timestamp)
	{
		view_replies.push_back({ reply_id, author, content, format_date(timestamp) });
	}
}
