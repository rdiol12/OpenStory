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

#include <string>
#include <cstdint>
#include <map>

namespace ms
{
	struct BuddyEntry
	{
		int32_t cid = 0;
		std::string name;
		int8_t status = 0;
		int32_t channel = -1;
		std::string group;

		bool online() const { return channel >= 0; }
	};

	class BuddyList
	{
	public:
		void update(const std::map<int32_t, BuddyEntry>& entries);
		void set_capacity(int8_t cap);
		void clear();

		const std::map<int32_t, BuddyEntry>& get_entries() const;
		int8_t get_capacity() const;

	private:
		std::map<int32_t, BuddyEntry> buddies;
		int8_t capacity = 50;
	};
}
