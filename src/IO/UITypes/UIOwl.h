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
#include "../../Net/InPacket.h"

#include <vector>

namespace ms
{
	// Owl of Minerva (UIWindow.img/itemSearch): search dialog + results table
	class UIOwl : public UIDragElement<PosOWL>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::OWL;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIOwl(int16_t owl_slot, int32_t owl_itemid);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		void doubleclick(Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		void set_top10(const std::vector<int32_t>& itemids);
		void show_results(int32_t itemid, InPacket& recv);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_SEARCH,
			BT_RETRY,
			BT_CANCEL,
			BT_GO,
			BT_PAGE_LEFT,
			BT_PAGE_RIGHT,
			BT_SEARCH2,
			BT_TOP10_GO,
			BT_CATEGORY_GO,
			BT_CLOSE,
			BT_PANEL_CLOSE,
			BT_OK
		};

		struct Result
		{
			std::string owner;
			int32_t mapid;
			std::string desc;
			int32_t bundles;
			int32_t perbundle;
			int32_t price;
			int32_t ownerid;
			int8_t channel;
		};

		void do_search();

		int16_t owl_slot;
		int32_t owl_itemid;

		bool results_mode;
		Texture search_bg;
		Texture result_bg;
		Point<int16_t> search_dim;
		Point<int16_t> result_dim;

		Textfield search_field;
		std::vector<std::pair<std::string, int32_t>> top10;
		std::vector<std::pair<std::string, int32_t>> suggestions;
		std::vector<std::pair<std::string, int32_t>> panel_rows;
		bool panel_cats = false;
		bool sort_by_price = false;
		std::string last_input;
		bool top10_open = false;
		int16_t page = 0;
		int16_t picked = -1;
		int16_t hovered = -1;
		Texture panel_bg;
		Texture check_off;
		Texture check_on;
		Texture info_btn_tex;
		Texture go_btn_tex;
		Texture go_btn_over;
		Texture go_btn_pressed;
		int16_t go_hover = -1;
		int32_t result_itemid;
		std::vector<Result> results;
		int16_t selected;
		int16_t result_offset;

		mutable Text rowtext;
		mutable Text titletext;
	};
}
