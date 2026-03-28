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

#include "../Template/Cache.h"

#include <string>
#include <vector>
#include <cstdint>

namespace ms
{
	class PetData : public Cache<PetData>
	{
	public:
		bool is_valid() const;
		explicit operator bool() const;

		const std::string& get_name() const;
		const std::string& get_desc() const;
		int32_t get_hunger() const;
		int32_t get_life() const;
		bool is_permanent() const;
		const std::vector<std::string>& get_commands() const;

	private:
		friend Cache<PetData>;
		PetData(int32_t itemid);

		bool valid;
		std::string name;
		std::string desc;
		int32_t hunger;
		int32_t life;
		bool permanent;
		std::vector<std::string> commands;
	};
}
