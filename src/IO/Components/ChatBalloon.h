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

#include "MapleFrame.h"

#include "../../Graphics/Text.h"

#include <nlnx/node.hpp>

namespace ms
{
	class ChatBalloon
	{
	public:
		ChatBalloon(int8_t type);
		ChatBalloon(nl::node src);
		ChatBalloon();

		void draw(Point<int16_t> position) const;
		void update();

		void change_text(const std::string& text);
		void expire();

		// Resolves an inline `#v/#i/#q/#s/#f/#e<id>#` macro to its sprite.
		// Shared so the chat window, speech balloons and NPC dialogue cannot
		// disagree about what a given macro renders as.
		static Texture resolve_inline_image(Text::Layout::ImageKind kind, int32_t id);
		// Draws every inline image of `label`, whose text was drawn at `origin`.
		static void draw_inline_images(const Text& label, Point<int16_t> origin,
			const Range<int16_t>& clip);

	private:
		// How long a line stays on screen
		static constexpr int16_t DURATION = 4000; // 4 seconds

		MapleFrame frame;
		Text textlabel;
		Texture arrow;
		int16_t duration;
	};

	class ChatBalloonHorizontal
	{
	public:
		ChatBalloonHorizontal();

		void draw(Point<int16_t> position) const;

		void change_text(const std::string& text);

	private:
		Text textlabel;
		Texture arrow;
		Texture center;
		Texture east;
		Texture northeast;
		Texture north;
		Texture northwest;
		Texture west;
		Texture southwest;
		Texture south;
		Texture southeast;
		int16_t xtile;
		int16_t ytile;
	};
}