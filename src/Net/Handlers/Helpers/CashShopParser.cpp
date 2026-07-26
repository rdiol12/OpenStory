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
#include "CashShopParser.h"

#include "CharacterParser.h"
#include "LoginParser.h"

#include "../../../Gameplay/Stage.h"

namespace ms
{
	namespace CashShopParser
	{
		StatsEntry parseCharacterInfo(InPacket& recv)
		{
			recv.read_long();
			recv.read_byte();

			StatsEntry statsentry = parseCharStats(recv);

			Player& player = Stage::get().get_player();

			player.get_stats().set_mapid(statsentry.mapid);
			player.get_stats().set_portal(statsentry.portal);

			recv.read_byte(); // 'buddycap'

			if (recv.read_bool())
				recv.read_string(); // 'linkedname'

			CharacterParser::parse_inventory(recv, player.get_inventory());
			CharacterParser::parse_skillbook(recv, player.get_skills());
			CharacterParser::parse_cooldowns(recv, player);
			CharacterParser::parse_questlog(recv, player.get_quests());
			CharacterParser::parse_minigame(recv);
			CharacterParser::parse_ring1(recv);
			CharacterParser::parse_ring2(recv);
			CharacterParser::parse_ring3(recv);
			CharacterParser::parse_teleportrock(recv, player.get_teleportrock());
			CharacterParser::parse_monsterbook(recv, player.get_monsterbook());
			CharacterParser::parse_nyinfo(recv);
			CharacterParser::parse_areainfo(recv);

			player.recalc_stats(true);

			recv.read_short();

			return statsentry;
		}

		StatsEntry parseCharStats(InPacket& recv)
		{
			recv.read_int(); // character id

			return LoginParser::parse_stats(recv, true, nullptr, nullptr, nullptr);
		}

		bool hasSPTable(int16_t job)
		{
			switch (job)
			{
			case Jobs::EVAN:
			case Jobs::EVAN1:
			case Jobs::EVAN2:
			case Jobs::EVAN3:
			case Jobs::EVAN4:
			case Jobs::EVAN5:
			case Jobs::EVAN6:
			case Jobs::EVAN7:
			case Jobs::EVAN8:
			case Jobs::EVAN9:
			case Jobs::EVAN10:
				return true;
			default:
				return false;
			}
		}

		void parseRemainingSkillInfo(InPacket& recv)
		{
			int count = recv.read_byte();

			for (int i = 0; i < count; i++)
			{
				recv.read_byte(); // Remaining SP index for job 
				recv.read_byte(); // The actual SP for that class
			}
		}
	}
}