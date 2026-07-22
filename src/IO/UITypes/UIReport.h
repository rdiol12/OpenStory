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
	class UIReport : public UIDragElement<PosREPORT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::REPORT;
		static constexpr bool FOCUSED = true;
		static constexpr bool TOGGLED = true;

		UIReport(const std::string& target = "");

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void set_target(const std::string& name);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		bool indragrange(Point<int16_t> cursorpos) const override;

		void open_name_list();
		void open_reason_list();
		void close_list();
		void send_report();

		enum Buttons : uint16_t
		{
			BT_SEND,
			BT_CANCEL
		};

		enum class DropList : uint8_t
		{
			NONE,
			NAME,
			REASON
		};

		static constexpr int16_t W = 266;
		static constexpr int16_t H = 302;

		// Box geometry (window-local, from UIWindow2 Claim/report art)
		static constexpr int16_t BOX_W = 144;
		static constexpr int16_t BOX_H = 18;
		static constexpr int16_t NB_X = 110;
		static constexpr int16_t NB_Y = 143;
		static constexpr int16_t DD_X = 110;
		static constexpr int16_t DD_Y = 163;
		static constexpr int16_t ROW_H = 17;

		static constexpr int NUM_REASONS = 5;
		static constexpr const char* REASON_LABELS[NUM_REASONS] = {
			"Hacking/Botting",
			"Scamming",
			"Bad Name",
			"Harassment",
			"Other"
		};

		std::string target_name;
		Textfield namefield;
		Textfield descfield;
		int16_t selected_reason;
		Text reason_text;

		DropList open_list;
		Point<int16_t> list_pos;
		std::vector<std::string> list_values;
		std::vector<Text> list_texts;
		int16_t hovered_option;
		Texture dd_box;
	};
}
