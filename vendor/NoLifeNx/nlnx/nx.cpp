//////////////////////////////////////////////////////////////////////////////
// NoLifeNx - Part of the NoLifeStory project                               //
// Copyright © 2013 Peter Atashian                                          //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#include "nx.hpp"
#include "file.hpp"
#include "node.hpp"

#include <fstream>
#include <vector>
#include <memory>
#include <stdexcept>

namespace nl
{
	namespace nx
	{
		std::vector<std::unique_ptr<file>> files;

		bool exists(std::string name)
		{
			return std::ifstream(name).is_open();
		}

		node add_file(std::string name)
		{
			if (!exists(name))
				return {};

			files.emplace_back(new file(name));

			return *files.back();
		}

		node base, character, effect, etc, item, map, mapPretty, mapLatest, map001, mob, morph, npc, quest, reactor, skill, sound, string, tamingmob, ui;

		void load_all(std::string directory)
		{
			if (!directory.empty() && directory.back() != '/') directory += '/';
			const auto load = [&](const char* name) { return add_file(directory + name); };
			if (exists(directory + "Base.nx"))
			{
				base = load("Base.nx");
				character = load("Character.nx");
				effect = load("Effect.nx");
				etc = load("Etc.nx");
				item = load("Item.nx");
				map = load("Map.nx");
				mapPretty = load("MapPretty.nx");
				mapLatest = load("MapLatest.nx");
				map001 = load("Map001.nx");
				mob = load("Mob.nx");
				morph = load("Morph.nx");
				npc = load("Npc.nx");
				quest = load("Quest.nx");
				reactor = load("Reactor.nx");
				skill = load("Skill.nx");
				sound = load("Sound.nx");
				string = load("String.nx");
				tamingmob = load("TamingMob.nx");
				ui = load("UI.nx");
			}
			else if (exists(directory + "Data.nx"))
			{
				base = load("Data.nx");
				character = base["Character"];
				effect = base["Effect"];
				etc = base["Etc"];
				item = base["Item"];
				map = base["Map"];
				mob = base["Mob"];
				morph = base["Morph"];
				npc = base["Npc"];
				quest = base["Quest"];
				reactor = base["Reactor"];
				skill = base["Skill"];
				sound = base["Sound"];
				string = base["String"];
				tamingmob = base["TamingMob"];
				ui = base["UI"];
			}
			else
			{
				throw std::runtime_error("Failed to locate nx files.");
			}
		}
	}
}
