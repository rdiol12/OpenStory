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

namespace ms
{
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
		void add_post(int32_t id, const std::string& author, const std::string& title, int64_t timestamp, int32_t reply_count);
		void clear_posts();

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

		// Post list display
		struct BBSPost
		{
			int32_t id;
			std::string author;
			std::string title;
			std::string date;
			int32_t reply_count;
		};

		std::vector<BBSPost> posts;

		// Pagination
		int16_t current_page;
		int16_t total_pages;

		// Text labels
		mutable Text title_text;
		mutable Text page_text;
		mutable Text post_title_label;
		mutable Text post_author_label;
		mutable Text post_date_label;
		mutable Text empty_text;

		// Post list area
		Texture list_backgrnd;

		// List display constants
		static constexpr int16_t MAX_VISIBLE_POSTS = 10;
		static constexpr int16_t POST_ROW_HEIGHT = 20;
		static constexpr int16_t POST_LIST_Y = 60;
	};
}
