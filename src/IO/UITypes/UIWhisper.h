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
#include "../../Graphics/Geometry.h"

namespace ms
{
	class UIWhisper : public UIDragElement<PosWHISPER>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::WHISPER;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIWhisper(const std::string& target = "");

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void recv_whisper(const std::string& from, const std::string& message, bool from_admin);
		void set_target(const std::string& name);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void send_whisper();
		void update_title();

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_SEND,
			BT_FIND
		};

		static constexpr int16_t WIDTH = 300;
		static constexpr int16_t CHAT_HEIGHT = 150;
		static constexpr int16_t INPUT_HEIGHT = 24;
		static constexpr int16_t HEADER_HEIGHT = 25;
		static constexpr int16_t LINE_HEIGHT = 14;
		static constexpr int16_t MAX_LINES = 50;

		std::string target_name;
		Textfield chatfield;
		Textfield namefield;

		struct ChatLine
		{
			Text text;
			Color::Name color;
		};

		std::vector<ChatLine> chat_lines;
		int16_t scroll_offset;

		ColorBox background;
		ColorBox header_bg;
		ColorBox input_bg;
		Text title_text;

		// Pre-allocated draw objects
		Text close_x;
		ColorBox send_bg;
		Text send_text;
	};
}
