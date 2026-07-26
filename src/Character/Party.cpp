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

#include "../Graphics/Geometry.h"

namespace ms
{
	void Party::update(int32_t partyid, const std::vector<PartyMember>& new_members, int32_t leader_cid)
	{
		id = partyid;
		leader = leader_cid;

		// carry known HP over — the full-party packets don't include HP, so a
		// silent update must not wipe the gauges until the next HP packet
		std::vector<PartyMember> merged = new_members;

		for (auto& nm : merged)
			for (const auto& om : members)
				if (om.cid == nm.cid)
				{
					nm.hp = om.hp;
					nm.maxhp = om.maxhp;
					break;
				}

		members = std::move(merged);
	}

	void Party::update_member_hp(int32_t cid, int32_t hp, int32_t maxhp)
	{
		for (auto& member : members)
		{
			if (member.cid == cid)
			{
				member.hp = hp;
				member.maxhp = maxhp;
				return;
			}
		}
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

	void PartyHpBar::set(int32_t h, int32_t mh)
	{
		hp = h;
		maxhp = mh;
	}

	void PartyHpBar::clear()
	{
		hp = 0;
		maxhp = 0;
	}

	void PartyHpBar::draw(Point<int16_t> absp) const
	{
		if (maxhp <= 0)
			return;

		static MobHpBar bar;
		int16_t percent = static_cast<int16_t>(
			hp > 0 ? (100 * hp) / maxhp : 0);
		if (percent > 100)
			percent = 100;
		bar.draw(absp + Point<int16_t>(0, -58), percent);
	}
}
