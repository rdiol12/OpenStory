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
#include "ChannelLoadData.h"

namespace ms
{
	void ChannelLoadData::update(int8_t wid, uint8_t count, const std::vector<int32_t>& loads)
	{
		Entry& entry = worlds_[wid];
		entry.count = count;
		entry.loads = loads;
	}

	uint8_t ChannelLoadData::get_count(int8_t wid) const
	{
		auto it = worlds_.find(wid);
		if (it == worlds_.end())
			return 0;

		return it->second.count;
	}

	int32_t ChannelLoadData::get_load(int8_t wid, uint8_t channel) const
	{
		auto it = worlds_.find(wid);
		if (it == worlds_.end())
			return 0;

		if (channel >= it->second.loads.size())
			return 0;

		return it->second.loads[channel];
	}
}
