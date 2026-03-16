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

#include "../Components/Charset.h"
#include "../Components/Gauge.h"

#include "../../Character/CharStats.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIStatusBar : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::STATUSBAR;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIStatusBar(const CharStats& stats);

		void draw(float alpha) const override;
		void update() override;

		bool is_in_range(Point<int16_t> cursorpos) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void toggle_qs();
		void toggle_menu();
		void remove_menus();
		bool is_menu_active();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		float getexppercent() const;
		float gethppercent() const;
		float getmppercent() const;

		enum Buttons : uint16_t
		{
			BT_WHISPER,
			BT_CALLGM,
			BT_CASHSHOP,
			BT_TRADE,
			BT_MENU,
			BT_OPTIONS,
			BT_CHARACTER,
			BT_STATS,
			BT_QUEST,
			BT_INVENTORY,
			BT_EQUIPS,
			BT_SKILL
		};

		const CharStats& stats;

		Gauge expbar;
		Gauge hpbar;
		Gauge mpbar;
		Charset statset;
		Charset levelset;
		Text namelabel;
		Text joblabel;
	};
}
