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
#include "UINotice.h"
#include "UIFamilyTree.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#include "../../Net/Packets/SocialPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIFamily::UIFamily() :
		UIDragElement<PosFAMILY>(Point<int16_t>(214, 27)),
		selected_ability(0),
		reputation(0),
		total_reputation(0),
		junior_page(0)
	{
		// v83 uses UIWindow2.img/Family — that node has proper
		// button origins so MapleButton positions itself from NX.
		nl::node src = nl::nx::ui["UIWindow2.img"]["Family"];

		// Three-layer backdrop: frame, inset panel, title bar.
		sprites.emplace_back(src["backgrnd"]);
		sprites.emplace_back(src["backgrnd2"]);
		sprites.emplace_back(src["backgrnd3"]);

		// UIWindow2 omits BtClose; the 12×12 at the top-right is
		// actually `BtFamilyPrecept`, which we repurpose as close.
		buttons[Buttons::BT_CLOSE]          = std::make_unique<MapleButton>(src["BtFamilyPrecept"]);
		buttons[Buttons::BT_FAMILY_PRECEPT] = std::make_unique<MapleButton>(src["BtFamilyPrecept"]);
		buttons[Buttons::BT_JUNIOR_ENTRY]   = std::make_unique<MapleButton>(src["BtJuniorEntry"]);
		buttons[Buttons::BT_TREE]           = std::make_unique<MapleButton>(src["BtTree"]);
		buttons[Buttons::BT_LEFT]           = std::make_unique<MapleButton>(src["BtLeft"]);
		buttons[Buttons::BT_RIGHT]          = std::make_unique<MapleButton>(src["BtRight"]);
		buttons[Buttons::BT_SPECIAL]        = std::make_unique<MapleButton>(src["BtSpecial"]);
		buttons[Buttons::BT_OK]             = std::make_unique<MapleButton>(src["BtOK"]);

		// The precept icon and close-icon share the same sprite and
		// overlap perfectly. Hide the dup so clicks go to BT_CLOSE.
		buttons[Buttons::BT_FAMILY_PRECEPT]->set_active(false);

		nl::node right_icon = src["RightIcon"];

		// The NX `type` sub-node holds the verbose v83 names which are
		// too long to fit below the 32x31 sprite — use short labels.
		static const char* short_names[NUM_ABILITIES] = {
			"Teleport to Member",
			"Summon Member",
			"Drop Rate Buff",
			"EXP Buff",
			"Drop + EXP Buff"
		};

		for (int i = 0; i < NUM_ABILITIES; i++)
		{
			std::string key = std::to_string(i);

			if (auto icon = right_icon[key])
				right_icons.emplace_back(icon);

			ability_names[i] = short_names[i];
		}

		title_label       = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Family");
		// The server's FamilyInfo packet sends family name (not the
		// leader's name) — label this accordingly and colour it black
		// so it reads on the inset panel. Server sends an empty string
		// when the player has no family yet.
		leader_label      = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, "(you do not have family name yet)");
		message_label     = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, "");
		senior_label      = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::WHITE, "Senior: None");
		rep_label         = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::BLACK, "0/0");
		today_rep_label   = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::BLACK, "+0");
		total_rep_label   = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::YELLOW, "Total Rep: 0");
		junior_label      = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::BLACK);
		ability_name_label= Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		ability_cost_label= Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		ability_uses_label= Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		ability_target_label  = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		ability_duration_label= Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		ability_effect_label  = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);

		refresh_ability_text();
		refresh_junior_text();

		dimension = Point<int16_t>(214, 343);
		dragarea = Point<int16_t>(214, 27);
	}

	void UIFamily::draw(float inter) const
	{
		UIElement::draw(inter);

		// Title in the 195x25 header bar (origin 10,27).
		title_label.draw(position + Point<int16_t>(107, 28));

		// Prefer the family motto at the top; if the leader hasn't set
		// one, fall back to the family name so the slot isn't empty.
		if (message_label.get_text().empty())
			leader_label.draw(position + Point<int16_t>(100, 50));
		else
			message_label.draw(position + Point<int16_t>(100, 50));

		// Senior & juniors panel — between the button row and the
		// ability icon. BtLeft/BtRight sit at y=181, and the icon
		// base is y=181 - 32(origin) = 149 ... 180.
		senior_label.draw(position + Point<int16_t>(20, 100));
		junior_label.draw(position + Point<int16_t>(62, 72));

		// Description above, sprite below, cost under the name.
		ability_name_label.draw(position + Point<int16_t>(107, 175));
		ability_cost_label.draw(position + Point<int16_t>(100, 192));
		ability_uses_label.draw(position + Point<int16_t>(190, 192));
		ability_target_label.draw(position + Point<int16_t>(12, 210));
		ability_duration_label.draw(position + Point<int16_t>(12, 226));
		ability_effect_label.draw(position + Point<int16_t>(12, 242));
		if (selected_ability >= 0 && selected_ability < (int)right_icons.size())
		{
			// Icon origin is (0,32) — Texture::draw subtracts origin,
			// so y=230 places the icon's top around y=198 on screen.
			right_icons[selected_ability].draw(position + Point<int16_t>(150, 250));
		}

		// Current rep right below the junior count (junior at y=72).
		// Format: current/total on one line, replacing the old Rep/Total
		// pair. total_rep_label is still updated but no longer drawn.
		rep_label.draw(position + Point<int16_t>(170, 92));
		today_rep_label.draw(position + Point<int16_t>(170, 111));
	}

	void UIFamily::update()
	{
		UIElement::update();
	}

	void UIFamily::set_family_info(const std::string& leader, int32_t rep, int32_t total_rep, int32_t todays_rep)
	{
		leader_name = leader;
		reputation = rep;
		total_reputation = total_rep;

		leader_label.change_text(leader.empty()
			? std::string("(you do not have family name yet)")
			: "Family: " + leader);
		rep_label.change_text(std::to_string(rep) + "/" + std::to_string(total_rep));
		total_rep_label.change_text("Total Rep: " + std::to_string(total_rep));
		today_rep_label.change_text("+" + std::to_string(todays_rep));
	}

	void UIFamily::set_senior(const std::string& name)
	{
		senior_name = name;
		senior_label.change_text("Senior: " + name);
	}

	void UIFamily::set_family_message(const std::string& message)
	{
		message_label.change_text(message);
	}

	void UIFamily::set_entitlement_usage(int ordinal, int times_used)
	{
		if (ordinal >= 0 && ordinal < NUM_ABILITIES)
		{
			ability_used[ordinal] = times_used;
			refresh_ability_text();
		}
	}

	void UIFamily::add_junior(const std::string& name)
	{
		juniors.push_back(name);
		refresh_junior_text();
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
		case Buttons::BT_OK:
			deactivate();
			break;

		case Buttons::BT_LEFT:
			cycle_ability(-1);
			break;

		case Buttons::BT_RIGHT:
			cycle_ability(+1);
			break;

		case Buttons::BT_SPECIAL:
		{
			// 0=teleport, 1=summon need a target-name string after the
			// int; buffs (2/3/4) are self-cast and just take the int.
			int ability = selected_ability;
			const bool needs_target = (ability == 0 || ability == 1);

			if (needs_target)
			{
				const char* prompt = (ability == 0)
					? "Enter the name of the family member to teleport to:"
					: "Enter the name of the family member to summon:";

				UI::get().emplace<UIEnterText>(
					prompt,
					[ability](const std::string& name)
					{
						if (!name.empty())
							FamilyUsePacket(ability, name).dispatch();
					},
					13);
			}
			else
			{
				FamilyUsePacket(ability).dispatch();
			}
			break;
		}

		case Buttons::BT_TREE:
			UI::get().emplace<UIFamilyTree>();
			break;

		case Buttons::BT_JUNIOR_ENTRY:
			// Shift both the text entry and the buttons up together so
			// the cursor doesn't get hidden; OK/Cancel also slide left.
			UI::get().emplace<UIEnterText>(
				"Enter the character name to invite as junior:",
				[](const std::string& name)
				{
					if (!name.empty())
						FamilyAddPacket(name).dispatch();
				},
				13,
				-22,  // field_y_offset — move text entry up
				-40,  // button_x_offset
				0);   // button_y_offset — buttons back down at default y
			break;

		case Buttons::BT_FAMILY_PRECEPT:
			// Dup of the close button sprite — kept inactive. Precept
			// editing goes through its own code path (no visible button
			// in this NX layout); callers can dispatch a
			// FamilyPreceptsPacket directly if needed.
			UI::get().emplace<UIEnterText>(
				"Enter the new family motto (leader only, max 200 chars):",
				[](const std::string& text)
				{
					if (!text.empty())
						FamilyPreceptsPacket(text).dispatch();
				},
				200);
			break;
		}

		return Button::State::NORMAL;
	}

	void UIFamily::cycle_ability(int delta)
	{
		if (right_icons.empty())
			return;

		int n = static_cast<int>(right_icons.size());
		selected_ability = (selected_ability + delta + n) % n;
		refresh_ability_text();
	}

	void UIFamily::refresh_ability_text()
	{
		// Costs in the 500–2500 range the user chose for this server.
		static const int costs[NUM_ABILITIES] = { 500, 1000, 1500, 1500, 2500 };
		// Per-day caps (Cosmic/v83 defaults — server only sends `used`).
		static const int caps[NUM_ABILITIES]  = { 3, 3, 1, 1, 1 };

		static const char* targets[NUM_ABILITIES] = {
			"Family Member",
			"Family Member",
			"Self + Party",
			"Self + Party",
			"Self + Party"
		};
		static const char* durations[NUM_ABILITIES] = {
			"Instant",
			"Instant",
			"15 min",
			"15 min",
			"15 min"
		};
		static const char* effects[NUM_ABILITIES] = {
			"Teleport to target",
			"Pull target to you",
			"+25% Drop Rate",
			"+25% EXP",
			"+25% Drop & EXP"
		};

		if (selected_ability >= 0 && selected_ability < NUM_ABILITIES)
		{
			ability_name_label.change_text(ability_names[selected_ability]);
			ability_cost_label.change_text(std::to_string(costs[selected_ability]));
			ability_uses_label.change_text(
				std::to_string(ability_used[selected_ability])
				+ "/" + std::to_string(caps[selected_ability]));
			ability_target_label.change_text(std::string("[target] ") + targets[selected_ability]);
			ability_duration_label.change_text(std::string("[time] ") + durations[selected_ability]);
			ability_effect_label.change_text(std::string("[effect] ") + effects[selected_ability]);
		}
		else
		{
			ability_name_label.change_text("");
			ability_cost_label.change_text("");
			ability_uses_label.change_text("");
			ability_target_label.change_text("");
			ability_duration_label.change_text("");
			ability_effect_label.change_text("");
		}
	}

	void UIFamily::refresh_junior_text()
	{
		junior_label.change_text(std::to_string(juniors.size()) + "/2");
	}
}
