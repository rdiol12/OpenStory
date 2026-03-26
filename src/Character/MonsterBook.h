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

#include <cstdint>
#include <map>

namespace ms
{
	// Class that represents the monster card collection of an individual character
	class MonsterBook
	{
	public:
		MonsterBook();

		void set_cover(int32_t cov);
		void add_card(int32_t cardid, int8_t level);

		int32_t get_cover() const;
		int8_t get_card_level(int32_t cardid) const;
		const std::map<int32_t, int8_t>& get_cards() const;
		int16_t get_total_cards() const;
		int32_t get_book_level() const;
		int32_t get_normal_cards() const;
		int32_t get_special_cards() const;

	private:
		int32_t cover;
		std::map<int32_t, int8_t> cards;
	};
}
