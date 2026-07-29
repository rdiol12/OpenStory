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

#include "../../Graphics/Animation.h"
#include "Layer.h"

#include <vector>

namespace ms
{
	class MapEffect
	{
	public:
		MapEffect(std::string path);
		MapEffect();

		void draw() const;
		void update();

	private:
		bool active;
		Animation effect;
		Point<int16_t> position;
	};

	// One-shot animations anchored to map coordinates.
	//
	// MapEffect above is a single screen-space overlay -- its draw() takes no
	// view offset, so it cannot follow the world. These do: they stack, scroll
	// with the camera, and delete themselves when the animation finishes.
	//
	// This is what "something happens at a point" needs. The only projectile the
	// client had, Bullet, homes on a mob oid and tracks that mob's head every
	// frame, so it cannot express a blast at a bare coordinate -- which is why
	// thrown grenades rendered nothing at all.
	class MapPointEffects
	{
	public:
		// Plays `src` (an NX animation node) once at a map position.
		// Ignored when the node is missing, so a skill with no authored effect
		// silently does nothing rather than drawing a placeholder.
		void add(nl::node src, Point<int16_t> position,
			int8_t layer = Layer::Id::SEVEN, bool flip = false);
		// Same, resolved from an Effect.wz-style path (e.g. "BasicEff.img/hit").
		void add(const std::string& path, Point<int16_t> position,
			int8_t layer = Layer::Id::SEVEN, bool flip = false);

		void draw(int8_t layer, double viewx, double viewy, float alpha) const;
		void update();
		void clear();

	private:
		struct Entry
		{
			Animation animation;
			Point<int16_t> position;
			int8_t layer;
			bool flip;
		};

		std::vector<Entry> entries;
	};
}