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

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIMonsterCarnival : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MONSTERCARNIVAL;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIMonsterCarnival();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_cp(int32_t my_cp, int32_t my_total, int32_t enemy_cp, int32_t enemy_total);
		void set_team(int8_t team);
		void set_summon_count(int8_t category, int16_t count);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_SIDE
		};

		Texture icon0, icon1, icon2, icon3;

		int32_t my_cp, my_total_cp;
		int32_t enemy_cp, enemy_total_cp;
		int8_t my_team;
		bool side_view = false;
		int16_t summon_counts[4];

		mutable Text my_cp_label;
		mutable Text enemy_cp_label;
		mutable Text team_label;
	};
}
