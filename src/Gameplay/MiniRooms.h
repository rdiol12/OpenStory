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
#include <string>

namespace ms
{
	class MiniRooms : public Singleton<MiniRooms>
	{
	public:
		struct Box
		{
			int8_t type;
			int32_t oid;
			std::string desc;
			int8_t skin;
		};

		void set(int32_t cid, int8_t type, int32_t oid, const std::string& desc, int8_t skin = 0)
		{
			boxes[cid] = { type, oid, desc, skin };
		}

		void clear(int32_t cid)
		{
			boxes.erase(cid);
		}

		void clear_all()
		{
			boxes.clear();
		}

		const Box* find(int32_t cid) const
		{
			auto it = boxes.find(cid);

			return it == boxes.end() ? nullptr : &it->second;
		}

	private:
		std::map<int32_t, Box> boxes;
	};
}
