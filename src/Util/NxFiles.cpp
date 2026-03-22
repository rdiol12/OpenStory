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
#include "NxFiles.h"

#ifdef USE_NX
#include <fstream>

#include <nlnx/node.hpp>
#include <nlnx/nx.hpp>

#ifdef PLATFORM_IOS
#include <string>
#include <sys/stat.h>

static std::string get_ios_nx_directory()
{
	// Look in the app's Documents directory first (user can transfer files via iTunes/Files)
	const char* home = getenv("HOME");
	if (home)
	{
		std::string docs = std::string(home) + "/Documents";
		struct stat st;
		if (stat(docs.c_str(), &st) == 0)
			return docs + "/";
	}
	return "";
}
#endif

namespace ms
{
	namespace NxFiles
	{
		Error init()
		{
#ifdef PLATFORM_IOS
			std::string nx_dir = get_ios_nx_directory();

			// Change working directory to where NX files are
			if (!nx_dir.empty())
				chdir(nx_dir.c_str());
#endif

			for (auto filename : filenames)
				if (std::ifstream{ filename }.good() == false)
					return Error(Error::Code::MISSING_FILE, filename);

			try
			{
				nl::nx::load_all();
			}
			catch (const std::exception& ex)
			{
				static const std::string message = ex.what();

				return Error(Error::Code::NLNX, message.c_str());
			}

			return Error::Code::NONE;
		}
	}
}
#endif