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
#include "UIFamily.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIFamily::UIFamily() : UIDragElement<PosFAMILY>(), reputation(0), total_reputation(0)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["Family"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"]);
		buttons[Buttons::BT_TREE] = std::make_unique<MapleButton>(src["BtTree"]);
		buttons[Buttons::BT_SPECIAL] = std::make_unique<MapleButton>(src["BtSpecial"]);
		buttons[Buttons::BT_JUNIOR_ENTRY] = std::make_unique<MapleButton>(src["BtJuniorEntry"]);
		buttons[Buttons::BT_FAMILY_PRECEPT] = std::make_unique<MapleButton>(src["BtFamilyPrecept"]);
		buttons[Buttons::BT_LEFT] = std::make_unique<MapleButton>(src["BtLeft"]);
		buttons[Buttons::BT_RIGHT] = std::make_unique<MapleButton>(src["BtRight"]);
		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(src["BtOK"]);

		nl::node right_icon = src["RightIcon"];

		for (uint8_t i = 0; i < 5; i++)
		{
			nl::node icon = right_icon[std::to_string(i)];

			if (icon)
				right_icons.emplace_back(icon);
		}

		leader_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Leader: None");
		senior_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Senior: None");
		rep_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW, "Rep: 0 / 0");
		junior_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UIFamily::draw(float inter) const
	{
		UIElement::draw(inter);

		leader_label.draw(position + Point<int16_t>(20, 40));
		senior_label.draw(position + Point<int16_t>(20, 58));
		rep_label.draw(position + Point<int16_t>(20, 76));

		// Draw junior list
		int16_t y = 100;

		for (const auto& junior : juniors)
		{
			junior_label.change_text("Junior: " + junior);
			junior_label.draw(position + Point<int16_t>(20, y));
			y += 16;
		}
	}

	void UIFamily::update()
	{
		UIElement::update();
	}

	void UIFamily::set_family_info(const std::string& leader, int32_t rep, int32_t total_rep)
	{
		leader_name = leader;
		reputation = rep;
		total_reputation = total_rep;

		leader_label.change_text("Leader: " + leader);
		rep_label.change_text("Rep: " + std::to_string(rep) + " / " + std::to_string(total_rep));
	}

	void UIFamily::set_senior(const std::string& name)
	{
		senior_name = name;
		senior_label.change_text("Senior: " + name);
	}

	void UIFamily::add_junior(const std::string& name)
	{
		juniors.push_back(name);
	}

	void UIFamily::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIFamily::get_type() const
	{
		return TYPE;
	}

	Button::State UIFamily::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_TREE:
			FamilyUsePacket(0).dispatch();
			break;
		case Buttons::BT_SPECIAL:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line("Family special abilities are level-dependent.", UIChatBar::YELLOW);
			break;
		case Buttons::BT_JUNIOR_ENTRY:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line("Type /junior [name] to invite a junior member.", UIChatBar::YELLOW);
			break;
		case Buttons::BT_FAMILY_PRECEPT:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line(leader_name + " - Family precepts are set by the leader.", UIChatBar::YELLOW);
			break;
		case Buttons::BT_LEFT:
		case Buttons::BT_RIGHT:
			break;
		case Buttons::BT_OK:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
