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
#include "MapleTVBroadcast.h"

#include "../Constants.h"

namespace ms
{
	void MapleTVBroadcast::start(const std::string& sender_name,
		const std::vector<std::string>& lines,
		const std::string& victim_name,
		int32_t duration_ms)
	{
		sender_name_ = sender_name;
		victim_name_ = victim_name;
		lines_ = lines;
		remaining_ms_ = duration_ms > 0 ? duration_ms : 15000;
		active_ = true;
	}

	void MapleTVBroadcast::stop()
	{
		active_ = false;
		sender_name_.clear();
		victim_name_.clear();
		lines_.clear();
		remaining_ms_ = 0;
	}

	void MapleTVBroadcast::update()
	{
		if (!active_) return;
		remaining_ms_ -= static_cast<int32_t>(Constants::TIMESTEP);
		if (remaining_ms_ <= 0)
			stop();
	}
}
