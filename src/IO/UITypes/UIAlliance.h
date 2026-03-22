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
#include "../../Graphics/Text.h"
#include "../../Graphics/Geometry.h"

namespace ms
{
	class UIAlliance : public UIDragElement<PosALLIANCE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::ALLIANCE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIAlliance();

		void draw(float inter) const override;
		void update() override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		UIElement::Type get_type() const override;

		// Called from packet handlers
		void set_alliance_info(const std::string& name, int8_t rank, int8_t capacity);
		void add_guild(const std::string& guild_name, int32_t guild_id);
		void clear_guilds();
		void set_notice(const std::string& notice);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_INVITE,
			BT_LEAVE
		};

		struct AllianceGuild
		{
			std::string name;
			int32_t guild_id;
		};

		std::string alliance_name;
		std::string notice;
		int8_t rank;
		int8_t capacity;
		std::vector<AllianceGuild> guilds;

		Text title_label;
		Text notice_label;
		mutable Text guild_label;

		// Pre-allocated draw objects (avoid per-frame GPU allocations)
		ColorBox bg_box;
		ColorBox title_bar;
		Text close_text;
		Text name_text;
		Text cap_text;
		Text no_alliance_text;
		Text notice_header;
		Text guild_header;
		ColorBox invite_box;
		Text invite_text;
		ColorBox leave_box;
		Text leave_text;
	};
}
