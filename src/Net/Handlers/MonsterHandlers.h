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
#pragma once

#include "../PacketHandler.h"

#include "../../Template/Singleton.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{
	struct MonsterLifeEntry
	{
		int32_t slotid = 0;
		int32_t cardid = 0;
		int16_t x = 0;
		int16_t y = 0;
		uint8_t level = 0;
		int32_t exp = 0;
	};

	class MonsterLifeState : public Singleton<MonsterLifeState>
	{
	public:
		void set_farm(int32_t owner, const std::string& name, uint8_t lvl, int32_t exp,
			bool edit, std::vector<MonsterLifeEntry> entries);
		void place(const MonsterLifeEntry& e);
		void move(int32_t slotid, int16_t x, int16_t y);
		void remove(int32_t slotid);
		void set_error(uint8_t code);

		bool is_loaded() const { return loaded; }
		bool is_editable() const { return editable; }
		int32_t get_owner() const { return owner_charid; }
		const std::string& get_owner_name() const { return owner_name; }
		uint8_t get_level() const { return farm_level; }
		int32_t get_exp() const { return farm_exp; }
		uint8_t get_error() const { return last_error; }
		const std::vector<MonsterLifeEntry>& get_entries() const { return entries; }

	private:
		bool loaded = false;
		bool editable = false;
		int32_t owner_charid = 0;
		std::string owner_name;
		uint8_t farm_level = 0;
		int32_t farm_exp = 0;
		uint8_t last_error = 0;
		std::vector<MonsterLifeEntry> entries;
	};

	struct MonsterBattleUnit
	{
		int32_t cardid = 0;
		uint8_t level = 0;
		int32_t exp = 0;
		int16_t owned = 0;
	};

	class MonsterBattleState : public Singleton<MonsterBattleState>
	{
	public:
		void set_collection(std::vector<MonsterBattleUnit> units, std::vector<int32_t> team);
		void set_team(std::vector<int32_t> team);
		void set_error(uint8_t code);

		bool is_loaded() const { return loaded; }
		uint8_t get_error() const { return last_error; }
		const std::vector<MonsterBattleUnit>& get_units() const { return units; }
		const std::vector<int32_t>& get_team() const { return team; }

	private:
		bool loaded = false;
		uint8_t last_error = 0;
		std::vector<MonsterBattleUnit> units;
		std::vector<int32_t> team;
	};

	class MonsterLifeUpdateHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class MonsterBattleUpdateHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}
