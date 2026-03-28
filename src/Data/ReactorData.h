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
	class ReactorData : public Cache<ReactorData>
	{
	public:
		struct StateEvent
		{
			int32_t type;
			int32_t item_id;
			int32_t quantity;
			int16_t lt_x;
			int16_t lt_y;
			int16_t rb_x;
			int16_t rb_y;
		};

		struct State
		{
			std::vector<StateEvent> events;
			bool hittable;
		};

		bool is_valid() const;
		explicit operator bool() const;

		int32_t get_num_states() const;
		const State& get_state(int32_t index) const;

	private:
		friend Cache<ReactorData>;
		ReactorData(int32_t reactorid);

		bool valid;
		std::vector<State> states;
	};
}
