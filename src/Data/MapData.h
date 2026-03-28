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

#include "../Template/Cache.h"

#include <string>
#include <vector>
#include <cstdint>

namespace ms
{
	class MapData : public Cache<MapData>
	{
	public:
		struct Portal
		{
			std::string name;
			std::string target_name;
			int32_t target_mapid;
			int8_t type;
			int16_t x;
			int16_t y;
		};

		struct LifeEntry
		{
			int32_t id;
			std::string type;
			int16_t x;
			int16_t y;
			int16_t cy;
			uint16_t fh;
			bool flip;
			int16_t rx0;
			int16_t rx1;
			int32_t mob_time;
		};

		bool is_valid() const;
		explicit operator bool() const;

		const std::string& get_name() const;
		const std::string& get_street_name() const;
		const std::string& get_bgm() const;
		const std::string& get_map_mark() const;
		int32_t get_return_map() const;
		int32_t get_forced_return() const;
		int32_t get_field_limit() const;
		float get_mob_rate() const;
		bool is_town() const;
		bool is_swim() const;
		bool is_hide_minimap() const;

		const std::vector<Portal>& get_portals() const;
		const std::vector<LifeEntry>& get_life() const;

	private:
		friend Cache<MapData>;
		MapData(int32_t mapid);

		bool valid;
		std::string name;
		std::string street_name;
		std::string bgm;
		std::string map_mark;
		int32_t return_map;
		int32_t forced_return;
		int32_t field_limit;
		float mob_rate;
		bool town;
		bool swim;
		bool hide_minimap;

		std::vector<Portal> portals;
		std::vector<LifeEntry> life;
	};
}
