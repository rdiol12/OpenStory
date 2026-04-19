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
#include "MapObjectHandlers.h"

#include "Helpers/LoginParser.h"
#include "Helpers/MovementParser.h"

#include "../../Gameplay/Stage.h"
#include "../../Gameplay/MapleMap/Mob.h"
#include "../../Gameplay/MapleMap/Summon.h"
#include "../../Gameplay/MapleMap/Dragon.h"

#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace ms
{
	static void mob_debug_log(const char* fmt, ...)
	{
		static FILE* f = nullptr;
		if (!f)
		{
			f = std::fopen("mob_debug.log", "w");
			if (!f) return;
		}
		std::time_t t = std::time(nullptr);
		std::tm lt{};
#ifdef _WIN32
		localtime_s(&lt, &t);
#else
		localtime_r(&t, &lt);
#endif
		std::fprintf(f, "[%02d:%02d:%02d] ", lt.tm_hour, lt.tm_min, lt.tm_sec);
		va_list ap;
		va_start(ap, fmt);
		std::vfprintf(f, fmt, ap);
		va_end(ap);
		std::fputc('\n', f);
		std::fflush(f);
	}

	void SpawnCharHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();

		// We don't need to spawn the player twice
		if (Stage::get().is_player(cid))
			return;

		uint8_t level = recv.read_byte();
		std::string name = recv.read_string();

		recv.read_string();	// guildname
		recv.read_short();	// guildlogobg
		recv.read_byte();	// guildlogobgcolor
		recv.read_short();	// guildlogo
		recv.read_byte();	// guildlogocolor

		recv.skip(8);

		bool morphed = recv.read_int() == 2;
		int32_t buffmask1 = recv.read_int();
		int16_t buffvalue = 0;

		if (buffmask1 != 0)
			buffvalue = morphed ? recv.read_short() : recv.read_byte();

		recv.read_int(); // buffmask 2

		recv.skip(43);

		recv.read_int(); // 'mount'

		recv.skip(61);

		int16_t job = recv.read_short();
		LookEntry look = LoginParser::parse_look(recv);

		recv.read_int(); // count of 5110000 
		recv.read_int(); // 'itemeffect'
		recv.read_int(); // 'chair'

		Point<int16_t> position = recv.read_point();
		int8_t stance = recv.read_byte();

		recv.skip(3);

		for (size_t i = 0; i < 3; i++)
		{
			int8_t available = recv.read_byte();

			if (available == 1)
			{
				recv.read_byte();	// 'byte2'
				recv.read_int();	// petid
				recv.read_string();	// name
				recv.read_int();	// unique id
				recv.read_int();
				recv.read_point();	// pos
				recv.read_byte();	// stance
				recv.read_int();	// fhid
			}
			else
			{
				break;
			}
		}

		recv.read_int(); // mountlevel
		recv.read_int(); // mountexp
		recv.read_int(); // mounttiredness

		// Mini room indicator (shop/game/etc.)
		recv.read_byte();

		bool chalkboard = recv.read_bool();
		std::string chalktext = chalkboard ? recv.read_string() : "";

		recv.skip(3);
		recv.read_byte(); // team

		Stage::get().get_chars().spawn(
			{ cid, look, level, job, name, stance, position }
		);
	}

	void RemoveCharHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		Stage::get().get_chars().remove(cid);
	}

	void SpawnPetHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		Optional<Char> character = Stage::get().get_character(cid);

		if (!character)
			return;

		uint8_t petindex = recv.read_byte();
		int8_t mode = recv.read_byte();

		if (mode == 1)
		{
			recv.skip(1);

			int32_t itemid = recv.read_int();
			std::string name = recv.read_string();
			int32_t uniqueid = recv.read_int();

			recv.skip(4);

			Point<int16_t> pos = recv.read_point();
			uint8_t stance = recv.read_byte();
			int32_t fhid = recv.read_int();

			character->add_pet(petindex, itemid, name, uniqueid, pos, stance, fhid);
		}
		else if (mode == 0)
		{
			bool hunger = recv.read_bool();

			character->remove_pet(petindex, hunger);
		}
	}

	void CharMovedHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.skip(4);
		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		Stage::get().get_chars().send_movement(cid, movements);
	}

	void UpdateCharLookHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte();
		LookEntry look = LoginParser::parse_look(recv);

		Stage::get().get_chars().update_look(cid, look);
	}

	void ShowForeignEffectHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int8_t effect = recv.read_byte();

		switch (effect)
		{
		case 0: // level up
			Stage::get().show_character_effect(cid, CharEffect::LEVELUP);
			break;
		case 1: // skill use
		case 2: // skill affected
		{
			if (recv.length() >= 5)
			{
				int32_t skillid = recv.read_int();
				recv.read_byte(); // direction
				// Consume remaining bytes for this effect
				while (recv.available())
					recv.read_byte();

				Stage::get().get_combat().show_buff(cid, skillid, effect);
			}
			break;
		}
		case 6: // job change
		case 7: // quest complete
		case 8: // buff effect
		case 9: // recovery (item use)
		case 10: // recovery
		{
			if (recv.available())
				recv.read_byte();
			break;
		}
		case 11: // wheel of fortune
		case 14: // scroll success/failure
		{
			// No extra data
			break;
		}
		case 13: // card effect
			Stage::get().show_character_effect(cid, CharEffect::MONSTER_CARD);
			break;
		default:
			// Consume all remaining bytes to avoid underflow on next packet
			break;
		}
	}

	void SpawnMobHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		recv.read_byte(); // 5 if controller == null
		int32_t id = recv.read_int();

		recv.skip(16);

		Point<int16_t> position = recv.read_point();
		int8_t stance = recv.read_byte();

		recv.skip(2);

		uint16_t fh = recv.read_short();
		int8_t effect = recv.read_byte();

		if (effect > 0)
		{
			recv.read_byte();
			recv.read_short();

			if (effect == 15)
				recv.read_byte();
		}

		int8_t team = recv.read_byte();

		recv.skip(4);

		mob_debug_log("SPAWN_MOB oid=%d id=%d pos=(%d,%d) stance=%d fh=%u effect=%d team=%d newspawn=%d",
			oid, id, position.x(), position.y(), (int)stance, (unsigned)fh, (int)effect, (int)team, (effect == -2) ? 1 : 0);

		Stage::get().get_mobs().spawn(
			{ oid, id, 0, stance, fh, effect == -2, team, position }
		);
	}

	void KillMobHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t animation = recv.read_byte();

		mob_debug_log("KILL_MOB oid=%d animation=%d", oid, (int)animation);

		Stage::get().get_mobs().remove(oid, animation);
	}

	void SpawnMobControllerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();

		mob_debug_log("SPAWN_MOB_C oid=%d mode=%d avail=%d", oid, (int)mode, (int)recv.available());

		if (mode == 0)
		{
			Stage::get().get_mobs().set_control(oid, false);
		}
		else
		{
			if (recv.available())
			{
				recv.skip(1);

				int32_t id = recv.read_int();

				recv.skip(16);

				Point<int16_t> position = recv.read_point();
				int8_t stance = recv.read_byte();

				recv.skip(2);

				uint16_t fh = recv.read_short();
				int8_t effect = recv.read_byte();

				if (effect > 0)
				{
					recv.read_byte();
					recv.read_short();

					if (effect == 15)
						recv.read_byte();
				}

				int8_t team = recv.read_byte();

				recv.skip(4);

				Stage::get().get_mobs().spawn(
					{ oid, id, mode, stance, fh, effect == -2, team, position }
				);
			}
			else
			{
				// Monster invisibility flag (event script usage)
			}
		}
	}

	void MobMovedHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		recv.read_byte();
		recv.read_byte(); // useskill
		recv.read_byte(); // skill
		recv.read_byte(); // skill 1
		recv.read_byte(); // skill 2
		recv.read_byte(); // skill 3
		recv.read_byte(); // skill 4

		Point<int16_t> position = recv.read_point();
		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		Stage::get().get_mobs().send_movement(oid, position, std::move(movements));
	}

	void ShowMobHpHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t hppercent = recv.read_byte();
		uint16_t playerlevel = Stage::get().get_player().get_stats().get_stat(MapleStat::Id::LEVEL);

		Stage::get().get_mobs().send_mobhp(oid, hppercent, playerlevel);
	}

	void SpawnNpcHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int32_t id = recv.read_int();
		Point<int16_t> position = recv.read_point();
		bool flip = recv.read_bool();
		uint16_t fh = recv.read_short();

		recv.read_short(); // rx0
		recv.read_short(); // rx1
		recv.read_byte();  // minimap (1=show on minimap)

		Stage::get().get_npcs().spawn(
			{ oid, id, position, flip, fh }
		);
	}

	void SpawnNpcControllerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();

		if (mode == 0)
		{
			Stage::get().get_npcs().remove(oid);
		}
		else
		{
			int32_t id = recv.read_int();
			Point<int16_t> position = recv.read_point();
			bool flip = recv.read_bool();
			uint16_t fh = recv.read_short();

			recv.read_short();	// 'rx'
			recv.read_short();	// 'ry'
			recv.read_bool();	// 'minimap'

			Stage::get().get_npcs().spawn(
				{ oid, id, position, flip, fh }
			);
		}
	}

	void DropLootHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();
		bool meso = recv.read_bool();
		int32_t itemid = recv.read_int();
		int32_t owner = recv.read_int();
		int8_t pickuptype = recv.read_byte();
		Point<int16_t> dropto = recv.read_point();

		recv.skip(4);

		Point<int16_t> dropfrom;

		if (mode != 2)
		{
			dropfrom = recv.read_point();

			recv.skip(2);

			Sound(Sound::Name::DROP).play();
		}
		else
		{
			dropfrom = dropto;
		}

		if (!meso)
			recv.skip(8);

		bool playerdrop = !recv.read_bool();

		Stage::get().get_drops().spawn(
			{ oid, itemid, meso, owner, dropfrom, dropto, pickuptype, mode, playerdrop }
		);
	}

	void RemoveLootHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();

		Optional<PhysicsObject> looter;

		if (mode > 1)
		{
			int32_t cid = recv.read_int();

			if (recv.length() > 0)
				recv.read_byte(); // pet
			else if (auto character = Stage::get().get_character(cid))
				looter = character->get_phobj();

			Sound(Sound::Name::PICKUP).play();
		}

		Stage::get().get_drops().remove(oid, mode, looter.get());
	}

	void HitReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();
		int8_t stance = recv.read_byte();
		recv.read_short(); // unknown
		int8_t frame_delay = recv.read_byte();

		Stage::get().get_reactors().trigger(oid, state);
	}

	void SpawnReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int32_t rid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();
		int8_t facing = recv.read_byte();     // facingDirection
		recv.read_short();                     // padding (0)

		Stage::get().get_reactors().spawn(
			{ oid, rid, state, point }
		);
	}

	void RemoveReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();

		Stage::get().get_reactors().remove(oid, state, point);
	}

	void ApplyMonsterStatusHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		recv.skip(8); // long 0 padding

		int32_t first_mask = recv.read_int();
		int32_t second_mask = recv.read_int();

		std::vector<std::pair<int32_t, MobStatusEntry>> statuses;

		// Parse statuses from first mask (isFirst() statuses)
		for (int32_t bit = 1; bit != 0; bit <<= 1)
		{
			if (first_mask & bit)
			{
				MobStatusEntry entry;
				entry.value = recv.read_short();
				entry.skillid = recv.read_int(); // skill ID or mob skill (type << 16 | level)
				entry.duration = recv.read_short(); // -1 = not displayed
				statuses.emplace_back(bit, entry);
			}
		}

		// Parse statuses from second mask (normal statuses)
		for (int32_t bit = 1; bit != 0; bit <<= 1)
		{
			if (second_mask & bit)
			{
				MobStatusEntry entry;
				entry.value = recv.read_short();
				entry.skillid = recv.read_int();
				entry.duration = recv.read_short();
				statuses.emplace_back(bit, entry);
			}
		}

		// Skip reflection data + size + padding
		if (recv.length() > 0)
		{
			// Remaining: optional reflection ints, byte size, int 0
			// Just skip whatever is left
		}

		Stage::get().get_mobs().apply_status(oid, second_mask, first_mask, statuses);
	}

	void CancelMonsterStatusHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		// Padding
		recv.skip(8); // long 0

		int32_t first_mask = recv.read_int();
		int32_t second_mask = recv.read_int();

		Stage::get().get_mobs().cancel_status(oid, second_mask, first_mask);
	}

	void SpawnSummonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t oid = recv.read_int();
		int32_t skill_id = recv.read_int();
		recv.skip(1); // 0x0A marker
		int8_t skill_level = recv.read_byte();
		Point<int16_t> position = recv.read_point();
		int8_t stance = recv.read_byte();
		recv.skip(2); // padding short 0
		int8_t move_type_val = recv.read_byte();
		bool attacks = recv.read_bool();
		// recv.read_bool(); // animated flag (not needed client-side)

		Summon::MovementType move_type;

		switch (move_type_val)
		{
		case 1:
			move_type = Summon::MovementType::FOLLOW;
			break;
		case 3:
			move_type = Summon::MovementType::CIRCLE_FOLLOW;
			break;
		default:
			move_type = Summon::MovementType::STATIONARY;
			break;
		}

		Stage::get().get_summons().spawn(
			{ oid, owner_id, skill_id, skill_level, stance, position, move_type, attacks }
		);
	}

	void RemoveSummonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t oid = recv.read_int();
		int8_t anim = recv.read_byte();

		Stage::get().get_summons().remove(oid, anim == 4);
	}

	void MoveSummonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t oid = recv.read_int();
		Point<int16_t> start = recv.read_point();

		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		Stage::get().get_summons().send_movement(oid, start, std::move(movements));
	}

	void SummonAttackHandler::handle(InPacket& recv) const
	{
		// Parse but don't need to act on client-side — damage is handled by mob HP updates
		int32_t owner_id = recv.read_int();
		int32_t summon_oid = recv.read_int();
		recv.skip(1); // char level
		recv.skip(1); // direction
		int8_t num_targets = recv.read_byte();

		for (int8_t i = 0; i < num_targets; i++)
		{
			recv.read_int(); // monster oid
			recv.skip(1);    // 6
			recv.read_int(); // damage
		}
	}

	void DamageSummonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t oid = recv.read_int();
		recv.skip(1); // 12
		int32_t damage = recv.read_int();

		Stage::get().get_summons().apply_damage(oid, damage);
	}

	void RemoveNpcHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		Stage::get().get_npcs().remove(oid);
	}

	void SpawnDoorHandler::handle(InPacket& recv) const
	{
		bool launched = recv.read_bool();
		int32_t owner_id = recv.read_int();
		Point<int16_t> pos = recv.read_point();

		Stage::get().get_doors().spawn(
			{ owner_id, owner_id, pos, launched }
		);
	}

	void RemoveDoorHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // always 0
		int32_t owner_id = recv.read_int();

		Stage::get().get_doors().remove(owner_id);
	}

	void SpawnMistHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int32_t mist_type = recv.read_int();  // 0=mob, 1=poison, 2=smokescreen, 4=recovery
		int32_t owner_id = recv.read_int();
		int32_t skill_id = recv.read_int();
		int8_t skill_level = recv.read_byte();
		int16_t delay = recv.read_short();
		int32_t x1 = recv.read_int();
		int32_t y1 = recv.read_int();
		int32_t x2 = recv.read_int();
		int32_t y2 = recv.read_int();
		recv.read_int(); // unknown

		Point<int16_t> pos1(static_cast<int16_t>(x1), static_cast<int16_t>(y1));
		Point<int16_t> pos2(static_cast<int16_t>(x2), static_cast<int16_t>(y2));

		Stage::get().get_mists().spawn(
			{ oid, owner_id, pos1, pos2, skill_id, skill_level, static_cast<int8_t>(mist_type) }
		);
	}

	void RemoveMistHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		Stage::get().get_mists().remove(oid);
	}

	void MovePetHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		uint8_t slot = recv.read_byte();
		recv.read_int(); // pet id

		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		if (auto character = Stage::get().get_character(cid))
		{
			if (!movements.empty())
			{
				const Movement& last = movements.back();
				// PetLook doesn't support full movement queues,
				// so just update position and stance from last movement
			}
		}
	}

	void PetChatHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte(); // pet index
		recv.read_byte(); // 0
		int8_t act = recv.read_byte();
		std::string text = recv.read_string();
		recv.read_byte(); // has balloon type

		// Display pet chat as a speech bubble on the character
		if (auto character = Stage::get().get_character(cid))
			character->speak(text);
	}

	void PetCommandHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		uint8_t pet_index = recv.read_byte();
		int8_t type = recv.read_byte();     // 1 = command response
		int8_t animation = recv.read_byte();
		bool talk = !recv.read_bool();
		// recv.read_bool(); // balloon type - may not be present

		// Pet command animation — no visual handler yet
	}

	void MoveMonsterResponseHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int16_t move_id = recv.read_short();
		bool use_skills = recv.read_bool();
		int16_t current_mp = recv.read_short();
		int8_t skill_id = recv.read_byte();
		int8_t skill_level = recv.read_byte();

		// Server acknowledgement of mob movement — client doesn't need to act
	}

	void DamageMonsterHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		recv.read_byte(); // 0
		int32_t damage = recv.read_int();

		if (damage > 0)
		{
			// Show damage number on the mob (from other players' attacks)
			// The mob HP bar is updated separately via SHOW_MOB_HP
		}
	}

	void CancelSkillEffectHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int32_t skill_id = recv.read_int();

		// Cancel the visual effect of a skill on a character
		// The buff cancellation is handled separately via CANCEL_FOREIGN_BUFF
	}

	void ChalkboardHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		bool has_chalkboard = recv.read_bool();
		std::string text = has_chalkboard ? recv.read_string() : "";

		// Chalkboard display above character — visual only
		// Would need a chalkboard text bubble system on OtherChar
	}

	void SpawnDragonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int16_t x = recv.read_short();
		recv.read_short(); // padding
		int16_t y = recv.read_short();
		recv.read_short(); // padding
		uint8_t stance = recv.read_byte();
		recv.read_byte(); // 0
		int16_t job = recv.read_short();

		Stage::get().get_dragons().spawn(
			{ owner_id, Point<int16_t>(x, y), stance, job }
		);
	}

	void MoveDragonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		Point<int16_t> start = recv.read_point();

		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		Stage::get().get_dragons().send_movement(owner_id, movements);
	}

	void RemoveDragonHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();

		Stage::get().get_dragons().remove(owner_id);
	}

	void SummonSkillHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int32_t skill_id = recv.read_int();
		int8_t new_stance = recv.read_byte();

		// Visual update only — the summon's skill animation
	}
}