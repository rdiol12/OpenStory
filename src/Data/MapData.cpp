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
#include "MapData.h"

#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	MapData::MapData(int32_t mapid)
	{
		valid = false;
		return_map = 999999999;
		forced_return = 999999999;
		field_limit = 0;
		mob_rate = 1.0f;
		town = false;
		swim = false;
		hide_minimap = false;

		std::string strid = string_format::extend_id(mapid, 9);
		std::string category = NxHelper::Map::get_map_category(mapid);

		nl::node src = nl::nx::map["Map"][category][strid + ".img"];

		if (!src)
			return;

		nl::node info = src["info"];

		bgm = (std::string)info["bgm"];
		return_map = info["returnMap"];
		forced_return = info["forcedReturn"];
		field_limit = info["fieldLimit"];
		mob_rate = info["mobRate"].get_real(1.0);
		town = info["town"].get_bool();
		swim = info["swim"].get_bool();
		hide_minimap = info["hideMinimap"].get_bool();
		map_mark = (std::string)info["mapMark"];

		nl::node map_node = NxHelper::Map::get_map_node_name(mapid);

		if (map_node)
		{
			nl::node strsrc = nl::nx::string["Map.img"][map_node.name()][std::to_string(mapid)];
			name = (std::string)strsrc["mapName"];
			street_name = (std::string)strsrc["streetName"];
		}

		nl::node portal_src = src["portal"];

		for (auto portal_node : portal_src)
		{
			Portal p;
			p.name = (std::string)portal_node["pn"];
			p.target_name = (std::string)portal_node["tn"];
			p.target_mapid = portal_node["tm"];
			p.type = (int8_t)portal_node["pt"].get_integer(0);
			p.x = portal_node["x"];
			p.y = portal_node["y"];
			portals.push_back(p);
		}

		nl::node life_src = src["life"];

		for (auto life_node : life_src)
		{
			LifeEntry l;
			l.id = life_node["id"];
			l.type = (std::string)life_node["type"];
			l.x = life_node["x"];
			l.y = life_node["y"];
			l.cy = life_node["cy"];
			l.fh = life_node["fh"];
			l.flip = life_node["f"].get_bool();
			l.rx0 = life_node["rx0"];
			l.rx1 = life_node["rx1"];
			l.mob_time = life_node["mobTime"];
			life.push_back(l);
		}

		valid = true;
	}

	bool MapData::is_valid() const
	{
		return valid;
	}

	MapData::operator bool() const
	{
		return valid;
	}

	const std::string& MapData::get_name() const
	{
		return name;
	}

	const std::string& MapData::get_street_name() const
	{
		return street_name;
	}

	const std::string& MapData::get_bgm() const
	{
		return bgm;
	}

	const std::string& MapData::get_map_mark() const
	{
		return map_mark;
	}

	int32_t MapData::get_return_map() const
	{
		return return_map;
	}

	int32_t MapData::get_forced_return() const
	{
		return forced_return;
	}

	int32_t MapData::get_field_limit() const
	{
		return field_limit;
	}

	float MapData::get_mob_rate() const
	{
		return mob_rate;
	}

	bool MapData::is_town() const
	{
		return town;
	}

	bool MapData::is_swim() const
	{
		return swim;
	}

	bool MapData::is_hide_minimap() const
	{
		return hide_minimap;
	}

	const std::vector<MapData::Portal>& MapData::get_portals() const
	{
		return portals;
	}

	const std::vector<MapData::LifeEntry>& MapData::get_life() const
	{
		return life;
	}
}
