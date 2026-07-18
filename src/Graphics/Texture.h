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

#include "DrawArgument.h"

#include <memory>
#include <string>
#include <vector>

#ifdef USE_NX
#include <nlnx/bitmap.hpp>
#endif

namespace ms
{
	// Represents a single image loaded from a of game data
	class Texture
	{
	public:
		Texture() {};
		Texture(nl::node source);
		~Texture() {};

		// Load a PNG from disk (AI-generated / custom art). Returns an invalid
		// Texture if the file is missing or unreadable. Pixels are cached by path
		// so the same file always maps to one atlas entry.
		static Texture from_file(const std::string& path, Point<int16_t> origin);
		// Wrap a raw BGRA buffer (synthesized art). The key identifies the image:
		// the first call for a key stores the pixels, later calls reuse them.
		static Texture from_pixels(const std::string& key, int16_t width, int16_t height, std::vector<uint8_t> bgra, Point<int16_t> origin);

		void draw(const DrawArgument& args) const;
		void draw(const DrawArgument& args, const Range<int16_t>& vertical) const;
		void shift(Point<int16_t> amount);

		bool is_valid() const;
		int16_t width() const;
		int16_t height() const;
		Point<int16_t> get_origin() const;
		Point<int16_t> get_dimensions() const;

	private:
		nl::bitmap bitmap;
		Point<int16_t> origin;
		Point<int16_t> dimensions;

		// Raw-pixel path for custom art: id keys the atlas slot, the shared BGRA
		// buffer stays alive so the atlas can re-upload after a clear.
		size_t rawid = 0;
		std::shared_ptr<std::vector<uint8_t>> rawpixels;
	};
}