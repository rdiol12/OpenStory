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
#include "UIPartyHelper.h"

#include "../UI.h"
#include "../../Gameplay/Stage.h"
#include "../../Character/Party.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		constexpr int16_t HEADER_H = 22;       // top backdrop strip
		constexpr int16_t ROW_H    = 16;       // per-member row height
		constexpr int16_t FOOTER_H = 8;        // bottom strip
		constexpr int16_t PANEL_W  = 168;      // matches QuestAlarm width
	}

	UIPartyHelper::UIPartyHelper()
		: UIDragElement<PosPARTYHELPER>(Point<int16_t>(PANEL_W, HEADER_H))
	{
		nl::node alarm = nl::nx::ui["UIWindow.img"]["QuestAlarm"];
		if (!alarm)
			alarm = nl::nx::ui["UIWindow2.img"]["QuestAlarm"];

		backgrnd_min    = Texture(alarm["backgrndmin"]);
		backgrnd_center = Texture(alarm["backgrndcenter"]);
		backgrnd_bottom = Texture(alarm["backgrndbottom"]);

		title     = Text(Text::Font::A12B, Text::Alignment::LEFT,
			Color::Name::WHITE, "Party", 0);
		row_label = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::WHITE, "", 0);

		dimension = Point<int16_t>(PANEL_W, HEADER_H);
		dragarea  = Point<int16_t>(PANEL_W, HEADER_H);
	}

	void UIPartyHelper::draw(float inter) const
	{
		const auto& members = Stage::get().get_player().get_party().get_members();
		if (members.empty()) return;

		// Header.
		backgrnd_min.draw(position);
		title.draw(position + Point<int16_t>(10, 4));

		// One row per member, drawn over a stretched center strip.
		int16_t row_count = static_cast<int16_t>(members.size());
		int16_t body_h    = row_count * ROW_H;

		backgrnd_center.draw(DrawArgument(
			position + Point<int16_t>(0, HEADER_H),
			Point<int16_t>(PANEL_W, body_h)));

		int32_t leader_cid =
			Stage::get().get_player().get_party().get_leader();
		int32_t my_cid =
			Stage::get().get_player().get_oid();

		for (int16_t i = 0; i < row_count; ++i)
		{
			const PartyMember& m = members[i];
			int16_t row_y = HEADER_H + i * ROW_H;

			// Leader prefix + self highlight so the panel reads at a
			// glance: ★ = leader, you = yellow, online = white,
			// offline = grey.
			std::string label = m.cid == leader_cid
				? "* " + m.name
				: "  " + m.name;

			Color::Name color = !m.online
				? Color::Name::DUSTYGRAY
				: (m.cid == my_cid
					? Color::Name::YELLOW
					: Color::Name::WHITE);

			row_label.change_color(color);
			row_label.change_text(label);
			row_label.draw(position + Point<int16_t>(10, row_y + 1));
		}

		// Footer cap.
		backgrnd_bottom.draw(position + Point<int16_t>(0, HEADER_H + body_h));
	}

	UIElement::Type UIPartyHelper::get_type() const
	{
		return TYPE;
	}
}
