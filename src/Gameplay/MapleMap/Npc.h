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

#include "MapObject.h"

#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"
#include "../../Util/Randomizer.h"

namespace ms
{
	// Quest mark type to display above an NPC
	enum class QuestMarkType : uint8_t
	{
		NONE,
		AVAILABLE,         // Normal quest available (yellow !)
		IN_PROGRESS,       // Quest in progress (yellow ?)
		COMPLETE,          // Quest ready to complete (yellow ? with light)
		AVAILABLE_REPEAT,  // Repeat quest available
		LOW_LEVEL,         // Available but low level
		HIGH_LEVEL         // Available but high level
	};

	// Represents a NPC on the current map
	// Implements the 'MapObject' interface to be used in a 'MapObjects' template
	class Npc : public MapObject
	{
	public:
		// Constructs an NPC by combining data from game files with data sent by the server
		Npc(int32_t npcid, int32_t oid, bool mirrored, uint16_t fhid, bool control, Point<int16_t> position);

		// Draws the current animation and name/function tags
		void draw(double viewx, double viewy, float alpha) const override;
		// Updates the current animation and physics
		int8_t update(const Physics& physics) override;

		// Changes stance and resets animation
		void set_stance(const std::string& stance);

		// Check whether this is a server-sided NPC
		bool isscripted() const;
		// Check if the NPC is in range of the cursor
		bool inrange(Point<int16_t> cursorpos, Point<int16_t> viewpos) const;

		// Returns the NPC name
		std::string get_name();
		// Returns the NPC's function description or title
		std::string get_func();
		// Returns the NPC's data ID
		int32_t get_npcid() const;

		// Recalculate quest mark for this NPC based on current quest state
		void update_quest_mark();

		// Initialize shared quest mark animations (call once at startup)
		static void init_quest_marks();

	private:
		std::map<std::string, Animation> animations;
		std::map<std::string, std::vector<std::string>> lines;
		std::vector<std::string> states;
		std::string name;
		std::string func;
		bool hidename;
		bool scripted;
		bool mouseonly;

		int32_t npcid;
		bool flip;
		std::string stance;
		bool control;

		Randomizer random;
		Text namelabel;
		Text funclabel;

		// Quest mark above NPC
		QuestMarkType quest_mark_type;
		mutable Animation quest_mark_anim;

		// Shared quest mark animations from MapHelper.img/quest (v83)
		static bool marks_initialized;
		static Animation mark_available;      // chatNext — yellow lightbulb (quest available)
		static Animation mark_in_progress;    // chatSelf — blue ? (quest in progress)
		static Animation mark_complete;       // chatComplete — yellow ! (ready to turn in)
		static Animation mark_repeat;
		static Animation mark_low_level;
		static Animation mark_high_level;
	};
}