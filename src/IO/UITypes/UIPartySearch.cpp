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
#include "UIPartySearch.h"

#include "../Components/MapleButton.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPartySearch::UIPartySearch() : UIDragElement<PosPARTYSEARCH>(Point<int16_t>(300, 20)),
		is_searching(false), scroll_offset(0)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["PartySearch"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_START] = std::make_unique<MapleButton>(src["BtStart"]);
		buttons[Buttons::BT_STOP] = std::make_unique<MapleButton>(src["BtStop"]);
		buttons[Buttons::BT_REG] = std::make_unique<MapleButton>(src["BtReg"]);
		buttons[Buttons::BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);
		buttons[Buttons::BT_PAUSE] = std::make_unique<MapleButton>(src["BtPause"]);

		// Load party icons
		nl::node party = src["Party"];

		for (int i = 0; i < 4; i++)
			party_icons[i] = party["party" + std::to_string(i)];

		check_on = src["check0"];
		check_off = src["check1"];

		// Initial button states
		buttons[Buttons::BT_STOP]->set_active(false);
		buttons[Buttons::BT_PAUSE]->set_active(false);

		leader_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);
		info_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
		status_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, "Click Start to search for parties");

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIPartySearch::draw(float inter) const
	{
		UIElement::draw(inter);

		if (results.empty())
		{
			status_label.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2));
		}
		else
		{
			int16_t visible = static_cast<int16_t>(results.size()) - scroll_offset;

			if (visible > MAX_VISIBLE)
				visible = MAX_VISIBLE;

			for (int16_t i = 0; i < visible; i++)
			{
				int16_t idx = i + scroll_offset;

				if (idx >= static_cast<int16_t>(results.size()))
					break;

				const auto& entry = results[idx];
				int16_t row_y = 40 + i * 30;

				// Draw party icon based on member count
				int icon_idx = (entry.member_count > 3) ? 3 : entry.member_count;
				party_icons[icon_idx].draw(position + Point<int16_t>(10, row_y));

				leader_label.change_text(entry.leader);
				leader_label.draw(position + Point<int16_t>(40, row_y + 2));

				std::string info = std::to_string(entry.member_count) + "/" + std::to_string(entry.max_members) +
					" (Lv." + std::to_string(entry.min_level) + "-" + std::to_string(entry.max_level) + ")";
				info_label.change_text(info);
				info_label.draw(position + Point<int16_t>(130, row_y + 2));
			}
		}
	}

	void UIPartySearch::update()
	{
		UIElement::update();
	}

	void UIPartySearch::add_party(const std::string& leader, int8_t member_count, int8_t max_members, int16_t min_level, int16_t max_level)
	{
		results.push_back({ leader, member_count, max_members, min_level, max_level });
	}

	void UIPartySearch::clear_results()
	{
		results.clear();
		scroll_offset = 0;
	}

	void UIPartySearch::set_searching(bool searching)
	{
		is_searching = searching;

		buttons[Buttons::BT_START]->set_active(!searching);
		buttons[Buttons::BT_STOP]->set_active(searching);
		buttons[Buttons::BT_PAUSE]->set_active(searching);

		if (searching)
			status_label.change_text("Searching...");
		else
			status_label.change_text("Search stopped");
	}

	void UIPartySearch::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIPartySearch::get_type() const
	{
		return TYPE;
	}

	Button::State UIPartySearch::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_START:
			set_searching(true);
			break;
		case Buttons::BT_STOP:
			set_searching(false);
			break;
		case Buttons::BT_REG:
			// TODO: Register for auto-search
			break;
		case Buttons::BT_CANCEL:
			deactivate();
			break;
		case Buttons::BT_PAUSE:
			set_searching(false);
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
