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

#include "../UIDragElement.h"

#include "../../Graphics/Texture.h"
#include "../../Graphics/Text.h"

namespace ms
{
	class UIFamily : public UIDragElement<PosFAMILY>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::FAMILY;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIFamily();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_family_info(const std::string& leader, int32_t rep, int32_t total_rep);
		void set_senior(const std::string& name);
		void add_junior(const std::string& name);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_TREE,
			BT_SPECIAL,
			BT_JUNIOR_ENTRY,
			BT_FAMILY_PRECEPT,
			BT_LEFT,
			BT_RIGHT,
			BT_OK
		};

		std::vector<Texture> right_icons;

		std::string leader_name;
		std::string senior_name;
		std::vector<std::string> juniors;
		int32_t reputation;
		int32_t total_reputation;

		mutable Text leader_label;
		mutable Text senior_label;
		mutable Text rep_label;
		mutable Text junior_label;
	};
}
