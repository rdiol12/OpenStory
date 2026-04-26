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
#include "BuddyList.h"

namespace ms
{
	void BuddyList::update(const std::map<int32_t, BuddyEntry>& entries)
	{
		buddies = entries;
	}

	void BuddyList::set_capacity(int8_t cap)
	{
		capacity = cap;
	}

	void BuddyList::clear()
	{
		buddies.clear();
	}

	bool BuddyList::update_channel(int32_t cid, int32_t channel)
	{
		auto it = buddies.find(cid);
		if (it == buddies.end()) return false;

		bool was_online = it->second.online();
		it->second.channel = channel;
		return was_online;
	}

	const std::map<int32_t, BuddyEntry>& BuddyList::get_entries() const
	{
		return buddies;
	}

	int8_t BuddyList::get_capacity() const
	{
		return capacity;
	}
}
