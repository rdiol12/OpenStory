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
#include "ReactorData.h"

#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	ReactorData::ReactorData(int32_t reactorid)
	{
		valid = false;

		std::string strid = string_format::extend_id(reactorid, 7);
		nl::node src = nl::nx::reactor[strid + ".img"];

		if (!src)
			return;

		for (int32_t i = 0; nl::node state_node = src[std::to_string(i)]; i++)
		{
			State state;
			state.hittable = false;

			nl::node event_node = state_node["event"];

			if (event_node)
			{
				for (auto evt : event_node)
				{
					StateEvent se;
					se.type = evt["type"];
					se.item_id = evt["0"];
					se.quantity = evt["1"].get_integer(1);
					se.lt_x = evt["lt"]["x"];
					se.lt_y = evt["lt"]["y"];
					se.rb_x = evt["rb"]["x"];
					se.rb_y = evt["rb"]["y"];

					if (se.type == 0)
						state.hittable = true;

					state.events.push_back(se);
				}
			}

			if (!event_node && state_node["hit"])
				state.hittable = true;

			states.push_back(state);
		}

		valid = !states.empty();
	}

	bool ReactorData::is_valid() const
	{
		return valid;
	}

	ReactorData::operator bool() const
	{
		return valid;
	}

	int32_t ReactorData::get_num_states() const
	{
		return static_cast<int32_t>(states.size());
	}

	const ReactorData::State& ReactorData::get_state(int32_t index) const
	{
		if (index >= 0 && index < static_cast<int32_t>(states.size()))
			return states[index];

		static const State empty_state = { {}, false };
		return empty_state;
	}
}
