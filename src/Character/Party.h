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

#include "../Template/Point.h"

#include <string>
#include <cstdint>
#include <vector>

namespace ms
{
	struct PartyMember
	{
		int32_t cid = 0;
		std::string name;
		int16_t job = 0;
		int16_t level = 0;
		int32_t channel = -1;
		int32_t mapid = 0;
		bool online = false;
		int32_t hp = 0;
		int32_t maxhp = 0;
	};

	class Party
	{
	public:
		void update(int32_t partyid, const std::vector<PartyMember>& members, int32_t leader_cid);
		void update_member_hp(int32_t cid, int32_t hp, int32_t maxhp);
		void clear();
		bool is_in_party() const;
		int32_t get_id() const;
		int32_t get_leader() const;
		const std::vector<PartyMember>& get_members() const;

	private:
		int32_t id = 0;
		int32_t leader = 0;
		std::vector<PartyMember> members;
	};

	// Party overhead HP gauge on a character (stamped by UIPartyHUD while
	// partied) — same look and behaviour as the mob bar.
	class PartyHpBar
	{
	public:
		void set(int32_t hp, int32_t maxhp);
		void clear();
		void draw(Point<int16_t> absp) const;

	private:
		int32_t hp = 0;
		int32_t maxhp = 0;
	};
}
