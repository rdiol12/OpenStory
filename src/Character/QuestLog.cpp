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
#include "QuestLog.h"

#include "../MapleStory.h"
#include "Inventory/Inventory.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#endif

namespace ms
{
	void QuestLog::add_started(int16_t qid, const std::string& qdata)
	{
		started[qid] = qdata;
	}

	bool QuestLog::is_completable(int16_t qid, const Inventory& inv) const
	{
		auto it = started.find(qid);
		if (it == started.end())
			return false;

		nl::node end_check = nl::nx::quest["Check.img"][std::to_string(qid)]["1"];
		if (!end_check)
			return false;

		bool has_any_req = false;

		for (auto item_node : end_check["item"])
		{
			int32_t itemid = item_node["id"];
			int32_t count = item_node["count"];
			if (itemid <= 0 || count <= 0)
				continue;
			has_any_req = true;
			if (inv.get_total_item_count(itemid) < count)
				return false;
		}

		const std::string& qdata = it->second;
		int mob_idx = 0;
		for (auto mob_node : end_check["mob"])
		{
			int32_t mobid = mob_node["id"];
			int32_t count = mob_node["count"];
			if (mobid <= 0 || count <= 0) { mob_idx++; continue; }
			has_any_req = true;

			int32_t killed = 0;
			size_t off = static_cast<size_t>(mob_idx) * 3;
			if (off + 3 <= qdata.size())
			{
				try { killed = std::stoi(qdata.substr(off, 3)); }
				catch (...) { killed = 0; }
			}
			if (killed < count)
				return false;
			mob_idx++;
		}

		return has_any_req;
	}

	void QuestLog::add_in_progress(int16_t qid, int16_t qidl, const std::string& qdata)
	{
		in_progress[qid] = make_pair(qidl, qdata);
	}

	void QuestLog::add_completed(int16_t qid, int64_t time)
	{
		started.erase(qid);
		in_progress.erase(qid);
		completed[qid] = time;
	}

	void QuestLog::forfeit(int16_t qid)
	{
		started.erase(qid);
		in_progress.erase(qid);
	}

	bool QuestLog::is_started(int16_t qid)
	{
		return started.count(qid) > 0;
	}

	int16_t QuestLog::get_last_started()
	{
		auto qend = started.end();
		qend--;

		return qend->first;
	}

	const std::map<int16_t, std::string>& QuestLog::get_started() const
	{
		return started;
	}

	const std::map<int16_t, int64_t>& QuestLog::get_completed() const
	{
		return completed;
	}
}
