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

#include "../Template/Singleton.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{
	// Tracks the world-wide MapleTV broadcast that's currently on the air.
	// Map-embedded TV sprites read this state to decide what avatar/message
	// to composite onto themselves.
	class MapleTVBroadcast : public Singleton<MapleTVBroadcast>
	{
	public:
		void start(const std::string& sender_name,
			const std::vector<std::string>& lines,
			const std::string& victim_name,
			int32_t duration_ms);

		void stop();
		void update();

		bool active() const { return active_; }
		const std::string& sender_name() const { return sender_name_; }
		const std::string& victim_name() const { return victim_name_; }
		const std::vector<std::string>& lines() const { return lines_; }
		int32_t remaining_ms() const { return remaining_ms_; }

	private:
		bool active_ = false;
		std::string sender_name_;
		std::string victim_name_;
		std::vector<std::string> lines_;
		int32_t remaining_ms_ = 0;
	};
}
