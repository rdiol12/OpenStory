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

#include <chrono>
#include <cstdint>
#include <map>
#include <unordered_set>

namespace ms
{
	// Tracks TalkToNPC requests and detects when the server fails to respond
	// (common on private servers that are missing NX quest data). Any NPC
	// whose talk request times out is flagged "unavailable" so UI code can
	// hide its quest indicator and move related quests to the
	// "Not Available" tab of the quest log.
	class NpcResponseTracker : public Singleton<NpcResponseTracker>
	{
	public:
		// Record that the client just sent a TalkToNPC packet for this NPC.
		void mark_pending(int32_t npcid);

		// Clear a pending entry because the server responded (dialog/shop).
		void clear_pending(int32_t npcid);

		// Expire stale pending entries. Returns true if anything was newly
		// flagged as unavailable during this call.
		bool tick();

		bool is_unavailable(int32_t npcid) const;

		const std::unordered_set<int32_t>& get_unavailable() const { return unavailable_; }

		// Monotonic revision that increments every time the unavailable set
		// changes. UI code can compare against a cached value to know when
		// to re-run filters.
		uint32_t revision() const { return revision_; }

		// Called when changing maps / logging out.
		void clear_all();

	private:
		using Clock = std::chrono::steady_clock;
		static constexpr auto TIMEOUT = std::chrono::milliseconds(3000);

		std::map<int32_t, Clock::time_point> pending_;
		std::unordered_set<int32_t> unavailable_;
		uint32_t revision_ = 0;
	};
}
