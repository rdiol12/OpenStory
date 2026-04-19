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
#include "UISkillMacro.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Data/SkillData.h"
#include "../../Net/Packets/PlayerPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	SkillMacro UISkillMacro::cached_macros[UISkillMacro::NUM_MACROS] = {};
	bool UISkillMacro::cache_loaded = false;

	void UISkillMacro::cache_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3)
	{
		if (index >= NUM_MACROS)
			return;

		cached_macros[index] = SkillMacro(name, shout, s1, s2, s3);
		cache_loaded = true;
	}

	UISkillMacro::UISkillMacro() : UIDragElement<PosSKILLMACRO>(Point<int16_t>(250, 20))
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["SkillMacro"];
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 6));
		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(src["BtOK"]);
		buttons[Buttons::BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"]);

		// Row layout constants
		row_origin = Point<int16_t>(12, 55);
		row_height = 62;

		// Initialize name textfields for each macro row
		for (int16_t i = 0; i < NUM_MACROS; i++)
		{
			Point<int16_t> field_lt = row_origin + Point<int16_t>(82, i * row_height + 2);
			Point<int16_t> field_rb = field_lt + Point<int16_t>(96, 16);

			name_fields[i] = Textfield(
				Text::A11M, Text::LEFT, Color::Name::BLACK,
				Rectangle<int16_t>(field_lt, field_rb), 13
			);
			name_fields[i].set_state(Textfield::State::NORMAL);
		}

		// Load skill slot texture if available
		nl::node slot_node = src["slot"];

		if (slot_node.size() > 0)
			skill_slot = slot_node;

		// Load shout checkbox textures if available
		nl::node shout_on_node = src["check1"];
		nl::node shout_off_node = src["check0"];

		if (shout_on_node.size() > 0)
			shout_on = shout_on_node;

		if (shout_off_node.size() > 0)
			shout_off = shout_off_node;

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);

		// Load any macros received from server before this UI was emplaced.
		if (cache_loaded)
		{
			for (int16_t i = 0; i < NUM_MACROS; i++)
			{
				const SkillMacro& c = cached_macros[i];
				macros[i] = c;
				name_fields[i].change_text(c.name);
			}
		}
	}

	void UISkillMacro::draw(float inter) const
	{
		UIElement::draw(inter);

		for (int16_t i = 0; i < NUM_MACROS; i++)
		{
			Point<int16_t> row_pos = position + row_origin + Point<int16_t>(0, i * row_height);

			// Draw name field
			name_fields[i].draw(position);

			// Draw 3 skill slots per row
			for (int16_t s = 0; s < 3; s++)
			{
				Point<int16_t> slot_pos = row_pos + Point<int16_t>(188 + s * 36, 0);

				if (skill_slot.is_valid())
					skill_slot.draw(DrawArgument(slot_pos));

				// Draw skill icon if set
				int32_t skillid = 0;

				if (s == 0) skillid = macros[i].skill1;
				else if (s == 1) skillid = macros[i].skill2;
				else skillid = macros[i].skill3;

				if (skillid > 0)
				{
					const SkillData& data = SkillData::get(skillid);
					const Texture& icon = data.get_icon(SkillData::Icon::NORMAL);

					if (icon.is_valid())
						icon.draw(DrawArgument(slot_pos + Point<int16_t>(2, 2)));
				}
			}

			// Draw shout checkbox
			Point<int16_t> shout_pos = row_pos + Point<int16_t>(60, 22);

			if (macros[i].shout)
			{
				if (shout_on.is_valid())
					shout_on.draw(DrawArgument(shout_pos));
			}
			else
			{
				if (shout_off.is_valid())
					shout_off.draw(DrawArgument(shout_pos));
			}
		}
	}

	void UISkillMacro::update()
	{
		UIElement::update();

		for (int16_t i = 0; i < NUM_MACROS; i++)
			name_fields[i].update(position);
	}

	void UISkillMacro::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Cursor::State UISkillMacro::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		Cursor::State bstate = UIElement::send_cursor(clicking, cursorpos);

		if (bstate != Cursor::State::IDLE)
			return bstate;

		// Check textfields
		for (int16_t i = 0; i < NUM_MACROS; i++)
		{
			Cursor::State tstate = name_fields[i].send_cursor(cursorpos, clicking);

			if (tstate != Cursor::State::IDLE)
				return tstate;
		}

		// Check shout checkboxes
		if (clicking)
		{
			for (int16_t i = 0; i < NUM_MACROS; i++)
			{
				Point<int16_t> shout_pos = position + row_origin + Point<int16_t>(60, i * row_height + 22);
				Rectangle<int16_t> shout_rect(shout_pos, shout_pos + Point<int16_t>(16, 16));

				if (shout_rect.contains(cursorpos))
				{
					macros[i].shout = !macros[i].shout;
					return Cursor::State::CLICKING;
				}
			}
		}

		return UIDragElement::send_cursor(clicking, cursorpos);
	}

	UIElement::Type UISkillMacro::get_type() const
	{
		return TYPE;
	}

	void UISkillMacro::set_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3)
	{
		if (index >= NUM_MACROS)
			return;

		macros[index] = SkillMacro(name, shout, s1, s2, s3);
		name_fields[index].change_text(name);
		cached_macros[index] = macros[index];
		cache_loaded = true;
	}

	bool UISkillMacro::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (const_cast<Icon&>(icon).get_type() != Icon::IconType::SKILL)
			return true;

		int32_t skill_id = icon.get_action_id();
		if (skill_id == 0)
			return true;

		for (int16_t i = 0; i < NUM_MACROS; i++)
		{
			for (int16_t s = 0; s < 3; s++)
			{
				Point<int16_t> slot_pos = position + row_origin + Point<int16_t>(188 + s * 36, i * row_height);
				Rectangle<int16_t> slot_rect(slot_pos, slot_pos + Point<int16_t>(32, 32));

				if (slot_rect.contains(cursorpos))
				{
					if (s == 0) macros[i].skill1 = skill_id;
					else if (s == 1) macros[i].skill2 = skill_id;
					else macros[i].skill3 = skill_id;
					return false;
				}
			}
		}

		return true;
	}

	Button::State UISkillMacro::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
			deactivate();
			break;
		case Buttons::BT_OK:
			save_macros();
			deactivate();
			break;
		case Buttons::BT_CANCEL:
			deactivate();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UISkillMacro::save_macros()
	{
		// Sync textfield text back to macro data
		for (int16_t i = 0; i < NUM_MACROS; i++)
			macros[i].name = name_fields[i].get_text();

		// Build packet data and send
		SkillMacroModifiedPacket::MacroData data[NUM_MACROS];

		for (int16_t i = 0; i < NUM_MACROS; i++)
		{
			data[i].name = macros[i].name;
			data[i].shout = macros[i].shout;
			data[i].skill1 = macros[i].skill1;
			data[i].skill2 = macros[i].skill2;
			data[i].skill3 = macros[i].skill3;

			cached_macros[i] = macros[i];
		}

		cache_loaded = true;

		SkillMacroModifiedPacket(data, NUM_MACROS).dispatch();
	}
}
