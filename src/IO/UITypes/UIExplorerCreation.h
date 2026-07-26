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

#include "UICreationBase.h"

namespace ms
{
	class UIExplorerCreation : public UICreationBase
	{
	public:
		UIExplorerCreation();

		void draw(float inter) const override;
		void update() override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void randomize_look();

		// Uniform 800x600 content scaling, centered in the view (same treatment
		// as UIWorldSelect / UICharSelect). lay() maps a design point to screen;
		// scl() scales an offset only.
		Point<int16_t> lay(int16_t x, int16_t y) const;
		// Sign content: uniform scale (no x/y distortion), anchored at the tuned
		// sign position, edges chained so stacked plates stay seam-free.
		Point<int16_t> slay(int16_t x, int16_t y) const;
		DrawArgument sign_args(const Texture& t, int16_t x, int16_t y) const;
		float ui_scale;
		float ui_scale_x;
		float ui_scale_y;

		std::vector<Sprite> sprites_lookboard;
		std::vector<std::pair<Texture, Point<int16_t>>> scenery;
		std::vector<std::pair<Texture, Point<int16_t>>> genderboard;
		Texture sky;
		Texture cloud;
		float cloudfx;
		Texture nameboard;
	};
}