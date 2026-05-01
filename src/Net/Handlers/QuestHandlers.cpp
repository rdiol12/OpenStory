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
#include "QuestHandlers.h"
#include "../../Audio/Audio.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIQuestLog.h"
#include "../../IO/UITypes/UIStatusBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

namespace ms
{
	void UpdateQuestInfoHandler::handle(InPacket& recv) const
	{
		// Cosmic uses UPDATE_QUEST_INFO (0xD3) for NPC info, time limits, expiration
		// Quest state changes go through SHOW_STATUS_INFO (0x27) mode 1
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 6:
		{
			// Add quest time limit
			int16_t count = recv.read_short();
			for (int16_t i = 0; i < count; i++)
			{
				int16_t questid = recv.read_short();
				int32_t time_ms = recv.read_int();

				// Set countdown for the first (or most recent) quest timer
				Stage::get().set_countdown(time_ms / 1000);

				chat::log("[Quest] Time limit set: " + std::to_string(time_ms / 1000) + " seconds", chat::LineType::YELLOW);
			}
			break;
		}
		case 7:
		{
			// Remove quest time limit
			int16_t pos = recv.read_short();
			int16_t questid = recv.read_short();
			(void)questid;
			break;
		}
		case 8:
		{
			// Quest NPC info (sent after quest start/complete)
			int16_t questid = recv.read_short();
			int32_t npcid = recv.read_int();
			int32_t zero = recv.read_int();
			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			break;
		}
		case 0x0A:
		{
			// Quest error — requirements not met
			int16_t questid = recv.read_short();
			(void)questid;

			chat::log("[Quest] You don't meet the requirements for this quest.", chat::LineType::RED);

			break;
		}
		case 0x0B:
		{
			// Quest failure — not enough mesos
			chat::log("[Quest] You don't have enough mesos.", chat::LineType::RED);

			break;
		}
		case 0x0D:
		{
			// Quest failure — equipment is currently worn
			chat::log("[Quest] Item is currently worn by character.", chat::LineType::RED);

			break;
		}
		case 0x0E:
		{
			// Quest failure — missing required item
			chat::log("[Quest] You don't have the required item.", chat::LineType::RED);

			break;
		}
		case 0x0F:
		{
			// Quest expired
			int16_t questid = recv.read_short();
			Stage::get().get_player().get_quests().forfeit(questid);

			chat::log("[Quest] The quest has expired.", chat::LineType::RED);

			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			// Refresh NPC quest marks
			Stage::get().get_npcs().refresh_quest_marks();

			break;
		}
		default:
			// Unhandled quest info mode
			break;
		}
	}

	void QuestClearHandler::handle(InPacket& recv) const
	{
		// Cosmic sends: SHORT questId — visual quest completion notification
		if (recv.length() < 2)
			return;

		int16_t questid = recv.read_short();
		(void)questid;

		// Play quest clear effect on player (light pillar)
		Stage::get().get_player().show_effect_id(CharEffect::Id::QUEST_CLEAR);

		// Play quest complete sound
		Sound(Sound::Name::QUESTCOMPLETE).play();

		// Show notice button alert
		if (auto statusbar = UI::get().get_element<UIStatusBar>())
			statusbar->notify();

		// Refresh quest log UI if open
		if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
			questlog_ui->load_quests();
	}

	void ScriptProgressMessageHandler::handle(InPacket& recv) const
	{
		// v83 SCRIPT_PROGRESS_MESSAGE (opcode 122): string message
		// Shows a progress/info message during scripted events (quests, NPCs, etc.)
		if (!recv.available())
			return;

		std::string message = recv.read_string();

		if (!message.empty())
		{
			auto messenger = UI::get().get_element<UIStatusMessenger>();

			if (messenger)
				messenger->show_status(Color::Name::WHITE, message);
		}
	}
}
