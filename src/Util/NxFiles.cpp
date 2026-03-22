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
#include <unistd.h>

#import <Foundation/Foundation.h>

// Returns the directory containing NX files on iOS.
// Checks app bundle first (bundled resources), then Documents folder.
static std::string get_ios_nx_directory()
{
	// 1. Check inside the app bundle (for bundled NX files)
	NSString* bundlePath = [[NSBundle mainBundle] resourcePath];
	if (bundlePath)
	{
		std::string bundleDir = [bundlePath UTF8String];
		std::string testFile = bundleDir + "/Base.nx";
		struct stat st;
		if (stat(testFile.c_str(), &st) == 0)
			return bundleDir + "/";
	}

	// 2. Check the Documents directory (user-transferred via Files app)
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	if ([paths count] > 0)
	{
		std::string docs = [[paths firstObject] UTF8String];
		std::string testFile = docs + "/Base.nx";
		struct stat st;
		if (stat(testFile.c_str(), &st) == 0)
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

			if (nx_dir.empty())
				return Error(Error::Code::MISSING_FILE, "NX files not found in app bundle or Documents folder");

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