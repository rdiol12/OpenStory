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

	// Guild operation response (create, invite, expel, rank change, etc.)
	class GuildOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild BBS (bulletin board system) response
	class GuildBBSHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Guild alliance operations
	// Opcode: ALLIANCE_OPERATION(66)
	class AllianceOperationHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Maple Messenger chat window
	// Opcode: MESSENGER(313)
	class MessengerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Level up notification (guild/family broadcast)
	// Opcode: NOTIFY_LEVELUP(105)
	class NotifyLevelUpHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Job change notification (guild/family broadcast)
	// Opcode: NOTIFY_JOB_CHANGE(107)
	class NotifyJobChangeHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding progress step
	class WeddingProgressHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding ceremony end
	class WeddingCeremonyEndHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Marriage request / wishlist
	class MarriageRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Marriage result
	class MarriageResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Wedding gift result
	class WeddingGiftResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Partner map transfer notification
	class NotifyMarriedPartnerHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family chart/pedigree
	class FamilyChartResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family info
	class FamilyInfoResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family generic result
	class FamilyResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join request
	class FamilyJoinRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join request result
	class FamilyJoinRequestResultHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family join accepted
	class FamilyJoinAcceptedHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family reputation gain
	class FamilyRepGainHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family member login/logout
	class FamilyLoginLogoutHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family summon request
	class FamilySummonRequestHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Family privilege list
	class FamilyPrivilegeListHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};

	// Rock-Paper-Scissors game
	// Opcode: RPS_GAME(312)
	class RPSGameHandler : public PacketHandler
	{
		void handle(InPacket& recv) const override;
	};
}