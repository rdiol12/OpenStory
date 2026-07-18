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

#include <vector>

#include "Stance.h"

#include "../../Template/Point.h"

#include <unordered_map>

namespace ms
{
	// A frame of animation for a skill or similar 'meta-stance' 
	// This simply redirects to a different stance and frame to use
	class BodyAction
	{
	public:
		BodyAction(nl::node src)
		{
			stance = Stance::by_string(src["action"]);
			frame = src["frame"];
			move = src["move"];

			int16_t sgndelay = src["delay"];

			if (sgndelay == 0)
				sgndelay = 100;

			if (sgndelay > 0)
			{
				delay = sgndelay;
				attackframe = true;
			}
			else if (sgndelay < 0)
			{
				delay = -sgndelay;
				attackframe = false;
			}
		}

		BodyAction() {}

		bool isattackframe() const
		{
			return attackframe;
		}

		uint8_t get_frame() const
		{
			return frame;
		}

		uint16_t get_delay() const
		{
			return delay;
		}

		Point<int16_t> get_move() const
		{
			return move;
		}

		Stance::Id get_stance() const
		{
			return stance;
		}

	private:
		Stance::Id stance;
		uint8_t frame;
		uint16_t delay;
		Point<int16_t> move;
		bool attackframe;
	};

	class BodyDrawInfo
	{
	public:
		// Per-frame arm rig for the procedural armor sleeve (Phase 2). Dormant
		// until the sleeve renderer exists; also lets the offline eyeball render
		// use the real shoulder pivot instead of the belly proxy. All points are
		// in body-origin space — the SAME frame as get_body_position (the torso
		// navel) — so torso/shoulder/hand need no conversion between them.
		struct ArmFrame
		{
			int16_t layer;             // underlying Body::Layer of the body arm this frame
			                           // (sleeve mirrors it for per-frame z-order)
			Point<int16_t> neck;       // raw neck anchor (top of torso; shoulder base)
			Point<int16_t> shoulder;   // = neck + SHOULDER_OFFSET (derived pivot; tuned)
			Point<int16_t> hand;       // hand anchor -> rotation + axial-stretch target
			Point<int16_t> elbow;      // detected mid-arm fold point (body space); the
			                           // sleeve hinges here when `bent`
			bool bent;                 // arm folds enough this frame to draw 2 segments
			bool present;              // false when the frame has no arm anchor (prone)
		};

		void init();

		Point<int16_t> get_body_position(Stance::Id stance, uint8_t frame) const;
		// The body's neck point per frame. Unlike the navel (which drifts
		// laterally within breathing frames while the body pixels stay put),
		// the neck is stable in idle and tracks real torso motion — the
		// correct anchor for rigid shell garments (see Clothing/aiShell).
		Point<int16_t> get_neck_position(Stance::Id stance, uint8_t frame) const;
		Point<int16_t> get_arm_position(Stance::Id stance, uint8_t frame) const;
		// Arm rig for the procedural sleeve (see ArmFrame). Returns a
		// present=false entry for frames with no arm anchor.
		const ArmFrame& get_arm_frame(Stance::Id stance, uint8_t frame) const;
		Point<int16_t> get_hand_position(Stance::Id stance, uint8_t frame) const;
		Point<int16_t> get_head_position(Stance::Id stance, uint8_t frame) const;
		Point<int16_t> gethairpos(Stance::Id stance, uint8_t frame) const;
		Point<int16_t> getfacepos(Stance::Id stance, uint8_t frame) const;
		uint8_t nextframe(Stance::Id stance, uint8_t frame) const;
		uint16_t get_delay(Stance::Id stance, uint8_t frame) const;

		uint16_t get_attackdelay(std::string action, size_t no) const;
		uint8_t next_actionframe(std::string action, uint8_t frame) const;
		const BodyAction* get_action(std::string action, uint8_t frame) const;

	private:
		std::unordered_map<uint8_t, Point<int16_t>> body_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> neck_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> arm_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> hand_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> head_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> hair_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, Point<int16_t>> face_positions[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, ArmFrame> arm_frames[Stance::Id::LENGTH];
		std::unordered_map<uint8_t, uint16_t> stance_delays[Stance::Id::LENGTH];

		std::unordered_map<std::string, std::unordered_map<uint8_t, BodyAction>> body_actions;
		std::unordered_map<std::string, std::vector<uint16_t>> attack_delays;
	};
}
