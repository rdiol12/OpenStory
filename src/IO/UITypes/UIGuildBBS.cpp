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

#include "../../Net/Packets/SocialPackets.h"

#include <ctime>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuildBBS::UIGuildBBS() : UIDragElement<PosGUILDBBS>(Point<int16_t>(W, H)), current_page(0), total_pages(1)
	{
		nl::node src = nl::nx::ui["GuildBBS.img"]["GuildBBS"];

		sprites.emplace_back(src["backgrnd"]);

		popup_tex = src["backgrnd3"];

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(src["BtQuit"], Point<int16_t>(692, 31));
		buttons[BT_WRITE] = std::make_unique<MapleButton>(src["BtWrite"], Point<int16_t>(50, 481));
		buttons[BT_NOTICE] = std::make_unique<MapleButton>(src["BtNotice"], Point<int16_t>(115, 481));
		buttons[BT_DELETE] = std::make_unique<MapleButton>(src["BtDelete"], Point<int16_t>(180, 481));
		buttons[BT_REPLY] = std::make_unique<MapleButton>(src["BtReply"], Point<int16_t>(702, 456));
		buttons[BT_REGISTER] = std::make_unique<MapleButton>(src["BtRegister"], Point<int16_t>(330, 356));
		buttons[BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"], Point<int16_t>(400, 356));

		buttons[BT_REGISTER]->set_active(false);
		buttons[BT_CANCEL]->set_active(false);

		subject_field = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(POPUP_X + 56, POPUP_Y + 12),
				Point<int16_t>(POPUP_X + 320, POPUP_Y + 28)
			),
			25
		);

		content_field = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(POPUP_X + 56, POPUP_Y + 40),
				Point<int16_t>(POPUP_X + 318, POPUP_Y + 205)
			),
			600
		);
		content_field.set_wrap(255);

		reply_field = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(430, 458),
				Point<int16_t>(695, 476)
			),
			25
		);
		reply_field.set_enter_callback(
			[this](std::string)
			{
				button_pressed(BT_REPLY);
			}
		);

		page_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "1 / 1");
		pane_hint = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DUSTYGRAY, "Select a post on the left to read it here.");
		notice_hint = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::ORANGE, "This post will be pinned as the guild notice.");
		post_title_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);
		post_author_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);
		post_date_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);
		body_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "", 310);
		empty_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::LIGHTGREY, "No posts yet.");

		dimension = Point<int16_t>(W, H);

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

		// --- Thread list (left table) ---
		if (posts.empty())
		{
			empty_text.draw(position + Point<int16_t>(197, LIST_Y + 80));
		}
		else
		{
			int16_t visible_count = static_cast<int16_t>(posts.size());

			if (visible_count > MAX_VISIBLE_POSTS)
				visible_count = MAX_VISIBLE_POSTS;

			for (int16_t i = 0; i < visible_count; i++)
			{
				const auto& post = posts[i];
				int16_t row_y = LIST_Y + i * ROW_H + 8;
				bool selected = viewing && post.id == view_id;

				post_title_label.change_color(selected ? Color::Name::ORANGE : Color::Name::WHITE);
				post_title_label.change_text(
					post.is_notice ? "[Notice] " + post.title : post.title);
				post_title_label.draw(position + Point<int16_t>(20, row_y));

				post_author_label.change_color(selected ? Color::Name::ORANGE : Color::Name::WHITE);
				post_author_label.change_text(post.author);
				post_author_label.draw(position + Point<int16_t>(285, row_y));

				post_date_label.change_color(selected ? Color::Name::ORANGE : Color::Name::WHITE);
				post_date_label.change_text(post.date);
				post_date_label.draw(position + Point<int16_t>(352, row_y));
			}
		}

		page_text.change_text(std::to_string(current_page + 1) + " / " + std::to_string(total_pages));
		page_text.draw(position + Point<int16_t>(255, 481));

		// --- Opened thread (right pane) ---
		if (!viewing && !writing)
			pane_hint.draw(position + Point<int16_t>(560, 140));

		if (viewing)
		{
			post_title_label.change_color(Color::Name::BLACK);
			post_title_label.change_text(view_title);
			post_title_label.draw(position + Point<int16_t>(PANE_X, PANE_Y));

			post_author_label.change_color(Color::Name::DUSTYGRAY);
			post_author_label.change_text(view_author + "  " + view_date);
			post_author_label.draw(position + Point<int16_t>(PANE_X + 90, PANE_Y + 18));

			body_text.change_text(view_content);
			body_text.draw(position + Point<int16_t>(PANE_X, PANE_Y + 40));

			// Replies in the baked rows bottom right
			int16_t shown = static_cast<int16_t>(view_replies.size());

			if (shown > MAX_VISIBLE_REPLIES)
				shown = MAX_VISIBLE_REPLIES;

			int16_t start = static_cast<int16_t>(view_replies.size()) - shown;

			for (int16_t i = 0; i < shown; i++)
			{
				const auto& reply = view_replies[start + i];
				int16_t row_y = REPLY_Y + i * REPLY_ROW_H;

				post_author_label.change_color(Color::Name::WHITE);
				post_author_label.change_text(reply.author);
				post_author_label.draw(position + Point<int16_t>(REPLY_X + 35, row_y));

				body_text.change_text(reply.content);
				body_text.draw(position + Point<int16_t>(REPLY_X + 80, row_y));
			}
		}

		// --- Write popup ---
		if (writing)
		{
			popup_tex.draw(DrawArgument(position + Point<int16_t>(POPUP_X, POPUP_Y) + popup_tex.get_origin()));

			if (writing_notice)
				notice_hint.draw(position + Point<int16_t>(POPUP_X + 164, POPUP_Y - 18));

			subject_field.draw(position, Point<int16_t>(0, -5));
			content_field.draw(position, Point<int16_t>(0, -5));
		}
		else
		{
			reply_field.draw(position, Point<int16_t>(0, -5));
		}

		UIElement::draw_buttons(inter);
	}

	void UIGuildBBS::update()
	{
		UIElement::update();

		subject_field.update(position);
		content_field.update(position);
		reply_field.update(position);
	}

	bool UIGuildBBS::indragrange(Point<int16_t> cursorpos) const
	{
		// Drag by the title bar only — the rest of the window is content
		Rectangle<int16_t> bar(position, position + Point<int16_t>(W, 48));

		return bar.contains(cursorpos);
	}

	void UIGuildBBS::set_write_mode(bool on, bool notice)
	{
		writing = on;
		writing_notice = notice;

		buttons[BT_REGISTER]->set_active(on);
		buttons[BT_CANCEL]->set_active(on);
		buttons[BT_WRITE]->set_active(!on);
		buttons[BT_NOTICE]->set_active(!on);
		buttons[BT_DELETE]->set_active(!on);
		buttons[BT_REPLY]->set_active(!on);

		if (on)
		{
			subject_field.change_text("");
			content_field.change_text("");
			subject_field.set_state(Textfield::State::FOCUSED);
		}
		else
		{
			subject_field.set_state(Textfield::State::NORMAL);
			content_field.set_state(Textfield::State::NORMAL);
		}
	}

	void UIGuildBBS::submit_post()
	{
		std::string title = subject_field.get_text();
		std::string content = content_field.get_text();

		if (title.empty() || content.empty())
		{
			UI::get().emplace<UIOk>("Please enter both a subject and content.", [](bool) {});
			return;
		}

		GuildBBSWritePacket(false, 0, writing_notice, title, content).dispatch();
		set_write_mode(false, false);
		request_list();
	}

	void UIGuildBBS::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (writing)
				set_write_mode(false, false);
			else
				deactivate();
		}
	}

	Cursor::State UIGuildBBS::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> local = cursorpos - position;

		if (writing)
		{
			if (clicked)
			{
				Rectangle<int16_t> subject_rect(
					Point<int16_t>(POPUP_X + 50, POPUP_Y + 8),
					Point<int16_t>(POPUP_X + 324, POPUP_Y + 32)
				);
				Rectangle<int16_t> content_rect(
					Point<int16_t>(POPUP_X + 50, POPUP_Y + 36),
					Point<int16_t>(POPUP_X + 320, POPUP_Y + 210)
				);

				if (subject_rect.contains(local))
				{
					content_field.set_state(Textfield::State::NORMAL);
					subject_field.set_state(Textfield::State::FOCUSED);
					return Cursor::State::CLICKING;
				}

				if (content_rect.contains(local))
				{
					subject_field.set_state(Textfield::State::NORMAL);
					content_field.set_state(Textfield::State::FOCUSED);
					return Cursor::State::CLICKING;
				}
			}

			return UIElement::send_cursor(clicked, cursorpos);
		}

		if (clicked)
		{
			// Reply input focus
			Rectangle<int16_t> reply_rect(
				Point<int16_t>(422, 452),
				Point<int16_t>(700, 480)
			);

			if (viewing && reply_rect.contains(local))
			{
				reply_field.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			if (reply_field.get_state() == Textfield::State::FOCUSED)
				reply_field.set_state(Textfield::State::NORMAL);

			// Clicking a row in the list opens the thread in the right pane
			if (local.x() >= 12 && local.x() <= 383)
			{
				int16_t row = (local.y() - LIST_Y) / ROW_H;

				if (local.y() >= LIST_Y && row >= 0
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

	Button::State UIGuildBBS::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			deactivate();
			break;
		case BT_WRITE:
			set_write_mode(true, false);
			break;
		case BT_NOTICE:
			set_write_mode(true, true);
			break;
		case BT_DELETE:
			if (viewing)
			{
				GuildBBSDeletePacket(view_id).dispatch();
				viewing = false;
				request_list();
			}
			break;
		case BT_REGISTER:
			submit_post();
			break;
		case BT_CANCEL:
			set_write_mode(false, false);
			break;
		case BT_REPLY:
		{
			std::string reply = reply_field.get_text();

			if (viewing && !reply.empty())
			{
				GuildBBSReplyPacket(view_id, reply).dispatch();
				reply_field.change_text("");
			}

			break;
		}
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIGuildBBS::start_post_list(int32_t thread_count)
	{
		posts.clear();

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
