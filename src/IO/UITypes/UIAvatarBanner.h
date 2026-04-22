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
#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <memory>
#include <string>
#include <vector>

namespace ms
{
	// On-screen receiver banner for avatar megaphones. Each 539xxxx cash
	// item maps to one of the animated variants at
	// Map.nx/MapHelper.img/AvatarMegaphone/{Bright,Burning,Heart,Tiger1,Tiger2}.
	// Frames are ~225x121; the banner plays the loop at the right side of
	// the screen, overlays the sender name bar, and auto-expires.
	class UIAvatarBanner : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::AVATARBANNER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIAvatarBanner();

		void show(const std::string& sender,
			const std::vector<std::string>& lines,
			int32_t itemid,
			int32_t duration_ms,
			const LookEntry& sender_look);

		void draw(float inter) const override;
		void update() override;

		UIElement::Type get_type() const override;

	private:
		Animation variant_anim;
		Texture name_bar;
		Text sender_text;
		std::vector<Text> line_texts;

		// Sender's character is rendered live from a CharLook built from
		// the parsed LookEntry in the SET_AVATAR_MEGAPHONE packet.
		std::unique_ptr<CharLook> sender_look;

		int32_t remaining_ms;
	};
}
