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
#include "UIControlHandlers.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIEquipInventory.h"
#include "../../IO/UITypes/UIStatsInfo.h"
#include "../../IO/UITypes/UISkillBook.h"
#include "../../IO/UITypes/UIBuddyList.h"
#include "../../IO/UITypes/UIWorldMap.h"
#include "../../IO/UITypes/UIKeyConfig.h"
#include "../../IO/UITypes/UIMonsterBook.h"
#include "../../IO/UITypes/UIQuestLog.h"
#include "../../IO/UITypes/UIMessenger.h"
#include "../../IO/UITypes/UIPartySearch.h"
#include "../../IO/UITypes/UIComboCounter.h"

namespace ms
{
	void OpenUIHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t ui_type = recv.read_byte();

		Player& player = Stage::get().get_player();

		// v83 OpenUI types — server requests client to open a specific window
		switch (ui_type)
		{
		case 0x00: // Equipment inventory
			UI::get().emplace<UIEquipInventory>(player.get_inventory());
			break;
		case 0x01: // Stats
			UI::get().emplace<UIStatsInfo>(player.get_stats());
			break;
		case 0x02: // Skill book
			UI::get().emplace<UISkillBook>(player.get_stats(), player.get_skills());
			break;
		case 0x03: // Buddy list
			UI::get().emplace<UIBuddyList>();
			break;
		case 0x04: // World map
			UI::get().emplace<UIWorldMap>();
			break;
		case 0x05: // Messenger
			UI::get().emplace<UIMessenger>();
			break;
		case 0x09: // Quest log
			UI::get().emplace<UIQuestLog>(player.get_quests());
			break;
		case 0x0A: // Key config
			UI::get().emplace<UIKeyConfig>(player.get_inventory(), player.get_skills());
			break;
		case 0x12: // Monster book
			UI::get().emplace<UIMonsterBook>();
			break;
		case 0x16: // Party search
			UI::get().emplace<UIPartySearch>();
			break;
		default:
			break;
		}
	}

	void LockUIHandler::handle(InPacket& recv) const
	{
		bool locked = recv.read_byte() != 0;

		if (locked)
			UI::get().disable();
		else
			UI::get().enable();
	}

	void DisableUIHandler::handle(InPacket& recv) const
	{
		bool disabled = recv.read_byte() != 0;

		if (disabled)
			UI::get().disable();
		else
			UI::get().enable();
	}

	void SpawnGuideHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		bool spawn = recv.read_bool();

		// Beginner guide NPC — show helpful message when spawned
		if (spawn)
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guide] Maple Guide has appeared! Press the guide key for help.", UIChatBar::LineType::YELLOW);
		}
	}

	void TalkGuideHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 0)
		{
			std::string text = recv.read_string();
			recv.skip(8); // two ints: display timing (200, 4000)

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guide] " + text, UIChatBar::LineType::YELLOW);
		}
		else if (mode == 1)
		{
			int32_t hint_id = recv.read_int();
			int32_t duration = recv.read_int();

			(void)hint_id;
			(void)duration;
		}
	}

	void ShowComboHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int32_t count = recv.read_int();

		Stage::get().set_combo(count);

		if (count > 0)
		{
			if (!UI::get().get_element<UIComboCounter>())
				UI::get().emplace<UIComboCounter>();
		}
	}
}
