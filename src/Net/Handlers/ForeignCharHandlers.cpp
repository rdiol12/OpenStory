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

		if (skill != -4 && recv.length() >= 5)
		{
			int32_t monsteridfrom = recv.read_int();
			int8_t direction = recv.read_byte();

			(void)monsteridfrom;
			(void)direction;
		}

		// Show damage number on the character
		Optional<Char> character = Stage::get().get_character(cid);
		if (character)
			character->show_damage(damage);
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
		// v83: int cid, long buffmask, then for each set bit: short value
		// Applies buff stat modifiers to another character on the map
		if (recv.length() < 4)
			return;

		int32_t cid = recv.read_int();

		if (recv.length() < 8)
			return;

		int64_t buffmask = recv.read_long();

		// Buff mask bits (v83):
		// bit 0 = PAD, bit 1 = PDD, bit 2 = MAD, bit 3 = MDD
		// bit 4 = ACC, bit 5 = EVA, bit 7 = SPEED, bit 8 = JUMP
		// bit 42 = DARKSIGHT
		uint8_t speed = 0;
		bool darksight = (buffmask & 0x40000000000LL) != 0;

		int bit = 0;
		for (int64_t mask = buffmask; mask != 0 && recv.length() >= 2; mask >>= 1, bit++)
		{
			if (mask & 1)
			{
				int16_t value = recv.read_short();

				// Speed buff (bit 7)
				if (bit == 7)
					speed = static_cast<uint8_t>(value);
			}
		}

		Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);
		if (ochar)
		{
			if (speed > 0)
				ochar->update_speed(speed);

			if (darksight)
				ochar->set_hidden(true);
		}
	}

	void CancelForeignBuffHandler::handle(InPacket& recv) const
	{
		// v83: int cid, long buffmask
		// Resets buff stat modifiers on another character
		if (recv.length() < 12)
			return;

		int32_t cid = recv.read_int();
		int64_t buffmask = recv.read_long();

		Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);
		if (ochar)
		{
			// If speed buff was cancelled (bit 7), reset speed
			if (buffmask & (1LL << 7))
				ochar->update_speed(100); // default speed

			// If DARKSIGHT was cancelled (bit 42), unhide
			if (buffmask & 0x40000000000LL)
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
		// Shows an item-use visual effect on a character
		if (recv.length() < 8)
			return;

		int32_t cid = recv.read_int();
		int32_t itemid = recv.read_int();

		// Load the item effect animation from Effect.wz
		nl::node effect_node = nl::nx::effect["ItemEff.img"][std::to_string(itemid)];

		if (effect_node)
		{
			Optional<Char> character = Stage::get().get_character(cid);
			if (character)
				character->show_attack_effect(Animation(effect_node), 0);
		}
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

		(void)flags;
		(void)speed;
		(void)direction;

		// Use Combat::show_buff which loads skill animation from NX and applies it
		Stage::get().get_combat().show_buff(cid, skillid, level);
	}

	void ThrowGrenadeHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int32_t x = recv.read_int();
		int32_t y = recv.read_int();
		int32_t key_down = recv.read_int();
		int32_t skill_id = recv.read_int();
		int32_t skill_level = recv.read_int();

		// Grenade visual — would need an animation system for projectiles
	}

	void PetNameChangeHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte(); // 0
		std::string new_name = recv.read_string();
		recv.read_byte(); // 0

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pet] Name changed to: " + new_name, UIChatBar::LineType::YELLOW);
	}

	void PetExceptionListHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int8_t pet_index = recv.read_byte();
		recv.read_long(); // pet id
		int8_t count = recv.read_byte();

		for (int8_t i = 0; i < count; i++)
			recv.read_int(); // excluded item id

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pet] Exception list updated: " + std::to_string(count) + " items excluded.", UIChatBar::LineType::YELLOW);
	}
}
