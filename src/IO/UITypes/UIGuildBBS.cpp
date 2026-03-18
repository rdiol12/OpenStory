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

#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIGuildBBS::UIGuildBBS() : UIDragElement<PosGUILDBBS>(), current_page(0), total_pages(1)
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["GuildBBS"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Texture bg = backgrnd;

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(src["backgrnd2"]);

		// Close button at top-right
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg.get_dimensions().x() - 19, 5));

		// Action buttons
		buttons[Buttons::BT_WRITE] = std::make_unique<MapleButton>(src["BtWrite"]);
		buttons[Buttons::BT_PREV] = std::make_unique<MapleButton>(src["BtPrev"]);
		buttons[Buttons::BT_NEXT] = std::make_unique<MapleButton>(src["BtNext"]);

		// Optional buttons (may not exist in all NX versions)
		if (src["BtDelete"])
			buttons[Buttons::BT_DELETE] = std::make_unique<MapleButton>(src["BtDelete"]);

		if (src["BtReply"])
			buttons[Buttons::BT_REPLY] = std::make_unique<MapleButton>(src["BtReply"]);

		// List background texture (if available)
		if (src["list"])
			list_backgrnd = src["list"];

		// Initialize text labels
		title_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Guild Board");
		page_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "1 / 1");
		post_title_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		post_author_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
		post_date_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::DUSTYGRAY);
		empty_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DUSTYGRAY, "No posts yet.");

		// Disable prev/next when on first/only page
		if (buttons.count(Buttons::BT_PREV) && buttons[Buttons::BT_PREV])
			buttons[Buttons::BT_PREV]->set_state(Button::State::DISABLED);

		if (buttons.count(Buttons::BT_NEXT) && buttons[Buttons::BT_NEXT])
			buttons[Buttons::BT_NEXT]->set_state(Button::State::DISABLED);

		dimension = bg.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIGuildBBS::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw title at top center
		title_text.draw(position + Point<int16_t>(dimension.x() / 2, 28));

		// Draw list background
		list_backgrnd.draw(DrawArgument(position));

		// Draw posts or empty message
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

				// Draw post title
				post_title_label.change_text(post.title);
				post_title_label.draw(position + Point<int16_t>(15, row_y + 2));

				// Draw post author
				post_author_label.change_text(post.author);
				post_author_label.draw(position + Point<int16_t>(200, row_y + 2));

				// Draw post date
				post_date_label.change_text(post.date);
				post_date_label.draw(position + Point<int16_t>(dimension.x() - 15, row_y + 2));
			}
		}

		// Draw page indicator at the bottom
		page_text.change_text(std::to_string(current_page + 1) + " / " + std::to_string(total_pages));
		page_text.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 30));

		UIElement::draw_buttons(inter);
	}

	void UIGuildBBS::update()
	{
		UIElement::update();
	}

	void UIGuildBBS::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UIGuildBBS::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

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
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_WRITE:
			// TODO: Open write post dialog (requires server packet)
			break;
		case Buttons::BT_PREV:
			if (current_page > 0)
			{
				current_page--;

				// Update prev/next button states
				if (current_page == 0)
				{
					auto it = buttons.find(Buttons::BT_PREV);
					if (it != buttons.end() && it->second)
						it->second->set_state(Button::State::DISABLED);
				}

				auto it = buttons.find(Buttons::BT_NEXT);
				if (it != buttons.end() && it->second)
					it->second->set_state(Button::State::NORMAL);
			}
			break;
		case Buttons::BT_NEXT:
			if (current_page < total_pages - 1)
			{
				current_page++;

				// Update prev/next button states
				if (current_page >= total_pages - 1)
				{
					auto it = buttons.find(Buttons::BT_NEXT);
					if (it != buttons.end() && it->second)
						it->second->set_state(Button::State::DISABLED);
				}

				auto it = buttons.find(Buttons::BT_PREV);
				if (it != buttons.end() && it->second)
					it->second->set_state(Button::State::NORMAL);
			}
			break;
		case Buttons::BT_DELETE:
			// TODO: Delete selected post (requires server packet)
			break;
		case Buttons::BT_REPLY:
			// TODO: Reply to selected post (requires server packet)
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
