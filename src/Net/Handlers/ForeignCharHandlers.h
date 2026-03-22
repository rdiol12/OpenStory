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

#include "../PacketHandler.h"

namespace ms
{
	// Damage shown on another player (mob hit, environment, etc.)
	class DamagePlayerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Facial expression / emote shown on another player
	class FacialExpressionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Foreign buff applied to another player on the map
	class GiveForeignBuffHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Foreign buff removed from another player on the map
	class CancelForeignBuffHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Party member HP update
	class UpdatePartyMemberHPHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild name changed for a player on the map
	class GuildNameChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild mark/emblem changed for a player on the map
	class GuildMarkChangedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Chair visual cancelled for a player
	class CancelChairHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Item effect shown on a player
	class ShowItemEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Show chair visual on a foreign character (opcode 0xC4)
	class ShowChairHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Skill effect shown on another player
	class SkillEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Throw grenade visual
	class ThrowGrenadeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet name change notification
	// Opcode: PET_NAMECHANGE(172)
	class PetNameChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Pet auto-feed exception list
	// Opcode: PET_EXCEPTION_LIST(173)
	class PetExceptionListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}
