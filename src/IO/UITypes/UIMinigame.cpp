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
#include "UIMinigame.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMinigame::UIMinigame() : UIDragElement<PosMINIGAME>(Point<int16_t>(250, 20)),
		is_my_turn(false), am_host(true), last_result(-1),
		my_wins(0), my_ties(0), my_losses(0)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["Minigame"];
		nl::node common = src["Common"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node omok = src["Omok"];
		nl::node backgrnd = omok["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = omok["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_START] = std::make_unique<MapleButton>(common["btStart"]);
		buttons[Buttons::BT_EXIT] = std::make_unique<MapleButton>(common["btExit"]);
		buttons[Buttons::BT_READY] = std::make_unique<MapleButton>(common["btReady"]);
		buttons[Buttons::BT_DRAW] = std::make_unique<MapleButton>(common["btDraw"]);
		buttons[Buttons::BT_ABSTAIN] = std::make_unique<MapleButton>(common["btAbsten"]);

		win_tex = common["win"];
		lose_tex = common["lose"];
		draw_tex = common["draw"];
		char_win = common["charWin"];
		char_lose = common["charLose"];
		turn_tex = common["turn"];
		ready_on = common["readyOn"];
		ready_off = common["readyOff"];

		// Host sees Start, guest sees Ready
		buttons[Buttons::BT_READY]->set_active(!am_host);
		buttons[Buttons::BT_START]->set_active(am_host);
		buttons[Buttons::BT_DRAW]->set_active(false);
		buttons[Buttons::BT_ABSTAIN]->set_active(false);

		score_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "W:0 T:0 L:0");
		turn_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::YELLOW, "");
		status_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Waiting for opponent...");

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIMinigame::draw(float inter) const
	{
		UIElement::draw(inter);

		Point<int16_t> center = position + Point<int16_t>(dimension.x() / 2, 0);

		// Draw turn indicator
		if (is_my_turn)
			turn_tex.draw(center + Point<int16_t>(0, 30));

		// Draw result overlay
		if (last_result >= 0)
		{
			switch (last_result)
			{
			case 0:
				lose_tex.draw(center + Point<int16_t>(0, dimension.y() / 2));
				char_lose.draw(position + Point<int16_t>(30, 50));
				break;
			case 1:
				win_tex.draw(center + Point<int16_t>(0, dimension.y() / 2));
				char_win.draw(position + Point<int16_t>(30, 50));
				break;
			case 2:
				draw_tex.draw(center + Point<int16_t>(0, dimension.y() / 2));
				break;
			}
		}

		score_label.draw(center + Point<int16_t>(0, dimension.y() - 30));
		turn_label.draw(center + Point<int16_t>(0, 40));
		status_label.draw(center + Point<int16_t>(0, dimension.y() - 50));
	}

	void UIMinigame::update()
	{
		UIElement::update();
	}

	void UIMinigame::set_game_result(int8_t result)
	{
		last_result = result;

		if (result == 1)
		{
			my_wins++;
			status_label.change_text("You win!");
		}
		else if (result == 0)
		{
			my_losses++;
			status_label.change_text("You lose!");
		}
		else if (result == 2)
		{
			my_ties++;
			status_label.change_text("Draw!");
		}

		score_label.change_text("W:" + std::to_string(my_wins) + " T:" + std::to_string(my_ties) + " L:" + std::to_string(my_losses));

		// Show start/ready buttons again
		buttons[Buttons::BT_START]->set_active(am_host);
		buttons[Buttons::BT_READY]->set_active(!am_host);
		buttons[Buttons::BT_DRAW]->set_active(false);
		buttons[Buttons::BT_ABSTAIN]->set_active(false);
	}

	void UIMinigame::set_my_turn(bool turn)
	{
		is_my_turn = turn;
		turn_label.change_text(turn ? "Your turn" : "Opponent's turn");
	}

	void UIMinigame::set_ready_state(bool host_ready, bool guest_ready)
	{
		if (host_ready && guest_ready)
			status_label.change_text("Game starting...");
		else if (guest_ready)
			status_label.change_text("Opponent ready");
		else
			status_label.change_text("Waiting...");
	}

	void UIMinigame::set_scores(int16_t w, int16_t t, int16_t l)
	{
		my_wins = w;
		my_ties = t;
		my_losses = l;
		score_label.change_text("W:" + std::to_string(w) + " T:" + std::to_string(t) + " L:" + std::to_string(l));
	}

	void UIMinigame::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			MinigameExitPacket().dispatch();
			deactivate();
		}
	}

	UIElement::Type UIMinigame::get_type() const
	{
		return TYPE;
	}

	Button::State UIMinigame::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			MinigameExitPacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_START:
			MinigameStartPacket().dispatch();
			last_result = -1;
			buttons[Buttons::BT_START]->set_active(false);
			buttons[Buttons::BT_DRAW]->set_active(true);
			buttons[Buttons::BT_ABSTAIN]->set_active(true);
			status_label.change_text("Game in progress");
			break;
		case Buttons::BT_EXIT:
			MinigameExitPacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_READY:
			MinigameReadyPacket().dispatch();
			buttons[Buttons::BT_READY]->set_active(false);
			status_label.change_text("Ready!");
			break;
		case Buttons::BT_DRAW:
			MinigameDrawPacket().dispatch();
			break;
		case Buttons::BT_ABSTAIN:
			MinigameForfeitPacket().dispatch();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
