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

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{
	// Client-side cache of the account's full character list. Cosmic
	// only sends this during CharSelect (just before the user picks a
	// character) — once the channel server takes over, the data is
	// gone. We tee the CharlistHandler payload into this singleton so
	// the in-game UI (e.g. UIUserList's account-chars pane) can keep
	// referring to it after the world handoff.
	class AccountCharStore
	{
	public:
		struct Entry
		{
			int32_t cid = 0;
			std::string name;
		};

		static AccountCharStore& get();

		void set(std::vector<Entry> chars);
		void clear();
		const std::vector<Entry>& list() const;

	private:
		AccountCharStore() = default;
		std::vector<Entry> chars;
	};
}
