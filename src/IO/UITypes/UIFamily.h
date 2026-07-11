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
		void set_family_info(const std::string& leader, int32_t rep, int32_t total_rep, int32_t todays_rep = 0);
		void set_entitlement_usage(int ordinal, int times_used);
		// Server-configured rep cost / daily use limit per entitlement
		// (FAMILY_PRIVILEGE_LIST). Static because the packet arrives at
		// login, before this window is first opened. Ordinals beyond the
		// five abilities shown here are ignored.
		static void set_entitlement_info(int ordinal, int32_t rep_cost, int32_t use_limit);
		void set_family_message(const std::string& message);
		void set_senior(const std::string& name);
		void add_junior(const std::string& name);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void cycle_ability(int delta);
		void refresh_ability_text();
		void refresh_junior_text();

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_TREE,
			BT_SPECIAL,
			BT_JUNIOR_ENTRY,
			BT_FAMILY_PRECEPT,
			BT_LEFT,
			BT_RIGHT,
			BT_OK,
			NUM_BUTTONS
		};

		static constexpr int NUM_ABILITIES = 5;
		static int32_t ability_costs[NUM_ABILITIES];
		static int32_t ability_caps[NUM_ABILITIES];
		std::vector<Texture> right_icons;
		std::string ability_names[NUM_ABILITIES];
		int ability_used[NUM_ABILITIES] = { 0, 0, 0, 0, 0 };
		int selected_ability;

		std::string leader_name;
		std::string senior_name;
		std::vector<std::string> juniors;
		int32_t reputation;
		int32_t total_reputation;
		int junior_page;

		mutable Text title_label;
		mutable Text leader_label;
		mutable Text message_label;
		mutable Text senior_label;
		mutable Text rep_label;
		mutable Text total_rep_label;
		mutable Text today_rep_label;
		mutable Text junior_label;
		mutable Text ability_name_label;
		mutable Text ability_cost_label;
		mutable Text ability_uses_label;
		mutable Text ability_target_label;
		mutable Text ability_duration_label;
		mutable Text ability_effect_label;
	};
}
