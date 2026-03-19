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

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIMinigame : public UIDragElement<PosMINIGAME>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MINIGAME;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIMinigame();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_game_result(int8_t result);
		void set_my_turn(bool turn);
		void set_ready_state(bool host_ready, bool guest_ready);
		void set_scores(int16_t my_wins, int16_t my_ties, int16_t my_losses);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_START,
			BT_EXIT,
			BT_READY,
			BT_DRAW,
			BT_ABSTAIN
		};

		Texture win_tex, lose_tex, draw_tex;
		Texture char_win, char_lose;
		Texture turn_tex;
		Texture ready_on, ready_off;

		bool is_my_turn;
		bool am_host;
		int8_t last_result;

		int16_t my_wins, my_ties, my_losses;

		mutable Text score_label;
		mutable Text turn_label;
		mutable Text status_label;
	};
}
