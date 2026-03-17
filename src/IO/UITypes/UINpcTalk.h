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

#include "../UIElement.h"

#include "../Components/Slider.h"

#include "../../Graphics/Text.h"

namespace ms
{
	class UINpcTalk : public UIElement
	{
	public:
		enum TalkType : int8_t
		{
			NONE = -1,
			SENDSAY = 0,		// 0 — OK/Next/Prev/NextPrev (determined by style bytes)
			SENDYESNO = 1,		// 1 — Yes/No
			SENDGETTEXT = 2,	// 2 — Text input
			SENDGETNUMBER = 3,	// 3 — Number input
			SENDSIMPLE = 4,		// 4 — Selection list (blue clickable text)
			SENDSTYLE = 7,		// 7 — Style/cosmetic selection
			SENDACCEPTDECLINE = 12,	// 12 (0x0C) — Accept/Decline
			LENGTH = 13
		};

		static constexpr Type TYPE = UIElement::Type::NPCTALK;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = false;

		UINpcTalk();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void change_text(int32_t npcid, int8_t msgtype, int16_t style, int8_t speaker, const std::string& text);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		bool is_valid_type(int8_t value);
		std::string format_text(const std::string& tx, const int32_t& npcid);
		std::string parse_simple_selections(const std::string& tx);

		static constexpr int16_t MAX_HEIGHT = 248;

		enum Buttons
		{
			CLOSE,
			NEXT,
			NO,
			OK,
			PREV,
			QNO,
			QYES,
			YES,
			QSTART,
			QAFTER,
			QCYES,
			QCNO,
			QGIVEUP
		};

		struct Selection
		{
			int32_t index;
			std::string text;
			int16_t line;
		};

		Texture top;
		Texture fill;
		Texture bottom;
		Texture nametag;
		Texture speaker;
		Texture dot_normal;
		Texture dot_hovered;
		Texture list_normal;
		Texture list_hovered;

		Text text;
		Text name;

		int16_t height;
		int16_t offset;
		int16_t unitrows;
		int16_t rowmax;
		int16_t min_height;

		bool show_slider;
		bool draw_text;
		Slider slider;
		TalkType type;
		std::string formatted_text;
		size_t formatted_text_pos;
		uint16_t timestep;

		std::vector<Selection> selections;
		int16_t selection_top;
		int32_t hovered_selection;

		std::function<void(bool)> onmoved;
	};
}