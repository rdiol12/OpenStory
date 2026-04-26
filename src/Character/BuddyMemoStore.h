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
#include <unordered_map>

namespace ms
{
	// Client-side per-buddy memo store. Cosmic never sees memos —
	// they live on disk in wz/buddymemo.txt as `cid|nickname|content`
	// lines. Nickname is the short alias (≤13c) shown in place of the
	// real name in the buddy list; content is a longer free-form note
	// (~50c) shown in tooltips / context menus.
	class BuddyMemoStore
	{
	public:
		struct Entry
		{
			std::string nickname;
			std::string content;
		};

		static BuddyMemoStore& get();

		// --- Nickname (the displayed alias) ---
		// Returns the nickname for `cid` if set, otherwise an empty
		// string. Length is bounded to 13 chars at set time.
		const std::string& get_memo(int32_t cid) const;
		bool has_memo(int32_t cid) const;
		void set_memo(int32_t cid, const std::string& nickname);
		void clear_memo(int32_t cid);

		// --- Memo content (the longer note) ---
		const std::string& get_content(int32_t cid) const;
		void set_content(int32_t cid, const std::string& content);

		// Atomic write of both fields at once (used by UIAddBuddy on OK).
		void set_both(int32_t cid, const std::string& nickname,
			const std::string& content);

		void load();
		void save() const;

	private:
		BuddyMemoStore() = default;

		std::unordered_map<int32_t, Entry> memos;
		mutable std::string empty_str;
	};
}
