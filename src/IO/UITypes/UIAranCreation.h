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
	class UIAranCreation : public UICreationBase
	{
	public:
		UIAranCreation();

		void draw(float inter) const override;
		void update() override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void randomize_look();
		void set_row_buttons(bool active);

		std::vector<Sprite> sprites_lookboard;
		Texture sky;
		Texture cloud;
		float cloudfx;
		Texture nameboard;
		Text botname;
		Text gendername;
	};
}