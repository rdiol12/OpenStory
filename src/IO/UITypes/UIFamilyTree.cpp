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
#include "UIFamilyTree.h"

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UINotice.h"
#include "../../Character/Job.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/SocialPackets.h"
#include "../../Graphics/DrawArgument.h"

#include <cmath>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIFamilyTree::UIFamilyTree() :
		UIDragElement<PosFAMILYTREE>(Point<int16_t>(578, 27))
	{
		// UIWindow2 has origins baked in; UIWindow as fallback for the
		// plate/selected sprites (which UIWindow2 just sources from it).
		nl::node src = nl::nx::ui["UIWindow2.img"]["FamilyTree"];

		sprites.emplace_back(src["backgrnd"]);

		buttons[Buttons::BT_CLOSE]        = std::make_unique<MapleButton>(src["BtClose"]);
		buttons[Buttons::BT_BYE]          = std::make_unique<MapleButton>(src["BtBye"]);
		buttons[Buttons::BT_JUNIOR_ENTRY] = std::make_unique<MapleButton>(src["BtJuniorEntry"]);
		buttons[Buttons::BT_LEFT]         = std::make_unique<MapleButton>(src["BtLeft"]);
		buttons[Buttons::BT_RIGHT]        = std::make_unique<MapleButton>(src["BtRight"]);

		// BtLeft/BtRight come with origin (0,0) in this NX — force
		// them to visible positions at the bottom-center of the window.
		buttons[Buttons::BT_LEFT]->set_position(Point<int16_t>(255, 360));
		buttons[Buttons::BT_RIGHT]->set_position(Point<int16_t>(310, 360));

		plate_leader = Texture(src["PlateLeader"]["0"]);
		plate_others = Texture(src["PlateOthers"]["0"]);
		selected     = Texture(src["selected"]);

		name_label  = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);
		level_label = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::WHITE);
		job_label   = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::WHITE);
		size_label  = Text(Text::Font::A11M, Text::Alignment::LEFT,   Color::Name::BLACK);
		family_name_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "");

		dimension = Point<int16_t>(578, 386);
		dragarea  = Point<int16_t>(578, 27);

		// Ask the server for the pedigree chart using our own cid;
		// FamilyChartResultHandler will call back into add_entry.
		int32_t my_cid = Stage::get().get_player().get_oid();
		current_viewed_id = my_cid;
		FamilyPedigreeRequestPacket(my_cid).dispatch();
	}

	void UIFamilyTree::draw(float inter) const
	{
		UIElement::draw(inter);

		// Rebuild clickable plate hitboxes every frame so pedigree
		// navigation always matches what's actually drawn.
		plate_hitboxes.clear();

		// Family member count (top-left) and family name (top-center).
		size_label.change_text(std::to_string(entries.size()));
		size_label.draw(position + Point<int16_t>(55, 32));

		family_name_label.draw(position + Point<int16_t>(289, 1));

		// Three-row layout approximating the v83 family tree:
		//   row 0 (top)   → leader plate
		//   row 1 (mid)   → the viewed character ("me"), offset right
		//   row 2 (below) → juniors, spread left-to-right
		constexpr int16_t plate_w = 134;
		constexpr int16_t col_w = 140;
		constexpr int16_t leader_y = 39;
		constexpr int16_t grand_senior_y = 95;
		constexpr int16_t senior_y = 145;
		constexpr int16_t me_y = 200;
		constexpr int16_t juniors_y = 248;

		auto draw_plate = [&](const Entry& e, int16_t x, int16_t y, bool leader)
		{
			int16_t ph = leader ? 36 : 34;
			Rectangle<int16_t> rect(position + Point<int16_t>(x, y),
				position + Point<int16_t>(x + plate_w, y + ph));
			plate_hitboxes.emplace_back(e.cid, rect);

			bool hovered = rect.contains(UI::get().get_cursor_position());

			if (leader)
				plate_leader.draw(position + Point<int16_t>(x, y));
			else
				plate_others.draw(position + Point<int16_t>(x, y));

			// Hover highlight pulses between ~0.25 and ~0.75 alpha so
			// the plate visibly breathes while the cursor is over it.
			if (hovered && !e.is_viewed)
			{
				float phase = (tick % 60) / 60.0f;                // 0..1
				float alpha = 0.25f + 0.25f * (1.0f + std::sin(phase * 6.2831853f));
				selected.draw(DrawArgument(
					position + Point<int16_t>(x - 3, y - 3), alpha));
			}

			if (e.is_viewed)
				selected.draw(position + Point<int16_t>(x - 3, y - 3));

			int16_t cx = x + plate_w / 2;

			name_label.change_text(e.name);
			name_label.draw(position + Point<int16_t>(cx, y - 3));

			// Level + job on the same row below the name.
			Job job_lookup(static_cast<uint16_t>(e.job));
			std::string lv_text = "Lv." + std::to_string(e.level);
			level_label.change_text(lv_text);
			// 1-px spacing between the name row and the Lv/job row.
			level_label.draw(position + Point<int16_t>(x + 14, y + 6));

			job_label.change_text(job_lookup.get_name());
			job_label.draw(position + Point<int16_t>(x + 60, y + 6));
		};

		int16_t leader_x = (578 - plate_w) / 2 + 2;

		// Figure out the leader, the viewed ("me"), and me's senior.
		// Leftovers (not leader / not viewed / not senior / not a
		// direct junior) are ignored in this 4-row layout.
		const Entry* leader = nullptr;
		const Entry* viewed = nullptr;
		const Entry* senior = nullptr;
		for (const auto& e : entries)
		{
			bool looks_like_leader = (leader_id != 0)
				? (e.cid == leader_id)
				: (e.parent_id == 0);
			if (!leader && looks_like_leader)
				leader = &e;
			if (!viewed && e.is_viewed)
				viewed = &e;
		}
		if (!leader && !entries.empty())
			leader = &entries[0];

		// Senior = entry whose cid == viewed.parent_id. Don't draw it
		// twice if the senior IS the leader (viewed is a direct child
		// of the leader).
		if (viewed && viewed->parent_id != 0)
		{
			for (const auto& e : entries)
			{
				if (e.cid == viewed->parent_id)
				{
					if (leader && e.cid == leader->cid)
						senior = nullptr;
					else
						senior = &e;
					break;
				}
			}
		}

		// Sibling = another child of my senior (same senior, different
		// cid from viewed). Only the first one is drawn; v83 caps a
		// senior at 2 juniors so in practice there's at most one.
		const Entry* sibling = nullptr;
		if (viewed && senior)
		{
			for (const auto& e : entries)
			{
				if (e.cid == viewed->cid) continue;
				if (e.parent_id == senior->cid)
				{
					sibling = &e;
					break;
				}
			}
		}

		// Grand-senior = senior's parent. Skip if it IS the leader
		// (already shown in the top row).
		const Entry* grand_senior = nullptr;
		if (senior && senior->parent_id != 0)
		{
			for (const auto& e : entries)
			{
				if (e.cid == senior->parent_id)
				{
					if (leader && e.cid == leader->cid)
						grand_senior = nullptr;
					else
						grand_senior = &e;
					break;
				}
			}
		}

		// Juniors = direct children of viewed (parent_id == viewed.cid).
		std::vector<const Entry*> juniors;
		if (viewed)
		{
			for (const auto& e : entries)
			{
				if (e.cid == viewed->cid) continue;
				if (e.parent_id == viewed->cid)
					juniors.push_back(&e);
			}
		}

		if (leader)
			draw_plate(*leader, leader_x, leader_y, true);

		// Grand-senior (senior's senior) between leader and senior.
		if (grand_senior)
			draw_plate(*grand_senior, leader_x, grand_senior_y, false);

		// Senior plate centred between grand-senior and me.
		if (senior)
			draw_plate(*senior, leader_x, senior_y, false);

		// "Me" (viewed) centred under the leader — no plate background,
		// just the selected highlight (if any) and the text labels.
		if (viewed)
		{
			int16_t x = leader_x;

			// Make "me" clickable too so you can re-center back on
			// yourself after navigating elsewhere.
			Rectangle<int16_t> me_rect(position + Point<int16_t>(x, me_y),
				position + Point<int16_t>(x + plate_w, me_y + 34));
			plate_hitboxes.emplace_back(viewed->cid, me_rect);

			if (viewed->is_viewed)
				selected.draw(position + Point<int16_t>(x - 3, me_y - 3));

			int16_t cx = x + plate_w / 2;

			name_label.change_text(viewed->name);
			name_label.draw(position + Point<int16_t>(cx, me_y - 3));

			Job job_lookup(static_cast<uint16_t>(viewed->job));
			level_label.change_text("Lv." + std::to_string(viewed->level));
			level_label.draw(position + Point<int16_t>(x + 14, me_y + 6));

			job_label.change_text(job_lookup.get_name());
			job_label.draw(position + Point<int16_t>(x + 60, me_y + 6));
		}

		// Sibling (my senior's other junior) drawn right next to me
		// with a 5-px gap between the two plates.
		if (sibling)
			draw_plate(*sibling, leader_x + plate_w + 5, me_y, false);

		// Helper: draw up to 2 grand-juniors (a junior's own juniors)
		// directly below the junior's plate centered at jx.
		constexpr int16_t grand_y_offset = 56;
		auto draw_grand_juniors = [&](const Entry* junior, int16_t jx)
		{
			if (!junior) return;

			std::vector<const Entry*> grand;
			for (const auto& e : entries)
				if (e.parent_id == junior->cid)
					grand.push_back(&e);

			int16_t gy = juniors_y + grand_y_offset;
			if (grand.size() == 1)
			{
				draw_plate(*grand[0], jx, gy, false);
			}
			else if (grand.size() >= 2)
			{
				// Each grand-junior pair stays centred under its own
				// junior with a 5-px gap between the two plates.
				int16_t gap = 5;
				draw_plate(*grand[0], jx - plate_w / 2 - gap / 2, gy, false);
				draw_plate(*grand[1], jx + plate_w / 2 + gap / 2, gy, false);
			}
		};

		int n = (int)juniors.size();
		if (n == 1)
		{
			int16_t jx = (578 - plate_w) / 2;
			draw_plate(*juniors[0], jx, juniors_y, false);
			draw_grand_juniors(juniors[0], jx);
		}
		else if (n == 2)
		{
			int16_t jx0 = 80;
			int16_t jx1 = 578 - 80 - plate_w;
			draw_plate(*juniors[0], jx0, juniors_y, false);
			draw_plate(*juniors[1], jx1, juniors_y, false);
			draw_grand_juniors(juniors[0], jx0);
			draw_grand_juniors(juniors[1], jx1);
		}
		else
		{
			int16_t row_w = n * col_w;
			int16_t x0 = (578 - row_w) / 2;
			for (int i = 0; i < n; i++)
			{
				int16_t jx = x0 + i * col_w;
				draw_plate(*juniors[i], jx, juniors_y, false);
				draw_grand_juniors(juniors[i], jx);
			}
		}
	}

	void UIFamilyTree::update()
	{
		UIElement::update();
		++tick;
	}

	Cursor::State UIFamilyTree::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Let UIDragElement handle title-bar drag first.
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);
		if (dragged)
			return dstate;

		// Plate hitboxes: hover shows CANGRAB, click re-centres the
		// tree on that member via a fresh pedigree request.
		for (const auto& [cid, rect] : plate_hitboxes)
		{
			if (rect.contains(cursorpos))
			{
				if (clicked)
				{
					if (cid != current_viewed_id)
					{
						current_viewed_id = cid;
						FamilyPedigreeRequestPacket(cid).dispatch();
					}
					return Cursor::State::CLICKING;
				}
				return Cursor::State::CANCLICK;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIFamilyTree::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	UIElement::Type UIFamilyTree::get_type() const
	{
		return TYPE;
	}

	void UIFamilyTree::clear_entries()
	{
		entries.clear();
	}

	void UIFamilyTree::set_leader_id(int32_t id)
	{
		leader_id = id;
	}

	void UIFamilyTree::set_family_name(const std::string& name)
	{
		family_name_label.change_text(name);
	}

	void UIFamilyTree::add_entry(int32_t cid, int32_t parent_id, int32_t job,
		int32_t level, bool online, int32_t rep, int32_t total_rep,
		int32_t reps_to_senior, int32_t todays_rep, int32_t channel,
		int32_t time_online, const std::string& name, bool is_viewed)
	{
		Entry e;
		e.cid = cid;
		e.parent_id = parent_id;
		e.job = job;
		e.level = level;
		e.online = online;
		e.rep = rep;
		e.total_rep = total_rep;
		e.todays_rep = todays_rep;
		e.name = name;
		e.is_viewed = is_viewed;
		entries.push_back(e);
	}

	Button::State UIFamilyTree::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;

		case Buttons::BT_BYE:
		{
			// Leave the family — confirm first so it's not one-click.
			UI::get().emplace<UIYesNo>(
				"Do you really want to leave your family?",
				[](bool yes)
				{
					if (yes)
						FamilySeparateLeavePacket().dispatch();
				});
			break;
		}

		case Buttons::BT_JUNIOR_ENTRY:
		{
			// Ask for a character name, then dispatch ADD_FAMILY.
			UI::get().emplace<UIEnterText>(
				"Enter the character name to invite as junior:",
				[](const std::string& name)
				{
					if (!name.empty())
						FamilyAddPacket(name).dispatch();
				},
				13);
			break;
		}

		case Buttons::BT_LEFT:
		case Buttons::BT_RIGHT:
			// Navigation now happens by clicking a member's plate;
			// the arrows are kept visible for layout parity but are
			// no-ops so they don't fight with plate-click pivot.
			break;
		}
		return Button::State::NORMAL;
	}
}
