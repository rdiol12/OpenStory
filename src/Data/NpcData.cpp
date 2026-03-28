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
#include "NpcData.h"

#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	NpcData::NpcData(int32_t npcid)
	{
		valid = false;
		link = 0;
		hide_name = false;
		talk_mouse_only = false;
		shop = false;

		std::string strid = string_format::extend_id(npcid, 7);
		nl::node src = nl::nx::npc[strid + ".img"];

		if (!src)
			return;

		nl::node info = src["info"];

		link = info["link"];
		hide_name = info["hideName"].get_bool();
		talk_mouse_only = info["talkMouseOnly"].get_bool();
		script = (std::string)info["script"];
		shop = info["shop"].get_bool();

		nl::node strsrc = nl::nx::string["Npc.img"][std::to_string(npcid)];
		name = (std::string)strsrc["name"];
		func = (std::string)strsrc["func"];

		nl::node speak_src = src["speak"];

		for (auto line : speak_src)
			speak_lines.push_back((std::string)line);

		valid = true;
	}

	bool NpcData::is_valid() const
	{
		return valid;
	}

	NpcData::operator bool() const
	{
		return valid;
	}

	const std::string& NpcData::get_name() const
	{
		return name;
	}

	const std::string& NpcData::get_func() const
	{
		return func;
	}

	const std::vector<std::string>& NpcData::get_speak_lines() const
	{
		return speak_lines;
	}

	bool NpcData::get_hide_name() const
	{
		return hide_name;
	}

	bool NpcData::get_talk_mouse_only() const
	{
		return talk_mouse_only;
	}

	bool NpcData::is_shop() const
	{
		return shop;
	}

	const std::string& NpcData::get_script() const
	{
		return script;
	}

	int32_t NpcData::get_link() const
	{
		return link;
	}
}
