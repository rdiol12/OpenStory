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
#include "UIRPSGame.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIRPSGame::UIRPSGame() : UIElement(Point<int16_t>(400, 250), Point<int16_t>(0, 0), ScaleMode::CENTER_OFFSET),
		current_phase(Phase::WAITING), player_selection(-1), npc_selection(-1), game_result(-1), wins(0)
	{
		nl::node main = nl::nx::ui["UIWindow.img"]["RpsGame"];

		nl::node backgrnd = main["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[Buttons::BT_ROCK] = std::make_unique<MapleButton>(main["BtRock"]);
		buttons[Buttons::BT_PAPER] = std::make_unique<MapleButton>(main["BtPaper"]);
		buttons[Buttons::BT_SCISSOR] = std::make_unique<MapleButton>(main["BtScissor"]);
		buttons[Buttons::BT_START] = std::make_unique<MapleButton>(main["BtStart"]);
		buttons[Buttons::BT_CONTINUE] = std::make_unique<MapleButton>(main["BtContinue"]);
		buttons[Buttons::BT_RETRY] = std::make_unique<MapleButton>(main["BtRetry"]);
		buttons[Buttons::BT_EXIT] = std::make_unique<MapleButton>(main["BtExit"]);

		rock = main["rock"];
		paper = main["paper"];
		scissor = main["scissor"];

		frock = main["Frock"];
		fpaper = main["Fpaper"];
		fscissor = main["Fscissor"];

		win_tex = main["win"];
		lose_tex = main["lose"];
		draw_tex = main["draw"];
		timeover_tex = main["timeover"];
		char_win = main["charWin"];
		char_lose = main["charLose"];

		score_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Wins: 0");
		result_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::YELLOW, "");

		set_phase(Phase::WAITING);

		dimension = bg_dimensions;
	}

	void UIRPSGame::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> center = position + Point<int16_t>(dimension.x() / 2, 0);

		if (current_phase == Phase::RESULT)
		{
			// Draw player's hand on left
			const Texture* player_hand = nullptr;

			switch (player_selection)
			{
			case 0: player_hand = &rock; break;
			case 1: player_hand = &paper; break;
			case 2: player_hand = &scissor; break;
			}

			if (player_hand)
				player_hand->draw(position + Point<int16_t>(50, 80));

			// Draw NPC's hand on right
			const Texture* npc_hand = nullptr;

			switch (npc_selection)
			{
			case 0: npc_hand = &frock; break;
			case 1: npc_hand = &fpaper; break;
			case 2: npc_hand = &fscissor; break;
			}

			if (npc_hand)
				npc_hand->draw(position + Point<int16_t>(dimension.x() - 110, 80));

			// Draw result overlay
			switch (game_result)
			{
			case 0:
				lose_tex.draw(center + Point<int16_t>(0, 50));
				char_lose.draw(position + Point<int16_t>(50, 40));
				break;
			case 1:
				win_tex.draw(center + Point<int16_t>(0, 50));
				char_win.draw(position + Point<int16_t>(50, 40));
				break;
			case 2:
				draw_tex.draw(center + Point<int16_t>(0, 50));
				break;
			}
		}

		score_label.draw(center + Point<int16_t>(0, dimension.y() - 30));
		result_label.draw(center + Point<int16_t>(0, dimension.y() - 50));
	}

	void UIRPSGame::update()
	{
		UIElement::update();
	}

	void UIRPSGame::show_result(int8_t p_choice, int8_t n_choice, int8_t result)
	{
		player_selection = p_choice;
		npc_selection = n_choice;
		game_result = result;

		if (result == 1)
		{
			wins++;
			result_label.change_text("You win!");
		}
		else if (result == 0)
		{
			result_label.change_text("You lose!");
		}
		else
		{
			result_label.change_text("Draw!");
		}

		score_label.change_text("Wins: " + std::to_string(wins));
		set_phase(Phase::RESULT);
	}

	void UIRPSGame::set_phase(int8_t phase)
	{
		current_phase = static_cast<Phase>(phase);

		bool waiting = (current_phase == Phase::WAITING);
		bool selecting = (current_phase == Phase::SELECTING);
		bool result = (current_phase == Phase::RESULT);

		buttons[Buttons::BT_START]->set_active(waiting);
		buttons[Buttons::BT_ROCK]->set_active(selecting);
		buttons[Buttons::BT_PAPER]->set_active(selecting);
		buttons[Buttons::BT_SCISSOR]->set_active(selecting);
		buttons[Buttons::BT_CONTINUE]->set_active(result && game_result == 1);
		buttons[Buttons::BT_RETRY]->set_active(result);
		buttons[Buttons::BT_EXIT]->set_active(waiting || result);
	}

	Cursor::State UIRPSGame::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIRPSGame::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIRPSGame::get_type() const
	{
		return TYPE;
	}

	Button::State UIRPSGame::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_START:
			set_phase(Phase::SELECTING);
			break;
		case Buttons::BT_ROCK:
			RPSSelectionPacket(0).dispatch();
			break;
		case Buttons::BT_PAPER:
			RPSSelectionPacket(1).dispatch();
			break;
		case Buttons::BT_SCISSOR:
			RPSSelectionPacket(2).dispatch();
			break;
		case Buttons::BT_CONTINUE:
			set_phase(Phase::SELECTING);
			break;
		case Buttons::BT_RETRY:
			wins = 0;
			score_label.change_text("Wins: 0");
			result_label.change_text("");
			set_phase(Phase::SELECTING);
			break;
		case Buttons::BT_EXIT:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
