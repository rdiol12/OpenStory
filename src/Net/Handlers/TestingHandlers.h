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
	// Handles the server response for secondary password (PIC) verification
	class CheckSpwResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Handles map field effects such as screen animations and environmental effects
	class FieldEffectHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Stub handlers for unhandled v83 packets — all log their data for debugging
	class RelogResponseHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class UpdateQuestInfoHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class FameResponseHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BuddyListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class FamilyHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class PartyOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class ClockHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class ForcedStatSetHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class SetTractionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class NpcActionHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class YellowTipHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class CatchMonsterHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	class BlowWeatherHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}