//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
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
#include "MonsterHandlers.h"

#include "../InPacket.h"

#include <algorithm>

namespace ms
{
	void MonsterLifeState::set_farm(int32_t owner, const std::string& name, uint8_t lvl,
		int32_t exp, bool edit, std::vector<MonsterLifeEntry> in)
	{
		owner_charid = owner;
		owner_name = name;
		farm_level = lvl;
		farm_exp = exp;
		editable = edit;
		entries = std::move(in);
		last_error = 0;
		loaded = true;
	}

	void MonsterLifeState::place(const MonsterLifeEntry& e)
	{
		auto it = std::find_if(entries.begin(), entries.end(),
			[&e](const MonsterLifeEntry& o) { return o.slotid == e.slotid; });

		if (it == entries.end())
			entries.push_back(e);
		else
			*it = e;
	}

	void MonsterLifeState::move(int32_t slotid, int16_t x, int16_t y)
	{
		for (auto& e : entries)
		{
			if (e.slotid == slotid)
			{
				e.x = x;
				e.y = y;
				return;
			}
		}
	}

	void MonsterLifeState::remove(int32_t slotid)
	{
		entries.erase(std::remove_if(entries.begin(), entries.end(),
			[slotid](const MonsterLifeEntry& e) { return e.slotid == slotid; }),
			entries.end());
	}

	void MonsterLifeState::set_error(uint8_t code)
	{
		last_error = code;
	}

	void MonsterBattleState::set_collection(std::vector<MonsterBattleUnit> in,
		std::vector<int32_t> t)
	{
		units = std::move(in);
		team = std::move(t);
		last_error = 0;
		loaded = true;
	}

	void MonsterBattleState::set_team(std::vector<int32_t> t)
	{
		team = std::move(t);
		last_error = 0;
	}

	void MonsterBattleState::set_error(uint8_t code)
	{
		last_error = code;
	}

	void MonsterLifeUpdateHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x00:
		{
			int32_t owner = recv.read_int();
			std::string name = recv.read_string();
			uint8_t lvl = static_cast<uint8_t>(recv.read_byte());
			int32_t exp = recv.read_int();
			bool edit = recv.read_bool();
			int16_t count = recv.read_short();

			std::vector<MonsterLifeEntry> entries;
			entries.reserve(std::max<int16_t>(count, 0));

			for (int16_t i = 0; i < count && recv.available(); i++)
			{
				MonsterLifeEntry e;
				e.slotid = recv.read_int();
				e.cardid = recv.read_int();
				e.x = recv.read_short();
				e.y = recv.read_short();
				e.level = static_cast<uint8_t>(recv.read_byte());
				e.exp = recv.read_int();
				entries.push_back(e);
			}

			MonsterLifeState::get().set_farm(owner, name, lvl, exp, edit, std::move(entries));
			break;
		}
		case 0x01:
		{
			MonsterLifeEntry e;
			e.slotid = recv.read_int();
			e.cardid = recv.read_int();
			e.x = recv.read_short();
			e.y = recv.read_short();
			MonsterLifeState::get().place(e);
			break;
		}
		case 0x02:
		{
			int32_t slotid = recv.read_int();
			int16_t x = recv.read_short();
			int16_t y = recv.read_short();
			MonsterLifeState::get().move(slotid, x, y);
			break;
		}
		case 0x03:
			MonsterLifeState::get().remove(recv.read_int());
			break;
		case 0x04:
			MonsterLifeState::get().set_error(static_cast<uint8_t>(recv.read_byte()));
			break;
		}
	}

	void MonsterBattleUpdateHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x00:
		{
			int16_t count = recv.read_short();

			std::vector<MonsterBattleUnit> units;
			units.reserve(std::max<int16_t>(count, 0));

			for (int16_t i = 0; i < count && recv.available(); i++)
			{
				MonsterBattleUnit u;
				u.cardid = recv.read_int();
				u.level = static_cast<uint8_t>(recv.read_byte());
				u.exp = recv.read_int();
				u.owned = recv.read_short();
				units.push_back(u);
			}

			int16_t tcount = recv.read_short();
			std::vector<int32_t> team;
			team.reserve(std::max<int16_t>(tcount, 0));

			for (int16_t i = 0; i < tcount && recv.available(); i++)
				team.push_back(recv.read_int());

			MonsterBattleState::get().set_collection(std::move(units), std::move(team));
			break;
		}
		case 0x01:
		{
			int8_t count = recv.read_byte();
			std::vector<int32_t> team;

			for (int8_t i = 0; i < count && recv.available(); i++)
				team.push_back(recv.read_int());

			MonsterBattleState::get().set_team(std::move(team));
			break;
		}
		case 0x02:
			MonsterBattleState::get().set_error(static_cast<uint8_t>(recv.read_byte()));
			break;
		}
	}
}
