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
	class UIRPSGame : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::RPSGAME;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIRPSGame();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handler to show result
		void show_result(int8_t player_choice, int8_t npc_choice, int8_t result);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void set_phase(int8_t phase);

		enum Buttons : uint16_t
		{
			BT_ROCK,
			BT_PAPER,
			BT_SCISSOR,
			BT_START,
			BT_CONTINUE,
			BT_RETRY,
			BT_EXIT
		};

		enum Phase : int8_t
		{
			WAITING,
			SELECTING,
			RESULT
		};

		Phase current_phase;
		int8_t player_selection;
		int8_t npc_selection;
		int8_t game_result;  // 0=lose, 1=win, 2=draw
		int16_t wins;

		// Hand sprite textures
		Texture rock, paper, scissor;
		Texture frock, fpaper, fscissor;

		// Result textures
		Texture win_tex, lose_tex, draw_tex, timeover_tex;
		Texture char_win, char_lose;

		Text score_label;
		Text result_label;
	};
}
