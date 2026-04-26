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
#include "NotificationCenter.h"

#include <algorithm>

namespace ms
{
	NotificationCenter& NotificationCenter::get()
	{
		static NotificationCenter instance;
		return instance;
	}

	int32_t NotificationCenter::push(std::string title, std::string body,
		std::function<void(bool)> resolver)
	{
		Entry e;
		e.id = next_id++;
		e.title = std::move(title);
		e.body = std::move(body);
		e.resolver = std::move(resolver);
		pending.push_back(std::move(e));
		return pending.back().id;
	}

	void NotificationCenter::resolve(int32_t id, bool yes)
	{
		auto it = std::find_if(pending.begin(), pending.end(),
			[id](const Entry& e) { return e.id == id; });
		if (it == pending.end()) return;

		auto resolver = std::move(it->resolver);
		pending.erase(it);
		if (resolver)
			resolver(yes);
	}

	void NotificationCenter::dismiss(int32_t id)
	{
		pending.erase(
			std::remove_if(pending.begin(), pending.end(),
				[id](const Entry& e) { return e.id == id; }),
			pending.end());
	}

	void NotificationCenter::clear()
	{
		pending.clear();
	}

	bool NotificationCenter::empty() const
	{
		return pending.empty();
	}

	const std::vector<NotificationCenter::Entry>& NotificationCenter::list() const
	{
		return pending;
	}
}
