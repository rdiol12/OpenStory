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
	// Whisper window on the v83 MESSAGE art (UIWindow2.img/MemoInGame/Send):
	// RECIPIENT field, message area showing the conversation with the input
	// line at its bottom, OK sends, CANCEL closes
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
		bool indragrange(Point<int16_t> cursorpos) const override;

		void send_whisper();

		enum Buttons : uint16_t
		{
			BT_SEND,
			BT_CLOSE
		};

		static constexpr int16_t W = 268;
		static constexpr int16_t H = 244;

		// Field geometry (window-local, from the Send art)
		static constexpr int16_t NAME_L = 76;
		static constexpr int16_t NAME_T = 87;
		static constexpr int16_t NAME_R = 252;
		static constexpr int16_t NAME_B = 100;
		static constexpr int16_t AREA_L = 12;
		static constexpr int16_t AREA_T = 136;
		static constexpr int16_t AREA_R = 257;
		static constexpr int16_t AREA_B = 194;
		static constexpr int16_t LINE_Y = 177;
		static constexpr int16_t LINE_HEIGHT = 12;
		static constexpr int16_t SHOWN_LINES = 3;
		static constexpr int16_t MAX_LINES = 50;

		std::string target_name;
		Textfield namefield;
		Textfield chatfield;

		struct ChatLine
		{
			Text text;
			Color::Name color;
		};

		std::vector<ChatLine> chat_lines;
		Texture divider;
	};
}
