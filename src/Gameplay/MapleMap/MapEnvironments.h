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

#include "../Physics/PhysicsObject.h"
#include "../../Graphics/Animation.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{
	class MapEnvironment
	{
	public:
		MapEnvironment(nl::node src, const std::string& name);

		void draw(double viewx, double viewy, float alpha) const;
		void update();

		void set_mode(int32_t mode);
		const std::string& get_name() const;
		bool is_active() const;

	private:
		enum Type
		{
			NORMAL,
			HTILED,
			VTILED,
			TILED,
			HMOVEA,
			VMOVEA,
			HMOVEB,
			VMOVEB
		};

		static Type typebyid(int32_t id)
		{
			if (id >= NORMAL && id <= VMOVEB)
				return static_cast<Type>(id);

			return NORMAL;
		}

		void settype(Type type);

		int16_t VWIDTH;
		int16_t VHEIGHT;
		int16_t WOFFSET;
		int16_t HOFFSET;

		std::string name;
		Animation animation;
		bool animated;
		int16_t cx;
		int16_t cy;
		double rx;
		double ry;
		int16_t htile;
		int16_t vtile;
		float opacity;
		bool flipped;
		bool active;

		MovingObject moveobj;
	};

	class MapEnvironments
	{
	public:
		MapEnvironments(nl::node src);
		MapEnvironments();

		void draw(double viewx, double viewy, float alpha) const;
		void update();

		void toggle(const std::string& name, int32_t mode);
		void reset_all();

	private:
		std::vector<MapEnvironment> environments;
	};
}
