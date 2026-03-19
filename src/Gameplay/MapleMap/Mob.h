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

#include "../Movement.h"

#include "../Combat/Attack.h"
#include "../Combat/Bullet.h"

#include "../../Audio/Audio.h"
#include "../../Graphics/EffectLayer.h"
#include "../../Graphics/Geometry.h"
#include "../../Util/Randomizer.h"
#include "../../Util/TimedBool.h"

#include <unordered_map>

namespace ms
{
	namespace MobStatus
	{
		enum Id : int32_t
		{
			WATK           = 0x1,
			WDEF           = 0x2,
			MATK           = 0x4,
			MDEF           = 0x8,
			ACC            = 0x10,
			AVOID          = 0x20,
			SPEED          = 0x40,
			STUN           = 0x80,
			FREEZE         = 0x100,
			POISON         = 0x200,
			SEAL           = 0x400,
			SHOWDOWN       = 0x800,
			WEAPON_ATTACK_UP  = 0x1000,
			WEAPON_DEFENSE_UP = 0x2000,
			MAGIC_ATTACK_UP   = 0x4000,
			MAGIC_DEFENSE_UP  = 0x8000,
			DOOM           = 0x10000,
			SHADOW_WEB     = 0x20000,
			WEAPON_IMMUNITY = 0x40000,
			MAGIC_IMMUNITY  = 0x80000,
			HARD_SKIN      = 0x200000,
			NINJA_AMBUSH   = 0x400000,
			VENOMOUS_WEAPON = 0x1000000,
			BLIND          = 0x2000000,
			SEAL_SKILL     = 0x4000000,
			INERTMOB       = 0x10000000
		};

		// First-mask statuses (from Cosmic: isFirst() == true)
		enum FirstMask : int32_t
		{
			NEUTRALISE     = 0x2,
			PHANTOM_IMPRINT = 0x4,
			WEAPON_REFLECT = 0x20000000,
			MAGIC_REFLECT  = 0x40000000
		};
	}

	struct MobStatusEntry
	{
		int16_t value;
		int32_t skillid;
		int16_t duration;
	};

	class Mob : public MapObject
	{
	public:
		static const size_t NUM_STANCES = 6;

		enum Stance : uint8_t
		{
			MOVE = 2,
			STAND = 4,
			JUMP = 6,
			HIT = 8,
			DIE = 10
		};

		static std::string nameof(Stance stance)
		{
			static const std::string stancenames[NUM_STANCES] =
			{
				"move",
				"stand",
				"jump",
				"hit1",
				"die1",
				"fly"
			};

			size_t index = (stance - 1) / 2;

			return stancenames[index];
		}

		static uint8_t value_of(Stance stance, bool flip)
		{
			return flip ? stance : stance + 1;
		}

		// Construct a mob by combining data from game files with data sent by the server
		Mob(int32_t oid, int32_t mobid, int8_t mode, int8_t stance, uint16_t fhid, bool newspawn, int8_t team, Point<int16_t> position);

		// Draw the mob
		void draw(double viewx, double viewy, float alpha) const override;
		// Update movement and animations
		int8_t update(const Physics& physics) override;

		// Change this mob's control mode:
		// 0 - no control, 1 - control, 2 - aggro
		void set_control(int8_t mode);
		// Send movement to the mob
		void send_movement(Point<int16_t> start, std::vector<Movement>&& movements);
		// Kill the mob with the appropriate type:
		// 0 - make inactive 1 - death animation 2 - fade out
		void kill(int8_t killtype);
		// Display the hp percentage above the mob
		// Use the playerlevel to determine color of NameTag
		void show_hp(int8_t percentage, uint16_t playerlevel);
		// Show an effect at the mob's position
		void show_effect(const Animation& animation, int8_t pos, int8_t z, bool flip);

		// Apply a status effect to this mob
		void apply_status(int32_t status_mask, int32_t first_mask, const std::vector<std::pair<int32_t, MobStatusEntry>>& statuses);
		// Cancel a status effect on this mob
		void cancel_status(int32_t status_mask, int32_t first_mask);

		// Calculate the damage to this mob with the specified attack
		std::vector<std::pair<int32_t, bool>> calculate_damage(const Attack& attack);
		// Apply damage to the mob
		void apply_damage(int32_t damage, bool toleft);

		// Create a touch damage attack to the player
		MobAttack create_touch_attack() const;

		// Check if this mob collides with the specified rectangle
		bool is_in_range(const Rectangle<int16_t>& range) const;
		// Check if this mob is still alive
		bool is_alive() const;
		// Return the head position
		Point<int16_t> get_head_position() const;

	private:
		enum FlyDirection
		{
			STRAIGHT,
			UPWARDS,
			DOWNWARDS,
			NUM_DIRECTIONS
		};

		// Set the stance by byte value
		void set_stance(uint8_t stancebyte);
		// Set the stance by enumeration value
		void set_stance(Stance newstance);
		// Start the death animation
		void apply_death();
		// Decide on the next state
		void next_move();
		// Send the current position and state to the server
		void update_movement();

		// Calculate the hit chance
		float calculate_hitchance(int16_t leveldelta, int32_t accuracy) const;
		// Calculate the minimum damage
		double calculate_mindamage(int16_t leveldelta, double mindamage, bool magic) const;
		// Calculate the maximum damage
		double calculate_maxdamage(int16_t leveldelta, double maxdamage, bool magic) const;
		// Calculate a random damage line based on the specified values
		std::pair<int32_t, bool> next_damage(double mindamage, double maxdamage, float hitchance, float critical) const;

		// Return the current 'head' position
		Point<int16_t> get_head_position(Point<int16_t> position) const;

		std::map<Stance, Animation> animations;
		std::string name;
		Sound hitsound;
		Sound diesound;
		uint16_t level;
		float speed;
		float flyspeed;
		uint16_t watk;
		uint16_t matk;
		uint16_t wdef;
		uint16_t mdef;
		uint16_t accuracy;
		uint16_t avoid;
		uint16_t knockback;
		bool undead;
		bool touchdamage;
		bool noflip;
		bool notattack;
		bool canmove;
		bool canjump;
		bool canfly;

		EffectLayer effects;
		Text namelabel;
		MobHpBar hpbar;
		Randomizer randomizer;

		TimedBool showhp;

		std::vector<Movement> movements;
		uint16_t counter;

		int32_t id;
		int8_t effect;
		int8_t team;
		bool dying;
		bool dead;
		bool control;
		bool aggro;
		Stance stance;
		bool flip;
		FlyDirection flydirection;
		float walkforce;
		int8_t hppercent;
		bool fading;
		bool fadein;
		Linear<float> opacity;

		// Active status effects
		std::unordered_map<int32_t, MobStatusEntry> statuses;
		bool stunned;
		bool frozen;
		bool poisoned;
		bool sealed;
		bool doomed;
		bool shadowed; // shadow web
	};
}