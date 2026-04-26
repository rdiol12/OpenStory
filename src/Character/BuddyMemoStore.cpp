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
#include "BuddyMemoStore.h"

#include <fstream>

namespace ms
{
	namespace
	{
		constexpr const char* MEMO_PATH = "buddymemo.txt";
		constexpr size_t MAX_NICK_LEN    = 13;
		constexpr size_t MAX_CONTENT_LEN = 80;
	}

	BuddyMemoStore& BuddyMemoStore::get()
	{
		static BuddyMemoStore instance;
		static bool loaded = false;
		if (!loaded)
		{
			loaded = true;
			instance.load();
		}
		return instance;
	}

	const std::string& BuddyMemoStore::get_memo(int32_t cid) const
	{
		auto it = memos.find(cid);
		if (it == memos.end()) return empty_str;
		return it->second.nickname;
	}

	bool BuddyMemoStore::has_memo(int32_t cid) const
	{
		auto it = memos.find(cid);
		return it != memos.end() && !it->second.nickname.empty();
	}

	void BuddyMemoStore::set_memo(int32_t cid, const std::string& nickname)
	{
		std::string n = nickname;
		if (n.size() > MAX_NICK_LEN) n.resize(MAX_NICK_LEN);

		auto& e = memos[cid];
		e.nickname = n;

		// If both fields ended up empty, drop the entry.
		if (e.nickname.empty() && e.content.empty())
			memos.erase(cid);

		save();
	}

	void BuddyMemoStore::clear_memo(int32_t cid)
	{
		auto it = memos.find(cid);
		if (it == memos.end()) return;
		it->second.nickname.clear();
		if (it->second.content.empty())
			memos.erase(it);
		save();
	}

	const std::string& BuddyMemoStore::get_content(int32_t cid) const
	{
		auto it = memos.find(cid);
		if (it == memos.end()) return empty_str;
		return it->second.content;
	}

	void BuddyMemoStore::set_content(int32_t cid, const std::string& content)
	{
		std::string c = content;
		if (c.size() > MAX_CONTENT_LEN) c.resize(MAX_CONTENT_LEN);

		auto& e = memos[cid];
		e.content = c;

		if (e.nickname.empty() && e.content.empty())
			memos.erase(cid);

		save();
	}

	void BuddyMemoStore::set_both(int32_t cid, const std::string& nickname,
		const std::string& content)
	{
		std::string n = nickname;
		std::string c = content;
		if (n.size() > MAX_NICK_LEN)    n.resize(MAX_NICK_LEN);
		if (c.size() > MAX_CONTENT_LEN) c.resize(MAX_CONTENT_LEN);

		if (n.empty() && c.empty())
			memos.erase(cid);
		else
		{
			auto& e = memos[cid];
			e.nickname = n;
			e.content  = c;
		}
		save();
	}

	void BuddyMemoStore::load()
	{
		memos.clear();
		std::ifstream ifs(MEMO_PATH);
		if (!ifs) return;

		std::string line;
		while (std::getline(ifs, line))
		{
			// Format v2: cid|nickname|content
			// Format v1 (legacy): cid=nickname  — still readable so old
			//                     stores migrate transparently.
			size_t bar1 = line.find('|');
			if (bar1 != std::string::npos)
			{
				try
				{
					int32_t cid = std::stoi(line.substr(0, bar1));
					size_t bar2 = line.find('|', bar1 + 1);
					std::string nick = (bar2 == std::string::npos)
						? line.substr(bar1 + 1)
						: line.substr(bar1 + 1, bar2 - bar1 - 1);
					std::string content = (bar2 == std::string::npos)
						? std::string()
						: line.substr(bar2 + 1);

					if (nick.size()    > MAX_NICK_LEN)    nick.resize(MAX_NICK_LEN);
					if (content.size() > MAX_CONTENT_LEN) content.resize(MAX_CONTENT_LEN);

					if (!nick.empty() || !content.empty())
					{
						auto& e = memos[cid];
						e.nickname = nick;
						e.content  = content;
					}
				}
				catch (...) {}
				continue;
			}

			size_t eq = line.find('=');
			if (eq == std::string::npos) continue;
			try
			{
				int32_t cid = std::stoi(line.substr(0, eq));
				std::string nick = line.substr(eq + 1);
				if (nick.size() > MAX_NICK_LEN) nick.resize(MAX_NICK_LEN);
				if (!nick.empty())
					memos[cid].nickname = nick;
			}
			catch (...) {}
		}
	}

	void BuddyMemoStore::save() const
	{
		std::ofstream ofs(MEMO_PATH);
		if (!ofs) return;
		for (const auto& kv : memos)
			ofs << kv.first << "|" << kv.second.nickname
				<< "|" << kv.second.content << "\n";
	}
}
