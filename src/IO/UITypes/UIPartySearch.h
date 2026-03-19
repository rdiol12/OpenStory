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
	class UIPartySearch : public UIDragElement<PosPARTYSEARCH>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::PARTYSEARCH;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIPartySearch();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void add_party(const std::string& leader, int8_t member_count, int8_t max_members, int16_t min_level, int16_t max_level);
		void clear_results();
		void set_searching(bool searching);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_START,
			BT_STOP,
			BT_REG,
			BT_CANCEL,
			BT_PAUSE
		};

		struct PartyEntry
		{
			std::string leader;
			int8_t member_count;
			int8_t max_members;
			int16_t min_level;
			int16_t max_level;
		};

		std::vector<PartyEntry> results;
		bool is_searching;
		int16_t scroll_offset;

		Texture party_icons[4];
		Texture check_on, check_off;

		mutable Text leader_label;
		mutable Text info_label;
		mutable Text status_label;

		static constexpr int16_t MAX_VISIBLE = 6;
	};
}
