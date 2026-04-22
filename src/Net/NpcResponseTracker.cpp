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
#include "NpcResponseTracker.h"

namespace ms
{
	void NpcResponseTracker::mark_pending(int32_t npcid)
	{
		if (npcid <= 0)
			return;

		// Re-enable an NPC that was previously flagged unavailable if the
		// player explicitly retries talking to it — the server might have
		// recovered or the data might be present in a later build.
		if (unavailable_.erase(npcid) > 0)
			++revision_;

		pending_[npcid] = Clock::now();
	}

	void NpcResponseTracker::clear_pending(int32_t npcid)
	{
		pending_.erase(npcid);

		if (unavailable_.erase(npcid) > 0)
			++revision_;
	}

	bool NpcResponseTracker::tick()
	{
		if (pending_.empty())
			return false;

		auto now = Clock::now();
		bool changed = false;

		for (auto it = pending_.begin(); it != pending_.end();)
		{
			if (now - it->second >= TIMEOUT)
			{
				if (unavailable_.insert(it->first).second)
					changed = true;

				it = pending_.erase(it);
			}
			else
			{
				++it;
			}
		}

		if (changed)
			++revision_;

		return changed;
	}

	bool NpcResponseTracker::is_unavailable(int32_t npcid) const
	{
		return unavailable_.count(npcid) > 0;
	}

	void NpcResponseTracker::clear_all()
	{
		pending_.clear();

		if (!unavailable_.empty())
		{
			unavailable_.clear();
			++revision_;
		}
	}
}
