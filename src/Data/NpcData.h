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

namespace ms
{
	class NpcData : public Cache<NpcData>
	{
	public:
		bool is_valid() const;
		explicit operator bool() const;

		const std::string& get_name() const;
		const std::string& get_func() const;
		const std::vector<std::string>& get_speak_lines() const;
		bool get_hide_name() const;
		bool get_talk_mouse_only() const;
		bool is_shop() const;
		const std::string& get_script() const;
		int32_t get_link() const;

	private:
		friend Cache<NpcData>;
		NpcData(int32_t npcid);

		bool valid;
		std::string name;
		std::string func;
		std::string script;
		std::vector<std::string> speak_lines;
		int32_t link;
		bool hide_name;
		bool talk_mouse_only;
		bool shop;
	};
}
