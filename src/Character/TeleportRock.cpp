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
#include "TeleportRock.h"

#include <algorithm>

namespace ms
{
	void TeleportRock::addlocation(int32_t mapid)
	{
		locations.push_back(mapid);
	}

	void TeleportRock::addviplocation(int32_t mapid)
	{
		viplocations.push_back(mapid);
	}

	void TeleportRock::set_locations(std::vector<int32_t> maps, bool vip)
	{
		// 999999999 is the server's empty-slot marker; keeping it would show
		// phantom destinations in the UI.
		maps.erase(std::remove(maps.begin(), maps.end(), 999999999), maps.end());

		if (vip)
			viplocations = std::move(maps);
		else
			locations = std::move(maps);
	}

	const std::vector<int32_t>& TeleportRock::get_locations(bool vip) const
	{
		return vip ? viplocations : locations;
	}
}