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
#include "UISkillBook.h"

#include "UIKeyConfig.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/SkillId.h"
#include "../../Data/JobData.h"
#include "../../Data/SkillData.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/PlayerPackets.h"
#include "../../Net/Packets/MessagingPackets.h"


#include <unordered_set>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UISkillBook::SkillIcon::SkillIcon(int32_t id) : skill_id(id) {}

	void UISkillBook::SkillIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		auto keyconfig = UI::get().get_element<UIKeyConfig>();
		Keyboard::Mapping mapping = Keyboard::Mapping(KeyType::SKILL, skill_id);

		if (remove)
			keyconfig->unstage_mapping(mapping);
		else
			keyconfig->stage_mapping(cursorposition, mapping);
	}

	Icon::IconType UISkillBook::SkillIcon::get_type()
	{
		return Icon::IconType::SKILL;
	}

	UISkillBook::MacroIcon::MacroIcon(int32_t i) : macro_index(i) {}

	void UISkillBook::MacroIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		auto keyconfig = UI::get().get_element<UIKeyConfig>();
		if (!keyconfig) return;
		Keyboard::Mapping mapping = Keyboard::Mapping(KeyType::MACRO, macro_index);

		if (remove)
			keyconfig->unstage_mapping(mapping);
		else
			keyconfig->stage_mapping(cursorposition, mapping);
	}

	UISkillBook::SkillDisplayMeta::SkillDisplayMeta(int32_t i, int32_t l) : id(i), level(l)
	{
		const SkillData& data = SkillData::get(id);

		Texture ntx = data.get_icon(SkillData::Icon::NORMAL);
		Texture dtx = data.get_icon(SkillData::Icon::DISABLED);
		Texture motx = data.get_icon(SkillData::Icon::MOUSEOVER);
		icon = std::make_unique<StatefulIcon>(std::make_unique<SkillIcon>(id), ntx, dtx, motx);

		std::string namestr = data.get_name();
		std::string levelstr = std::to_string(level);

		name_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, namestr);
		level_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR, levelstr);

		constexpr uint16_t MAX_NAME_WIDTH = 97;
		size_t overhang = 3;

		while (name_text.width() > MAX_NAME_WIDTH)
		{
			namestr.replace(namestr.end() - overhang, namestr.end(), "..");
			overhang += 1;

			name_text.change_text(namestr);
		}
	}

	void UISkillBook::SkillDisplayMeta::draw(const DrawArgument& args) const
	{
		icon->draw(args.getpos());
		name_text.draw(args + Point<int16_t>(38, -5));
		level_text.draw(args + Point<int16_t>(38, 13));
	}

	int32_t UISkillBook::SkillDisplayMeta::get_id() const
	{
		return id;
	}

	int32_t UISkillBook::SkillDisplayMeta::get_level() const
	{
		return level;
	}

	StatefulIcon* UISkillBook::SkillDisplayMeta::get_icon() const
	{
		return icon.get();
	}

	UISkillBook::UISkillBook(const CharStats& in_stats, const SkillBook& in_skillbook) : UIDragElement<PosSKILL>(Point<int16_t>(174, 20)), stats(in_stats), skillbook(in_skillbook), grabbing(false), tab(0), macro_enabled(false)
	{
		nl::node Skill = nl::nx::ui["UIWindow2.img"]["Skill"];
		nl::node main = Skill["main"];
		nl::node ui_backgrnd = main["backgrnd"];

		bg_dimensions = Texture(ui_backgrnd).get_dimensions();

		skilld = main["skill0"];
		skille = main["skill1"];
		skillb = main["skillBlank"];
		line = main["line"];

		buttons[Buttons::BT_HYPER] = std::make_unique<MapleButton>(main["BtHyper"]);
		buttons[Buttons::BT_GUILDSKILL] = std::make_unique<MapleButton>(main["BtGuildSkill"]);
		buttons[Buttons::BT_RIDE] = std::make_unique<MapleButton>(main["BtRide"]);
		buttons[Buttons::BT_MACRO] = std::make_unique<MapleButton>(main["BtMacro"]);

		buttons[Buttons::BT_HYPER]->set_state(Button::State::DISABLED);
		buttons[Buttons::BT_GUILDSKILL]->set_state(Button::State::DISABLED);
		buttons[Buttons::BT_RIDE]->set_state(Button::State::DISABLED);


		sprites.emplace_back(ui_backgrnd, Point<int16_t>(1, 0));
		sprites.emplace_back(main["backgrnd2"]);
		sprites.emplace_back(main["backgrnd3"]);

		nl::node macro = Skill["macro"];

		macro_backgrnd = macro["backgrnd"];
		macro_backgrnd2 = macro["backgrnd2"];
		macro_backgrnd3 = macro["backgrnd3"];
		macro_check_sprite = macro["check"];
		macro_select_sprite = macro["select"];
		// Macro activation icons (32x32) — each row gets its own style
		// from Macroicon/0..4. MACRO_COUNT=3 so we use indices 0, 1, 2.
		{
			nl::node macro_icons = nl::nx::ui["UIWindow.img"]["SkillMacro"]["Macroicon"];
			for (int16_t i = 0; i < MACRO_COUNT; i++)
			{
				macro_handle_sprites[i] = macro_icons[std::to_string(i)]["icon"];
			}
		}

		buttons[Buttons::BT_MACRO_OK] = std::make_unique<MapleButton>(macro["BtOK"], Point<int16_t>(bg_dimensions.x(), 0));

		// Load slot sprite from the standalone SkillMacro NX (UIWindow.img/SkillMacro)
		nl::node sm = nl::nx::ui["UIWindow.img"]["SkillMacro"];
		if (sm["slot"].size() > 0)
		{
			macro_slot = sm["slot"];
			// Normalize: draw at pos places top-left at pos (origin -> 0,0)
			macro_slot.shift(macro_slot.get_origin());
		}

		// Layout for the v83 inline macro panel:
		//   - 3 rows x 3 slots grid at the top
		//   - Single name field at the bottom (edits selected row)
		macro_slots_origin = Point<int16_t>(14, 46);
		macro_slot_size = 32;
		macro_slot_gap = 4;          // 32 + 4 = 36 between slot columns
		macro_row_gap = 12;          // 32 + 12 = 44 between slot rows

		// Text input box baked into the backdrop — centered around (97, 210)
		// per in-game click measurement.
		macro_name_origin = Point<int16_t>(60, 200);
		macro_name_size = Point<int16_t>(120, 20);

		selected_macro = 0;

		for (int16_t i = 0; i < MACRO_COUNT; i++)
		{
			macro_names[i] = "";
			macro_shouts[i] = false;
			macro_skills[i][0] = 0;
			macro_skills[i][1] = 0;
			macro_skills[i][2] = 0;

			macro_row_icons[i] = std::make_unique<Icon>(
				std::make_unique<MacroIcon>(i), macro_handle_sprites[i], -1);
		}

		{
			Point<int16_t> field_lt = Point<int16_t>(bg_dimensions.x(), 0) + macro_name_origin;
			Point<int16_t> field_rb = field_lt + macro_name_size;
			macro_name_field = Textfield(
				Text::A11M, Text::LEFT, Color::Name::BLACK,
				Rectangle<int16_t>(field_lt, field_rb), 13
			);
		}


		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 23, 6));

		nl::node Tab = main["Tab"];
		nl::node enabled = Tab["enabled"];
		nl::node disabled = Tab["disabled"];

		for (uint16_t i = Buttons::BT_TAB0; i <= Buttons::BT_TAB4; ++i)
		{
			uint16_t tabid = i - Buttons::BT_TAB0;
			buttons[i] = std::make_unique<TwoSpriteButton>(disabled[tabid], enabled[tabid]);
		}

		for (uint16_t i = Buttons::BT_SPUP0; i <= Buttons::BT_SPUP3; ++i)
		{
			uint16_t row = i - Buttons::BT_SPUP0;
			Point<int16_t> spup_position = SKILL_OFFSET + Point<int16_t>(124, 20 + row * ROW_HEIGHT);
			buttons[i] = std::make_unique<MapleButton>(main["BtSpUp"], spup_position);
		}

		booktext = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "", 150);
		splabel = Text(Text::Font::A12M, Text::Alignment::RIGHT, Color::Name::BLACK);

		slider = Slider(
			Slider::Type::DEFAULT_SILVER, Range<int16_t>(92, 236), 154, ROWS, 1,
			[&](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				bool above = offset + shift >= 0;
				bool below = offset + 4 + shift <= skillcount;

				if (above && below)
					change_offset(offset + shift);
			}
		);

		change_job(stats.get_stat(MapleStat::Id::JOB));

		set_macro(false);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	void UISkillBook::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		bookicon.draw(position + Point<int16_t>(11, 85));
		booktext.draw(position + Point<int16_t>(95, 59));
		splabel.draw(position + Point<int16_t>(bg_dimensions.x() - 18, 251));

		Point<int16_t> skill_position = position + SKILL_OFFSET;

		for (size_t i = 0; i < ROWS; i++)
		{
			size_t idx = offset + i;

			if (idx < skills.size())
			{
				if (check_required(skills[idx].get_id()))
				{
					skille.draw(skill_position);
				}
				else
				{
					skilld.draw(skill_position);
					skills[idx].get_icon()->set_state(StatefulIcon::State::DISABLED);
				}

				skills[idx].draw(skill_position + ICON_OFFSET);
			}
			else
			{
				skillb.draw(skill_position);
			}

			if (i < ROWS - 1)
				line.draw(skill_position + LINE_OFFSET);

			skill_position.shift_y(ROW_HEIGHT);
		}

		slider.draw(position);

		if (macro_enabled)
		{
			Point<int16_t> macro_pos = position + Point<int16_t>(bg_dimensions.x(), 0);

			macro_backgrnd.draw(macro_pos + Point<int16_t>(1, 0));
			macro_backgrnd2.draw(macro_pos);
			macro_backgrnd3.draw(macro_pos);

			// Row-selection highlight (drawn under the slot grid)
			if (macro_select_sprite.is_valid())
			{
				Point<int16_t> row_tl = macro_pos + macro_slots_origin
					+ Point<int16_t>(-1, selected_macro * (macro_slot_size + macro_row_gap) - 6);
				macro_select_sprite.draw(DrawArgument(row_tl));
			}

			// Per-row macro activation icon at the right end of each row.
			// The Macroicon sprite has origin (0, 32) so texture.draw
			// subtracts that — pass +32 on Y to land on the target top.
			int16_t handle_x = macro_slots_origin.x() + 3 * macro_slot_size + 2 * macro_slot_gap + 20;
			int16_t handle_y_offset = -6;  // nudge up
			for (int16_t i = 0; i < MACRO_COUNT; i++)
			{
				if (!macro_handle_sprites[i].is_valid()) continue;
				Point<int16_t> h_pos = macro_pos + Point<int16_t>(
					handle_x,
					macro_slots_origin.y() + i * (macro_slot_size + macro_row_gap)
						+ handle_y_offset + 32);
				macro_handle_sprites[i].draw(DrawArgument(h_pos));
			}

			// 3x3 macro skill slot grid
			for (int16_t i = 0; i < MACRO_COUNT; i++)
			{
				for (int16_t s = 0; s < 3; s++)
				{
					Point<int16_t> slot_pos = macro_pos + macro_slots_origin + Point<int16_t>(
						s * (macro_slot_size + macro_slot_gap),
						i * (macro_slot_size + macro_row_gap)
					);

					int32_t skillid = macro_skills[i][s];

					if (skillid > 0)
					{
						const SkillData& data = SkillData::get(skillid);
						Texture icon = data.get_icon(SkillData::Icon::NORMAL);

						if (icon.is_valid())
						{
							icon.shift(Point<int16_t>(0, 32));
							// Third column nudged a bit further left so it
							// lines up with the backdrop's right cutout.
							int16_t icon_dx = (s == 2) ? -2 : 1;
							icon.draw(DrawArgument(slot_pos + Point<int16_t>(icon_dx, -2)));
						}
					}
				}
			}

			// Bottom name field (for currently selected row)
			macro_name_field.draw(position);

			// Preview slot to the left of the name field — shows the
			// macro icon for the currently selected row.
			if (selected_macro >= 0 && selected_macro < MACRO_COUNT
				&& macro_handle_sprites[selected_macro].is_valid())
			{
				Point<int16_t> preview_pos = macro_pos + Point<int16_t>(15, 189);
				// Macroicon origin is (0, 32) so the texture's top-left
				// is at draw_pos.y - 32 — shift y by +32 to land it on
				// the intended spot.
				macro_handle_sprites[selected_macro].draw(
					DrawArgument(preview_pos + Point<int16_t>(0, 32)));
			}

			// Shout checkbox state indicator — real NX sprite drawn at
			// its own origin (-161, -235) inside the macro panel.
			if (macro_shouts[selected_macro] && macro_check_sprite.is_valid())
			{
				Point<int16_t> macro_pos = position + Point<int16_t>(bg_dimensions.x(), 0);
				macro_check_sprite.draw(DrawArgument(macro_pos));
			}
		}


		UIElement::draw_buttons(alpha);
	}

	void UISkillBook::update()
	{
		UIElement::update();

		if (macro_enabled)
			macro_name_field.update(position);

		// Drain queued macro skills one per tick when the player is
		// ready to attack. `combat.use_move` is a no-op while the
		// previous skill's animation / cooldown is still running, so
		// we can't just fire all three back to back.
		if (!pending_macro_skills.empty())
		{
			auto& stage = Stage::get();
			if (stage.get_player().can_attack())
			{
				int32_t next_id = pending_macro_skills.front();
				pending_macro_skills.erase(pending_macro_skills.begin());
				stage.get_combat().use_move(next_id);
			}
		}
	}

	Button::State UISkillBook::button_pressed(uint16_t id)
	{
		std::string sp_text = splabel.get_text();
		int16_t cur_sp = sp_text.empty() ? 0 : std::stoi(sp_text);

		switch (id)
		{
		case Buttons::BT_CLOSE:
			close();
			break;
		case Buttons::BT_MACRO:
			set_macro(!macro_enabled);
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
		case Buttons::BT_TAB4:
			change_tab(id - Buttons::BT_TAB0);

			return Button::State::PRESSED;
		case Buttons::BT_SPUP0:
		case Buttons::BT_SPUP1:
		case Buttons::BT_SPUP2:
		case Buttons::BT_SPUP3:
			send_spup(id - Buttons::BT_SPUP0 + offset);
			break;
		case Buttons::BT_MACRO_OK:
			save_macros();
			set_macro(false);
			break;
		case Buttons::BT_HYPER:
		case Buttons::BT_GUILDSKILL:
		case Buttons::BT_RIDE:
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UISkillBook::toggle_active()
	{
		UIElement::toggle_active();
		clear_tooltip();
	}

	void UISkillBook::doubleclick(Point<int16_t> cursorpos)
	{
		const SkillDisplayMeta* skill = skill_by_position(cursorpos - position);

		if (skill)
		{
			int32_t skill_id = skill->get_id();
			int32_t skill_level = skillbook.get_level(skill_id);

			if (skill_level > 0)
				Stage::get().get_combat().use_move(skill_id);
		}
	}

	void UISkillBook::remove_cursor()
	{
		UIDragElement::remove_cursor();

		slider.remove_cursor();
	}

	Cursor::State UISkillBook::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (macro_enabled)
		{
			// Shout checkbox baked into backdrop around (152, 236).
			if (clicked)
			{
				Point<int16_t> cb_tl = position + Point<int16_t>(bg_dimensions.x(), 0)
					+ Point<int16_t>(144, 228);
				Rectangle<int16_t> cb_rect(cb_tl, cb_tl + Point<int16_t>(18, 18));
				if (cb_rect.contains(cursorpos))
				{
					macro_shouts[selected_macro] = !macro_shouts[selected_macro];
					return Cursor::State::CLICKING;
				}
			}

			// Bottom name field (single, edits selected row)
			Cursor::State tstate = macro_name_field.send_cursor(cursorpos, clicked);
			if (tstate != Cursor::State::IDLE)
				return tstate;

			if (clicked)
			{
				Point<int16_t> macro_pos = position + Point<int16_t>(bg_dimensions.x(), 0);
				int16_t handle_x = macro_slots_origin.x() + 3 * macro_slot_size + 2 * macro_slot_gap + 20;
				int16_t handle_y_offset = -6;

				for (int16_t i = 0; i < MACRO_COUNT; i++)
				{
					// Draggable macro handle at the right end of the row.
					Point<int16_t> h_tl = macro_pos + Point<int16_t>(
						handle_x,
						macro_slots_origin.y() + i * (macro_slot_size + macro_row_gap) + handle_y_offset);
					Rectangle<int16_t> h_rect(h_tl, h_tl + Point<int16_t>(32, 32));

					if (h_rect.contains(cursorpos))
					{
						macro_select_row(i);
						if (macro_row_icons[i])
						{
							macro_row_icons[i]->start_drag(cursorpos - h_tl);
							UI::get().drag_icon(macro_row_icons[i].get());
						}
						return Cursor::State::GRABBING;
					}

					// Slot grid — click selects the row for name editing.
					Point<int16_t> row_tl = macro_pos + macro_slots_origin + Point<int16_t>(0, i * (macro_slot_size + macro_row_gap));
					Point<int16_t> row_br = row_tl + Point<int16_t>(3 * macro_slot_size + 2 * macro_slot_gap, macro_slot_size);
					Rectangle<int16_t> row_rect(row_tl, row_br);

					if (row_rect.contains(cursorpos))
					{
						macro_select_row(i);
						return Cursor::State::CLICKING;
					}
				}
			}
		}

		Point<int16_t> cursor_relative = cursorpos - position;

		if (slider.isenabled())
		{
			if (Cursor::State new_state = slider.send_cursor(cursor_relative, clicked))
			{
				clear_tooltip();

				return new_state;
			}
		}

		// UIDragElement::send_cursor already called UIElement::send_cursor
		// which handles button clicks. Check if a button was hit and return early.
		if (clicked)
		{
			for (uint16_t i = Buttons::BT_SPUP0; i <= Buttons::BT_SPUP3; i++)
			{
				if (buttons[i]->is_active() && buttons[i]->bounds(position).contains(cursorpos))
					return dstate;
			}
			if (buttons[Buttons::BT_CLOSE]->is_active() && buttons[Buttons::BT_CLOSE]->bounds(position).contains(cursorpos))
				return dstate;
			if (buttons[Buttons::BT_MACRO]->is_active() && buttons[Buttons::BT_MACRO]->bounds(position).contains(cursorpos))
				return dstate;
		}

		Point<int16_t> skill_position = position + SKILL_OFFSET;

		if (!grabbing)
		{
			for (size_t i = 0; i < ROWS; i++)
			{
				size_t idx = offset + i;

				if (idx >= skills.size())
					break;

				constexpr Rectangle<int16_t> bounds = Rectangle<int16_t>(0, 148, 0, 32);
				Point<int16_t> icon_pos = skill_position + ICON_OFFSET;
				bool inrange = bounds.contains(cursorpos - skill_position);

				if (inrange)
				{
					if (clicked)
					{
						clear_tooltip();
						grabbing = true;

						int32_t skill_id = skills[idx].get_id();
						int32_t skill_level = skillbook.get_level(skill_id);

						if (skill_level > 0 && !SkillData::get(skill_id).is_passive())
						{
							skills[idx].get_icon()->start_drag(cursorpos - icon_pos);
							UI::get().drag_icon(skills[idx].get_icon());

							return Cursor::State::GRABBING;
						}
						else
						{
							return Cursor::State::IDLE;
						}
					}
					else
					{
						skills[idx].get_icon()->set_state(StatefulIcon::State::MOUSEOVER);
						show_skill(skills[idx].get_id());

						return Cursor::State::IDLE;
					}
				}

				skill_position.shift_y(ROW_HEIGHT);
			}

			for (size_t i = 0; i < skills.size(); i++)
			{
				skills[i].get_icon()->set_state(StatefulIcon::State::NORMAL);
			}
			clear_tooltip();
		}
		else
		{
			grabbing = false;
		}

		return dstate;
	}

	void UISkillBook::send_scroll(double yoffset)
	{
		if (!slider.isenabled())
			return;

		int16_t new_offset = offset;

		if (yoffset < 0)
			new_offset = std::min<int16_t>(offset + 1, skillcount > ROWS ? skillcount - ROWS : 0);
		else if (yoffset > 0)
			new_offset = std::max<int16_t>(offset - 1, 0);

		if (new_offset != offset)
		{
			change_offset(new_offset);
			slider.setrows(new_offset, ROWS, skillcount);
		}
	}

	void UISkillBook::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				close();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				clear_tooltip();

				Job::Level level = job.get_level();
				uint16_t id = tab + 1;
				uint16_t new_tab = tab + Buttons::BT_TAB0;

				if (new_tab < Buttons::BT_TAB4 && id <= level)
					new_tab++;
				else
					new_tab = Buttons::BT_TAB0;

				change_tab(new_tab - Buttons::BT_TAB0);
			}
		}
	}

	UIElement::Type UISkillBook::get_type() const
	{
		return TYPE;
	}

	void UISkillBook::update_stat(MapleStat::Id stat, int16_t value)
	{
		switch (stat)
		{
		case MapleStat::Id::JOB:
			change_job(value);
			break;
		case MapleStat::Id::SP:
			change_sp();
			break;
		}
	}

	void UISkillBook::update_skills(int32_t skill_id)
	{
		change_tab(tab);
	}

	void UISkillBook::change_job(uint16_t id)
	{
		job.change_job(id);

		Job::Level level = job.get_level();

		for (uint16_t i = 0; i <= Job::Level::FOURTH; i++)
			buttons[Buttons::BT_TAB0 + i]->set_active(i <= level);

		change_tab(level - Job::Level::BEGINNER);
	}

	void UISkillBook::change_sp()
	{
		Job::Level joblevel = joblevel_by_tab(tab);
		uint16_t level = stats.get_stat(MapleStat::Id::LEVEL);
		if (joblevel == Job::Level::BEGINNER)
		{
			int16_t remaining_beginner_sp = 0;

			if (level >= 7)
				remaining_beginner_sp = 6;
			else
				remaining_beginner_sp = level - 1;

			for (size_t i = 0; i < skills.size(); i++)
			{
				int32_t skillid = skills[i].get_id();
				int32_t base = skillid % 10000;

				// Three Snails/Recovery/Nimble Feet (works for Explorer, Noblesse, Aran)
				if (base == 1000 || base == 1001 || base == 1002)
					remaining_beginner_sp -= skills[i].get_level();
			}

			beginner_sp = remaining_beginner_sp;
			splabel.change_text(std::to_string(beginner_sp));
		}
		else
		{
			sp = stats.get_stat(MapleStat::Id::SP);
			splabel.change_text(std::to_string(sp));
		}

		change_offset(offset);
	}

	void UISkillBook::change_tab(uint16_t new_tab)
	{
		buttons[Buttons::BT_TAB0 + tab]->set_state(Button::NORMAL);
		buttons[Buttons::BT_TAB0 + new_tab]->set_state(Button::PRESSED);
		tab = new_tab;

		skills.clear();
		skillcount = 0;

		Job::Level joblevel = joblevel_by_tab(tab);
		uint16_t subid = job.get_subjob(joblevel);

		const JobData& data = JobData::get(subid);
		bookicon = data.get_icon();
		booktext.change_text(data.get_name());

		// Union of two sources, both filtered to prefix == subid:
		//  (a) NX Skill.wz/<subid>.img — full job tree, so unlearned
		//      skills appear as grey 0-level rows.
		//  (b) skillbook entries the server has granted whose prefix
		//      (sid / 10000) matches subid — covers anything the server
		//      assigned that NX doesn't list.
		// Both lists are filtered by prefix so unrelated branches
		// (e.g. Aran skills on an Explorer beginner) never appear.
		std::unordered_set<int32_t> seen;
		auto push = [&](int32_t sid)
		{
			if (sid / 10000 != static_cast<int32_t>(subid))
				return;
			// NX flags mount/item-tied skills (Balrog, spaceship,
			// custom riding) with invisible = 1. Suppress those even
			// though they appear under a class's skill list.
			if (SkillData::get(sid).is_invisible())
				return;
			if (!seen.insert(sid).second)
				return;
			skills.emplace_back(sid, skillbook.get_level(sid));
			skillcount++;
		};

		for (int32_t sid : data.get_skills())
			push(sid);

		for (auto& entry : skillbook.get_entries())
			push(entry.first);

		slider.setrows(ROWS, skillcount);
		offset = 0;
		change_sp();
	}

	void UISkillBook::change_offset(uint16_t new_offset)
	{
		offset = new_offset;

		for (int16_t i = 0; i < ROWS; i++)
		{
			uint16_t index = Buttons::BT_SPUP0 + i;
			uint16_t row = offset + i;
			buttons[index]->set_active(row < skillcount);

			if (row < skills.size())
			{
				int32_t skill_id = skills[row].get_id();
				bool canraise = can_raise(skill_id);
				buttons[index]->set_state(canraise ? Button::State::NORMAL : Button::State::DISABLED);
			}
		}
	}

	void UISkillBook::show_skill(int32_t id)
	{
		int32_t skill_id = id;
		int32_t level = skillbook.get_level(id);
		int32_t masterlevel = skillbook.get_masterlevel(id);
		int64_t expiration = skillbook.get_expiration(id);

		UI::get().show_skill(Tooltip::Parent::SKILLBOOK, skill_id, level, masterlevel, expiration);
	}

	void UISkillBook::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::SKILLBOOK);
	}

	bool UISkillBook::can_raise(int32_t skill_id) const
	{
		Job::Level joblevel = joblevel_by_tab(tab);

		if (joblevel == Job::Level::BEGINNER && beginner_sp <= 0)
			return false;

		if (tab + Buttons::BT_TAB0 != Buttons::BT_TAB0 && sp <= 0)
			return false;

		int32_t level = skillbook.get_level(skill_id);
		int32_t masterlevel = skillbook.get_masterlevel(skill_id);

		if (masterlevel == 0)
			masterlevel = SkillData::get(skill_id).get_masterlevel();

		if (level >= masterlevel)
			return false;

		int32_t base_id = skill_id % 10000;

		switch (base_id)
		{
		case SkillId::Id::ANGEL_BLESSING:
			return false;
		default:
			return check_required(skill_id);
		}
	}

	void UISkillBook::send_spup(uint16_t row)
	{
		if (row >= skills.size())
			return;

		int32_t id = skills[row].get_id();

		std::string sp_txt = splabel.get_text();
		int16_t cur_sp = sp_txt.empty() ? 0 : std::stoi(sp_txt);

		if (cur_sp <= 0)
			return;

		spend_sp(id);
		change_sp();
	}

	void UISkillBook::spend_sp(int32_t skill_id)
	{
		SpendSpPacket(skill_id).dispatch();
	}

	Job::Level UISkillBook::joblevel_by_tab(uint16_t t) const
	{
		switch (t)
		{
		case 1:
			return Job::Level::FIRST;
		case 2:
			return Job::Level::SECOND;
		case 3:
			return Job::Level::THIRD;
		case 4:
			return Job::Level::FOURTH;
		default:
			return Job::Level::BEGINNER;
		}
	}

	const UISkillBook::SkillDisplayMeta* UISkillBook::skill_by_position(Point<int16_t> cursorpos) const
	{
		int16_t x = cursorpos.x();

		if (x < SKILL_OFFSET.x() || x > 148)
			return nullptr;

		int16_t y = cursorpos.y();

		if (y < SKILL_OFFSET.y())
			return nullptr;

		uint16_t row = (y - SKILL_OFFSET.y()) / ROW_HEIGHT;

		if (row >= ROWS)
			return nullptr;

		uint16_t skill_idx = offset + row;

		if (skill_idx >= skills.size())
			return nullptr;

		return skills.data() + skill_idx;
	}

	void UISkillBook::close()
	{
		clear_tooltip();
		deactivate();
	}

	bool UISkillBook::check_required(int32_t id) const
	{
		std::unordered_map<int32_t, int32_t> required = skillbook.collect_required(id);

		if (required.size() <= 0)
			required = SkillData::get(id).get_reqskills();

		for (auto reqskill : required)
		{
			int32_t reqskill_level = skillbook.get_level(reqskill.first);
			int32_t req_level = reqskill.second;

			if (reqskill_level < req_level)
				return false;
		}

		return true;
	}

	void UISkillBook::set_macro(bool enabled)
	{
		macro_enabled = enabled;

		if (macro_enabled)
		{
			// Extend dimension to cover the inline macro panel so drops over it hit this UI.
			int16_t panel_w = macro_backgrnd.get_dimensions().x();
			int16_t panel_h = std::max<int16_t>(bg_dimensions.y(),
				macro_name_origin.y() + macro_name_size.y() + 16);
			dimension = Point<int16_t>(bg_dimensions.x() + panel_w, panel_h);
		}
		else
		{
			dimension = bg_dimensions;
		}

		buttons[Buttons::BT_MACRO_OK]->set_active(macro_enabled);
		buttons[Buttons::BT_MACRO_OK]->set_state(macro_enabled ? Button::State::NORMAL : Button::State::DISABLED);

		if (macro_enabled)
		{
			macro_name_field.change_text(macro_names[selected_macro]);
			// Focus immediately so typing starts working without a
			// second click.
			macro_name_field.set_state(Textfield::State::FOCUSED);
		}
		else
		{
			macro_name_field.set_state(Textfield::State::DISABLED);
		}
	}

	void UISkillBook::macro_select_row(int16_t row)
	{
		if (row < 0 || row >= MACRO_COUNT)
			return;

		// Persist current field text into the previously selected macro
		macro_names[selected_macro] = macro_name_field.get_text();

		selected_macro = row;
		macro_name_field.change_text(macro_names[selected_macro]);
	}

	void UISkillBook::load_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3)
	{
		if (index >= MACRO_COUNT)
			return;

		macro_names[index] = name;
		macro_shouts[index] = shout;
		macro_skills[index][0] = s1;
		macro_skills[index][1] = s2;
		macro_skills[index][2] = s3;

		if (index == selected_macro)
			macro_name_field.change_text(name);
	}

	void UISkillBook::trigger_macro(int32_t index)
	{
		if (index < 0 || index >= MACRO_COUNT)
			return;

		// Queue all 3 skills — update() fires them one at a time as
		// the player becomes ready to attack again, so the second and
		// third don't get swallowed by can_attack() == false.
		pending_macro_skills.clear();
		for (int32_t s = 0; s < 3; s++)
		{
			int32_t skill_id = macro_skills[index][s];
			if (skill_id > 0)
				pending_macro_skills.push_back(skill_id);
		}

		// Shout the macro name so nearby players see it in chat.
		if (macro_shouts[index] && !macro_names[index].empty())
			GeneralChatPacket(macro_names[index], true).dispatch();
	}

	void UISkillBook::save_macros()
	{
		// Persist the currently edited name into the selected row
		macro_names[selected_macro] = macro_name_field.get_text();

		SkillMacroModifiedPacket::MacroData data[MACRO_COUNT];

		for (int16_t i = 0; i < MACRO_COUNT; i++)
		{
			data[i].name = macro_names[i];
			data[i].shout = macro_shouts[i];
			data[i].skill1 = macro_skills[i][0];
			data[i].skill2 = macro_skills[i][1];
			data[i].skill3 = macro_skills[i][2];
		}

		SkillMacroModifiedPacket(data, MACRO_COUNT).dispatch();
	}

	bool UISkillBook::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (!macro_enabled)
			return true;

		if (const_cast<Icon&>(icon).get_type() != Icon::IconType::SKILL)
			return true;

		int32_t skill_id = icon.get_action_id();
		if (skill_id == 0)
			return true;

		Point<int16_t> macro_pos = position + Point<int16_t>(bg_dimensions.x(), 0);

		for (int16_t i = 0; i < MACRO_COUNT; i++)
		{
			for (int16_t s = 0; s < 3; s++)
			{
				Point<int16_t> slot_pos = macro_pos + macro_slots_origin + Point<int16_t>(
					s * (macro_slot_size + macro_slot_gap),
					i * (macro_slot_size + macro_row_gap)
				);
				Rectangle<int16_t> slot_rect(slot_pos, slot_pos + Point<int16_t>(macro_slot_size, macro_slot_size));

				if (slot_rect.contains(cursorpos))
				{
					macro_skills[i][s] = skill_id;
					return true;
				}
			}
		}

		return true;
	}

}