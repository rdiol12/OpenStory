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

#include "../Graphics/Animation.h"
#include "../Graphics/Text.h"
#include "../Template/Point.h"
#include "../Template/Singleton.h"

#include <cstdint>
#include <map>
#include <string>

namespace ms
{
	class HiredMerchants : public Singleton<HiredMerchants>
	{
	public:
		struct Merchant
		{
			int32_t oid;
			int32_t item_id;
			Point<int16_t> pos;
			std::string owner;
			std::string desc;
			int8_t skin;
			Animation stand;
		};

		void add(int32_t owner_id, int32_t oid, int32_t item_id, Point<int16_t> pos,
			const std::string& owner, const std::string& desc, int8_t skin);
		void remove(int32_t owner_id);
		void clear_all();

		void draw(Point<int16_t> viewpos, float alpha) const;
		void update();

		const Merchant* find_at(Point<int16_t> mappos) const;

	private:
		std::map<int32_t, Merchant> merchants;
	};
}
