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
#include "ForeignCharHandlers.h"

#include "../../Gameplay/Stage.h"
#include "../../Character/Buff.h"
#include "../../Character/OtherChar.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

#include <iomanip>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void DamagePlayerHandler::handle(InPacket& recv) const
	{
		// v83 DAMAGE_PLAYER: int cid, byte skill, int damage, ...
		// Shows damage taken by another player on the map
		if (recv.length() < 9)
			return;

		int32_t cid = recv.read_int();
		int8_t skill = recv.read_byte();

		if (skill == -3 && recv.length() >= 4)
			recv.read_int(); // padding 0

		int32_t damage = recv.read_int();

		bool has_knockback = false;
		int8_t direction = 0;

		if (skill != -4 && recv.length() >= 5)
		{
			int32_t monsteridfrom = recv.read_int();
			direction = recv.read_byte();
			has_knockback = true;

			(void)monsteridfrom;
		}

		// Show damage number on the character
		Optional<Char> character = Stage::get().get_character(cid);
		if (character)
		{
			character->show_damage(damage);

			// Knock the character back the way the server says they were hit.
			// Player::damage encodes this as `fromleft ? 0 : 1` where the flag
			// is true when the attacker is to the RIGHT, so 0 means knocked
			// left. Matching it here is what makes a foreign player recoil the
			// same way the local one does instead of absorbing hits in place.
			if (has_knockback && damage > 0)
				character->get_phobj().hspeed = (direction == 0) ? -1.5 : 1.5;
		}
	}

	void FacialExpressionHandler::handle(InPacket& recv) const
	{
		// v83 FACIAL_EXPRESSION: int cid, int expression
		if (recv.length() < 8)
			return;

		int32_t cid = recv.read_int();
		int32_t expression = recv.read_int();

		Optional<Char> character = Stage::get().get_character(cid);
		if (character)
			character->set_expression(expression);
	}

	void GiveForeignBuffHandler::handle(InPacket& recv) const
	{
		// v83 (Cosmic PacketCreator::giveForeignBuff): int cid, long firstmask,
		// long secondmask, then one short per set buffstat (written in
		// ascending mask-bit order), then int 0 + short 0 padding.
		// The mask bit values match Buffstat::first_codes / second_codes.
		if (recv.length() < 20)
			return;

		int32_t cid = recv.read_int();
		uint64_t firstmask = static_cast<uint64_t>(recv.read_long());
		uint64_t secondmask = static_cast<uint64_t>(recv.read_long());

		Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);

		// Monster Riding is broadcast with a custom body (showMonsterRiding):
		// short 0, int mountItemId, int skillId — the mount slot is never part
		// of the visible look, so the item id MUST come from the packet.
		if (firstmask & Buffstat::first_codes.at(Buffstat::Id::MONSTER_RIDING))
		{
			recv.read_short();
			int32_t mount_itemid = recv.read_int();

			if (ochar && mount_itemid != 0)
				ochar->set_riding(mount_itemid);

			return;
		}

		// First-mask buffs (Dash, Energy Charge, Speed Infusion, ...) have no
		// foreign-character effect path — consume their values to stay aligned.
		for (int bit = 0; bit < 64 && recv.length() >= 2; bit++)
			if (firstmask & (1ULL << bit))
				recv.read_short();

		for (int bit = 0; bit < 64 && recv.length() >= 2; bit++)
		{
			uint64_t code = 1ULL << bit;

			if (!(secondmask & code))
				continue;

			int16_t value = recv.read_short();
			(void)value;

			// Dark Sight / GM hide — render the character transparent.
			if (code == Buffstat::second_codes.at(Buffstat::Id::DARKSIGHT))
				if (ochar)
					ochar->set_hidden(true);

			// Remaining foreign buffs Cosmic broadcasts (MORPH, GHOST_MORPH,
			// SHADOWPARTNER, SOULARROW, COMBO, AURA, WK_CHARGE) have no
			// working visual path on OtherChar in this client — morphs and
			// shadow partner copies are not rendered, and SPEED/JUMP only
			// affect movement, which is already server-echoed for foreign
			// characters. Skill cast animations arrive separately via
			// SkillEffectHandler.
		}
	}

	void CancelForeignBuffHandler::handle(InPacket& recv) const
	{
		// v83 (Cosmic PacketCreator::cancelForeignBuff): int cid,
		// long firstmask, long secondmask
		if (recv.length() < 20)
			return;

		int32_t cid = recv.read_int();
		uint64_t firstmask = static_cast<uint64_t>(recv.read_long());
		uint64_t secondmask = static_cast<uint64_t>(recv.read_long());

		if (firstmask & Buffstat::first_codes.at(Buffstat::Id::MONSTER_RIDING))
			if (auto rc = Stage::get().get_chars().get_char(cid))
				rc->set_riding(0);

		Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);
		if (ochar)
		{
			// If DARKSIGHT was cancelled, unhide
			if (secondmask & Buffstat::second_codes.at(Buffstat::Id::DARKSIGHT))
				ochar->set_hidden(false);
		}
	}

	void UpdatePartyMemberHPHandler::handle(InPacket& recv) const
	{
		// v83: int cid, int hp, int maxhp
		// Updates a party member's HP in the party data
		if (recv.length() < 12)
			return;

		int32_t cid = recv.read_int();
		int32_t hp = recv.read_int();
		int32_t maxhp = recv.read_int();

		Stage::get().get_player().get_party().update_member_hp(cid, hp, maxhp);
	}

	void GuildNameChangedHandler::handle(InPacket& recv) const
	{
		// v83: int cid, string guildname
		// Updates guild name display for a character on the map
		if (recv.length() < 4)
			return;

		int32_t cid = recv.read_int();
		std::string guildname = recv.available() ? recv.read_string() : "";

		Optional<Char> character = Stage::get().get_character(cid);
		if (character)
			character->set_guild(guildname);
	}

	void GuildMarkChangedHandler::handle(InPacket& recv) const
	{
		// v83: int cid, short bg, byte bgcolor, short logo, byte logocolor
		// Updates guild mark/emblem for a character on the map
		if (recv.length() < 4)
			return;

		int32_t cid = recv.read_int();

		if (recv.length() >= 6)
		{
			int16_t bg = recv.read_short();
			int8_t bgcolor = recv.read_byte();
			int16_t logo = recv.read_short();
			int8_t logocolor = recv.read_byte();

			Optional<Char> character = Stage::get().get_character(cid);
			if (character)
				character->set_guild_mark(bg, bgcolor, logo, logocolor);
		}
	}

	void CancelChairHandler::handle(InPacket& recv) const
	{
		// v83: int cid
		// Cancels the chair sitting visual for a character
		if (recv.length() < 4)
			return;

		int32_t cid = recv.read_int();

		// Skip the local player — we manage our own chair state
		if (Stage::get().is_player(cid))
			return;

		Optional<Char> character = Stage::get().get_character(cid);
		if (character)
			character->set_state(Char::State::STAND);
	}

	void ShowItemEffectHandler::handle(InPacket& recv) const
	{
		// v83: int cid, int itemid
		// Toggles a persistent item-use aura on a character. itemid=0 clears.
		if (recv.length() < 8)
			return;

		int32_t cid = recv.read_int();
		int32_t itemid = recv.read_int();

		if (auto character = Stage::get().get_character(cid))
			character->set_item_effect(itemid);
	}

	// Opcode 196 (0xC4) — SHOW_CHAIR
	// Shows a chair being used by a foreign character on the map
	void ShowChairHandler::handle(InPacket& recv) const
	{
		int32_t charid = recv.read_int();
		int32_t itemid = recv.read_int();

		// Skip the local player — we manage our own chair state
		if (Stage::get().is_player(charid))
			return;

		if (auto other = Stage::get().get_character(charid))
		{
			if (itemid > 0)
				other->set_state(Char::State::SIT);
			else
				other->set_state(Char::State::STAND);
		}
	}

	void SkillEffectHandler::handle(InPacket& recv) const
	{
		// Skill effect on another player — shows the skill animation
		// Format: int cid, int skillid, byte level, byte flags, byte speed, byte direction
		if (!recv.available())
			return;

		int32_t cid = recv.read_int();
		int32_t skillid = recv.read_int();
		int8_t level = recv.read_byte();
		int8_t flags = recv.read_byte();
		int8_t speed = recv.read_byte();
		int8_t direction = recv.read_byte();

		// `flags` is the skill-specific display mask; nothing in the client reads
		// it yet, so it stays unused rather than being guessed at.
		(void)flags;

		Stage::get().get_combat().show_buff(cid, skillid, level,
			static_cast<uint8_t>(speed), direction);
	}

	void ThrowGrenadeHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int32_t x = recv.read_int();
		int32_t y = recv.read_int();
		int32_t key_down = recv.read_int();
		int32_t skill_id = recv.read_int();
		int32_t skill_level = recv.read_int();

		(void)key_down;

		// Throw animation on the caster...
		Stage::get().get_combat().show_buff(cid, skill_id,
			static_cast<int8_t>(skill_level));

		// ...then the blast where it lands. The skill's own `effect` node is the
		// authored explosion; skills without one fall back to nothing rather than
		// a placeholder, since a wrong effect reads worse than no effect.
		std::string job = std::to_string(skill_id / 10000);
		while (job.size() < 3)
			job.insert(0, 1, '0');

		nl::node blast = nl::nx::skill[job + ".img"]["skill"]
			[std::to_string(skill_id)]["effect"];

		Stage::get().get_point_effects().add(blast,
			Point<int16_t>(static_cast<int16_t>(x), static_cast<int16_t>(y)));
	}

	void PetNameChangeHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte(); // 0
		std::string new_name = recv.read_string();
		recv.read_byte(); // 0

		if (auto character = Stage::get().get_character(cid))
		{
			PetLook& pet = character->get_pet(0);

			if (pet.get_itemid() != 0)
				pet.set_name(new_name);
		}

		chat::log("[Pet] Name changed to: " + new_name, chat::LineType::YELLOW);
	}

	void PetExceptionListHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int8_t pet_index = recv.read_byte();
		recv.read_long(); // pet id
		int8_t count = recv.read_byte();

		for (int8_t i = 0; i < count; i++)
			recv.read_int(); // excluded item id

		chat::log("[Pet] Exception list updated: " + std::to_string(count) + " items excluded.", chat::LineType::YELLOW);
	}
}
