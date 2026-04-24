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

#include <vector>

namespace ms
{
	class UIFamilyTree : public UIDragElement<PosFAMILYTREE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::FAMILYTREE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIFamilyTree();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Cleared + rebuilt from FamilyChartResultHandler each refresh.
		void clear_entries();
		void add_entry(int32_t cid, int32_t parent_id, int32_t job,
			int32_t level, bool online, int32_t rep, int32_t total_rep,
			int32_t reps_to_senior, int32_t todays_rep, int32_t channel,
			int32_t time_online, const std::string& name, bool is_viewed);

		// Authoritative leader id — sourced from FamilyInfoResultHandler.
		void set_leader_id(int32_t leader_id);

		void set_family_name(const std::string& name);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_BYE,
			BT_JUNIOR_ENTRY,
			BT_LEFT,
			BT_RIGHT,
			NUM_BUTTONS
		};

		struct Entry
		{
			int32_t cid;
			int32_t parent_id;
			int32_t job;
			int32_t level;
			bool online;
			int32_t rep;
			int32_t total_rep;
			int32_t todays_rep;
			std::string name;
			bool is_viewed;
		};

		std::vector<Entry> entries;
		int32_t leader_id = 0;
		int32_t current_viewed_id = 0;

		// Populated by draw() so send_cursor knows which plate was
		// clicked (cid → screen-space plate rectangle).
		mutable std::vector<std::pair<int32_t, Rectangle<int16_t>>> plate_hitboxes;

		// Frame counter drives the hover-highlight alpha pulse.
		mutable uint32_t tick = 0;

		Texture plate_leader;
		Texture plate_others;
		Texture selected;

		mutable Text name_label;
		mutable Text level_label;
		mutable Text job_label;
		mutable Text size_label;
		mutable Text family_name_label;
	};
}
