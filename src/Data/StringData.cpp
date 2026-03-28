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
#include "StringData.h"

#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace StringData
	{
		std::string get_mob_name(int32_t mobid)
		{
			return (std::string)nl::nx::string["Mob.img"][std::to_string(mobid)]["name"];
		}

		std::string get_mob_desc(int32_t mobid)
		{
			return (std::string)nl::nx::string["Mob.img"][std::to_string(mobid)]["desc"];
		}

		std::string get_npc_name(int32_t npcid)
		{
			return (std::string)nl::nx::string["Npc.img"][std::to_string(npcid)]["name"];
		}

		std::string get_npc_func(int32_t npcid)
		{
			return (std::string)nl::nx::string["Npc.img"][std::to_string(npcid)]["func"];
		}

		std::string get_map_name(int32_t mapid)
		{
			nl::node map_node = NxHelper::Map::get_map_node_name(mapid);

			if (map_node)
			{
				nl::node strsrc = nl::nx::string["Map.img"][map_node.name()][std::to_string(mapid)];
				return (std::string)strsrc["mapName"];
			}

			return "";
		}

		std::string get_map_street(int32_t mapid)
		{
			nl::node map_node = NxHelper::Map::get_map_node_name(mapid);

			if (map_node)
			{
				nl::node strsrc = nl::nx::string["Map.img"][map_node.name()][std::to_string(mapid)];
				return (std::string)strsrc["streetName"];
			}

			return "";
		}

		std::string get_map_desc(int32_t mapid)
		{
			nl::node map_node = NxHelper::Map::get_map_node_name(mapid);

			if (map_node)
			{
				nl::node strsrc = nl::nx::string["Map.img"][map_node.name()][std::to_string(mapid)];
				return (std::string)strsrc["mapDesc"];
			}

			return "";
		}

		std::string get_skill_name(int32_t skillid)
		{
			return (std::string)nl::nx::string["Skill.img"][std::to_string(skillid)]["name"];
		}

		std::string get_skill_desc(int32_t skillid)
		{
			return (std::string)nl::nx::string["Skill.img"][std::to_string(skillid)]["desc"];
		}
	}
}
