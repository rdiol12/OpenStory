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
#include "EventHandlers.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIEvent.h"
#include "../../IO/UITypes/UIMonsterCarnival.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

namespace ms
{
	void MonsterCarnivalStartHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();
		int16_t team_cp = recv.read_short();
		int16_t team_total_cp = recv.read_short();
		int16_t opp_cp = recv.read_short();
		int16_t opp_total_cp = recv.read_short();
		recv.read_short(); // unused
		recv.read_long();  // unused

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
		{
			carnival->set_team(team);
			carnival->set_cp(cp, total_cp, opp_cp, opp_total_cp);
		}
	}

	void MonsterCarnivalCPHandler::handle(InPacket& recv) const
	{
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
			carnival->set_cp(cp, total_cp, -1, -1);
	}

	void MonsterCarnivalPartyCPHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
		{
			// Team 0 = my team, team 1 = enemy
			if (team == 0)
				carnival->set_cp(cp, total_cp, -1, -1);
			else
				carnival->set_cp(-1, -1, cp, total_cp);
		}
	}

	void MonsterCarnivalSummonHandler::handle(InPacket& recv) const
	{
		int8_t tab = recv.read_byte();
		int8_t number = recv.read_byte();
		std::string name = recv.read_string();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
			carnival->set_summon_count(tab, number);

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + name + " summoned a monster!", UIChatBar::LineType::YELLOW);
	}

	void MonsterCarnivalMessageHandler::handle(InPacket& recv) const
	{
		int8_t message = recv.read_byte();

		std::string msg;
		switch (message)
		{
		case 1: msg = "You don't have enough CP!"; break;
		case 2: msg = "The summon limit has been reached!"; break;
		case 3: msg = "You cannot summon during this phase!"; break;
		case 4: msg = "The carnival is about to end!"; break;
		default: msg = "Carnival message: " + std::to_string(message); break;
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + msg, UIChatBar::LineType::YELLOW);
	}

	void MonsterCarnivalDiedHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		std::string name = recv.read_string();
		int8_t lost_cp = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + name + " was eliminated! (-" + std::to_string(lost_cp) + " CP)", UIChatBar::LineType::RED);
	}

	void OXQuizHandler::handle(InPacket& recv) const
	{
		bool asking = recv.read_byte() != 0;
		int8_t question_set = recv.read_byte();
		int16_t question_id = recv.read_short();

		if (asking)
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[OX Quiz] Question " + std::to_string(question_set) + "-" + std::to_string(question_id) + ": Move to O or X!", UIChatBar::LineType::YELLOW);
		}
	}

	void SnowballStateHandler::handle(InPacket& recv) const
	{
		// First check if it's an enter-map packet (21 zero bytes) or state update
		int8_t state = recv.read_byte();

		if (state == 0) // Could be entermap zeroes
		{
			recv.skip(20); // rest of the 21 zeroes
		}
		else
		{
			int32_t ball0_hp = recv.read_int();
			int32_t ball1_hp = recv.read_int();
			recv.read_short(); // ball0 position
			recv.read_byte();  // -1
			recv.read_short(); // ball1 position
			recv.read_byte();  // -1

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Snowball] Team 1 HP: " + std::to_string(ball0_hp) + " | Team 2 HP: " + std::to_string(ball1_hp), UIChatBar::LineType::YELLOW);
		}
	}

	void HitSnowballHandler::handle(InPacket& recv) const
	{
		int8_t what = recv.read_byte();
		int32_t damage = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Snowball] Hit! Damage: " + std::to_string(damage), UIChatBar::LineType::YELLOW);
	}

	void SnowballMessageHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte(); // 0=down, 1=up
		int32_t message = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Snowball] Team " + std::to_string(team) + " event update!", UIChatBar::LineType::YELLOW);
	}

	void CoconutHitHandler::handle(InPacket& recv) const
	{
		int16_t id = recv.read_short();
		int16_t delay = recv.read_short();
		int8_t type = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Coconut] Hit id=" + std::to_string(id) + " type=" + std::to_string(type), UIChatBar::LineType::YELLOW);
	}

	void CoconutScoreHandler::handle(InPacket& recv) const
	{
		int16_t team1 = recv.read_short();
		int16_t team2 = recv.read_short();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Coconut] Score: Team 1 = " + std::to_string(team1) + " | Team 2 = " + std::to_string(team2), UIChatBar::LineType::YELLOW);
	}

	void AriantScoreHandler::handle(InPacket& recv) const
	{
		int8_t count = recv.read_byte();

		std::string scoreboard = "[Ariant Arena] Scores: ";

		for (int8_t i = 0; i < count; i++)
		{
			std::string name = recv.read_string();
			int32_t score = recv.read_int();

			if (i > 0)
				scoreboard += ", ";

			scoreboard += name + ": " + std::to_string(score);
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(scoreboard, UIChatBar::LineType::YELLOW);
	}

	void AriantShowResultHandler::handle(InPacket& recv) const
	{
		// Empty body — triggers result dialog
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Ariant] Match results!", UIChatBar::LineType::YELLOW);
	}

	void PyramidGaugeHandler::handle(InPacket& recv) const
	{
		int32_t gauge = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pyramid] Gauge: " + std::to_string(gauge), UIChatBar::LineType::YELLOW);
	}

	void PyramidScoreHandler::handle(InPacket& recv) const
	{
		int8_t rank = recv.read_byte(); // 0=S, 1=A, 2=B, 3=C, 4=D
		int32_t exp = recv.read_int();

		std::string ranks[] = { "S", "A", "B", "C", "D" };
		std::string rank_str = (rank >= 0 && rank <= 4) ? ranks[rank] : "?";

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pyramid] Rank " + rank_str + " - EXP: " + std::to_string(exp), UIChatBar::LineType::YELLOW);
	}

	void TournamentHandler::handle(InPacket& recv) const
	{
		int8_t state = recv.read_byte();
		int8_t sub_state = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] State update: " + std::to_string(state) + "/" + std::to_string(sub_state), UIChatBar::LineType::YELLOW);
	}

	void TournamentMatchTableHandler::handle(InPacket& recv) const
	{
		// Empty body — prompts match table dialog
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] Match table received.", UIChatBar::LineType::YELLOW);
	}

	void TournamentSetPrizeHandler::handle(InPacket& recv) const
	{
		int8_t set_prize = recv.read_byte();
		int8_t has_prize = recv.read_byte();

		if (has_prize)
		{
			int32_t item1 = recv.read_int();
			int32_t item2 = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Tournament] Prize items set.", UIChatBar::LineType::YELLOW);
		}
	}

	void TournamentUEWHandler::handle(InPacket& recv) const
	{
		int8_t state = recv.read_byte(); // bitflags for round info

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] UEW state: " + std::to_string(state), UIChatBar::LineType::YELLOW);
	}

	void DojoWarpUpHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 0
		recv.read_byte(); // 6

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Dojo] Warping to next floor!", UIChatBar::LineType::YELLOW);
	}

	void SendTVHandler::handle(InPacket& recv) const
	{
		int8_t has_partner = recv.read_byte(); // 3 = with partner, 1 = solo
		int8_t type = recv.read_byte(); // 0=normal, 1=star, 2=heart

		// CharLook + messages follow — complex parsing
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] Broadcast received.", UIChatBar::LineType::YELLOW);
	}

	void RemoveTVHandler::handle(InPacket& recv) const
	{
		// Empty packet body
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] Broadcast ended.", UIChatBar::LineType::YELLOW);
	}

	void EnableTVHandler::handle(InPacket& recv) const
	{
		recv.read_int(); // 0
		recv.read_byte(); // 0

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] MapleTV enabled.", UIChatBar::LineType::YELLOW);
	}

	void SetAvatarMegaphoneHandler::handle(InPacket& recv) const
	{
		int32_t item_id = recv.read_int();
		std::string name = recv.read_string();

		// Multiple message strings follow, then channel, ear flag, and CharLook
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[AvatarMegaphone] " + name, UIChatBar::LineType::YELLOW);
	}

	void ClearAvatarMegaphoneHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 1
	}

	void SpawnKiteHandler::handle(InPacket& recv) const
	{
		int32_t obj_id = recv.read_int();
		int32_t item_id = recv.read_int();
		std::string msg = recv.read_string();
		std::string name = recv.read_string();
		int16_t x = recv.read_short();
		int16_t fh = recv.read_short();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Kite] " + name + ": " + msg, UIChatBar::LineType::YELLOW);
	}

	void RemoveKiteHandler::handle(InPacket& recv) const
	{
		int8_t anim = recv.read_byte();
		int32_t obj_id = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Kite] A kite has been removed.", UIChatBar::LineType::YELLOW);
	}

	void CannotSpawnKiteHandler::handle(InPacket& recv) const
	{
		// Empty body
		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, "Cannot place a kite here.");
	}

	void EventInfoHandler::handle(InPacket& recv) const
	{
		int16_t event_count = recv.read_short();

		std::vector<EventData> events;
		events.reserve(event_count);

		for (int16_t i = 0; i < event_count; i++)
		{
			EventData ev;
			ev.type = recv.read_byte();
			ev.name = recv.read_string();
			ev.description = recv.read_string();
			ev.seconds_remaining = recv.read_int();
			ev.multiplier = recv.read_short();
			ev.has_item_rewards = recv.read_byte() != 0;

			int16_t reward_count = recv.read_short();

			for (int16_t r = 0; r < reward_count; r++)
			{
				int32_t item_id = recv.read_int();
				int16_t quantity = recv.read_short();
				ev.rewards.emplace_back(item_id, quantity);
			}

			events.push_back(std::move(ev));
		}

		if (auto event_ui = UI::get().get_element<UIEvent>())
			event_ui->set_events(std::move(events));
	}
}