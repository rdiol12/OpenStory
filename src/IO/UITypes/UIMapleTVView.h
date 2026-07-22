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

#include "../UIElement.h"

#include "../../Character/Look/CharLook.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Animation.h"
#include "../../Graphics/Texture.h"

namespace ms
{
	// On-air MapleTV overlay (MapleTV.img/TVbasic): the TV frame in the
	// top-right corner with the sender's character and the broadcast
	// lines, shown while a SEND_TV broadcast is live
	class UIMapleTVView : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAPLETVVIEW;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIMapleTVView();

		void draw(float inter) const override;
		void update() override;

		// Never intercept clicks — it's a pure overlay
		bool is_in_range(Point<int16_t> cursorpos) const override;

		UIElement::Type get_type() const override;

	private:
		Texture tv_frame;
		Animation tv_off;
		mutable Text line_text;
		mutable Text name_text;

		CharLook sender_look;
		bool has_look = false;
		int32_t seen_serial = -1;
		uint16_t line_tick = 0;
		size_t line_index = 0;
	};
}
