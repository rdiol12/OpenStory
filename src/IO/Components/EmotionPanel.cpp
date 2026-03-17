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
#include "EmotionPanel.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	EmotionPanel::EmotionPanel()
	{
		nl::node src = nl::nx::ui["Emotion.img"];

		if (!src)
			return;

		for (auto iter : src)
		{
			emotions.emplace_back(iter);
			emotion_names.emplace_back(iter.name());
		}
	}

	void EmotionPanel::draw(Point<int16_t> position) const
	{
		int16_t x_offset = 0;

		for (size_t i = 0; i < emotions.size(); i++)
		{
			emotions[i].draw(DrawArgument(position + Point<int16_t>(x_offset, 0)), 1.0f);
			x_offset += 34;
		}
	}

	size_t EmotionPanel::count() const
	{
		return emotions.size();
	}

	Animation EmotionPanel::get_emotion(size_t index) const
	{
		if (index < emotions.size())
			return emotions[index];

		return Animation();
	}
}
