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

#include "../Template/Singleton.h"

#include <cstdint>
#include <map>
#include <vector>

namespace ms
{
	// Caches per-world channel metadata received from the login server's
	// world-list packet. The data outlives UIWorldSelect so that in-game UIs
	// (UIChannel) can still query channel count / population after login.
	class ChannelLoadData : public Singleton<ChannelLoadData>
	{
	public:
		void update(int8_t wid, uint8_t count, const std::vector<int32_t>& loads);

		uint8_t get_count(int8_t wid) const;
		int32_t get_load(int8_t wid, uint8_t channel) const;

		// Population threshold considered "full" for gauge scaling. Cosmic /
		// HeavenMS typically caps channel population well below this.
		static constexpr int32_t MAX_LOAD = 800;

	private:
		struct Entry
		{
			uint8_t count = 0;
			std::vector<int32_t> loads;
		};

		std::map<int8_t, Entry> worlds_;
	};
}
