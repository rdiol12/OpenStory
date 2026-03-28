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
#include "PetData.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	PetData::PetData(int32_t itemid)
	{
		valid = false;
		hunger = 0;
		life = 0;
		permanent = false;

		std::string strid = std::to_string(itemid);
		nl::node src = nl::nx::item["Pet"][strid + ".img"];

		if (!src)
			return;

		nl::node info = src["info"];

		hunger = info["hungry"];
		life = info["life"];
		permanent = info["permanent"].get_bool();

		nl::node interact = info["interact"];

		for (auto cmd : interact)
			commands.push_back(cmd.name());

		nl::node strsrc = nl::nx::string["Cash.img"][strid];
		name = (std::string)strsrc["name"];
		desc = (std::string)strsrc["desc"];

		valid = true;
	}

	bool PetData::is_valid() const
	{
		return valid;
	}

	PetData::operator bool() const
	{
		return valid;
	}

	const std::string& PetData::get_name() const
	{
		return name;
	}

	const std::string& PetData::get_desc() const
	{
		return desc;
	}

	int32_t PetData::get_hunger() const
	{
		return hunger;
	}

	int32_t PetData::get_life() const
	{
		return life;
	}

	bool PetData::is_permanent() const
	{
		return permanent;
	}

	const std::vector<std::string>& PetData::get_commands() const
	{
		return commands;
	}
}
