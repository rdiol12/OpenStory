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
#include "Party.h"

namespace ms
{
	void Party::update(int32_t partyid, const std::vector<PartyMember>& new_members, int32_t leader_cid)
	{
		id = partyid;
		leader = leader_cid;
		members = new_members;
	}

	void Party::clear()
	{
		id = 0;
		leader = 0;
		members.clear();
	}

	bool Party::is_in_party() const
	{
		return id != 0;
	}

	int32_t Party::get_id() const
	{
		return id;
	}

	int32_t Party::get_leader() const
	{
		return leader;
	}

	const std::vector<PartyMember>& Party::get_members() const
	{
		return members;
	}
}
