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

#include "../OutPacket.h"

namespace ms
{
	// --- RPS Game Packets ---

	// Send RPS selection to server
	class RPSSelectionPacket : public OutPacket
	{
	public:
		RPSSelectionPacket(int8_t selection) : OutPacket(OutPacket::Opcode::RPS_ACTION)
		{
			write_byte(selection);  // 0=rock, 1=paper, 2=scissors
		}
	};

	// --- Messenger Packets ---

	// Open messenger
	class MessengerOpenPacket : public OutPacket
	{
	public:
		MessengerOpenPacket() : OutPacket(OutPacket::Opcode::MESSENGER)
		{
			write_byte(0);  // OPEN
		}
	};

	// Invite player to messenger
	class MessengerInvitePacket : public OutPacket
	{
	public:
		MessengerInvitePacket(const std::string& name) : OutPacket(OutPacket::Opcode::MESSENGER)
		{
			write_byte(3);  // INVITE
			write_string(name);
		}
	};

	// Send chat in messenger
	class MessengerChatPacket : public OutPacket
	{
	public:
		MessengerChatPacket(const std::string& message) : OutPacket(OutPacket::Opcode::MESSENGER)
		{
			write_byte(6);  // CHAT
			write_string(message);
		}
	};

	// Leave messenger
	class MessengerLeavePacket : public OutPacket
	{
	public:
		MessengerLeavePacket() : OutPacket(OutPacket::Opcode::MESSENGER)
		{
			write_byte(2);  // LEAVE
		}
	};

	// --- Family Packets ---

	// Use family special ability.
	//  - Buffs (drop/exp/combo):         FamilyUsePacket(type)
	//  - Teleport to / Summon member:    FamilyUsePacket(type, target_name)
	// Cosmic's FamilyUseHandler does `readInt()` and, for REUNION/SUMMON,
	// also `readString()` for the target.
	class FamilyUsePacket : public OutPacket
	{
	public:
		FamilyUsePacket(int32_t type) : OutPacket(OutPacket::Opcode::USE_FAMILY)
		{
			write_int(type);
		}

		FamilyUsePacket(int32_t type, const std::string& target_name)
			: OutPacket(OutPacket::Opcode::USE_FAMILY)
		{
			write_int(type);
			write_string(target_name);
		}
	};

	// Invite a player to become my junior (ADD_FAMILY, 0x93).
	// Cosmic's FamilyAddHandler reads a single character-name string.
	class FamilyAddPacket : public OutPacket
	{
	public:
		FamilyAddPacket(const std::string& name) : OutPacket(OutPacket::Opcode::ADD_FAMILY)
		{
			write_string(name);
		}
	};

	// Leave the family (self). Cosmic's FamilySeparateHandler treats an
	// empty-payload packet on opcode 0x95 as "by junior" — i.e. me
	// breaking the relation with my senior.
	class FamilySeparateLeavePacket : public OutPacket
	{
	public:
		FamilySeparateLeavePacket()
			: OutPacket(OutPacket::Opcode::SEPARATE_FAMILY_BY_JUNIOR)
		{
		}
	};

	// Ask server for the tree / pedigree chart (OPEN_FAMILY_PEDIGREE).
	class FamilyPedigreeRequestPacket : public OutPacket
	{
	public:
		FamilyPedigreeRequestPacket(int32_t cid)
			: OutPacket(OutPacket::Opcode::OPEN_FAMILY_PEDIGREE)
		{
			write_int(cid);
		}
	};

	// Set the family motto / precepts (leader only; max 200 chars).
	// Cosmic's FamilyPreceptsHandler reads a single string.
	class FamilyPreceptsPacket : public OutPacket
	{
	public:
		FamilyPreceptsPacket(const std::string& text)
			: OutPacket(OutPacket::Opcode::CHANGE_FAMILY_MESSAGE)
		{
			write_string(text);
		}
	};

	// Accept / decline a family-summon invite. Cosmic's handler reads
	// a family-name string (discarded) plus a single byte flag.
	class FamilySummonResponsePacket : public OutPacket
	{
	public:
		FamilySummonResponsePacket(const std::string& family_name, bool accept)
			: OutPacket(OutPacket::Opcode::FAMILY_SUMMON_RESPONSE)
		{
			write_string(family_name);
			write_byte(accept ? 1 : 0);
		}
	};

	// Decline messenger invite by sending a note back to the inviter.
	class MessengerDeclinePacket : public OutPacket
	{
	public:
		MessengerDeclinePacket(const std::string& inviter_name, const std::string& text)
			: OutPacket(OutPacket::Opcode::MESSENGER)
		{
			write_byte(5);  // NOTE (decline message)
			write_string(text);
			write_string(inviter_name);
			write_byte(0);
		}
	};

	// Accept family join request. Cosmic's AcceptFamilyHandler reads
	// int inviterId, a (discarded) string, and a byte accept-flag.
	class FamilyAcceptPacket : public OutPacket
	{
	public:
		FamilyAcceptPacket(int32_t inviter_id) : OutPacket(OutPacket::Opcode::ACCEPT_FAMILY)
		{
			write_int(inviter_id);
			write_string("");
			write_byte(1);  // accept = true
		}
	};

	// --- Party Search Packets ---

	// Register for party search. Cosmic's handler is an empty stub —
	// players are auto-attached to the search pool when partyless via
	// `Character.updatePartySearchAvailability(true)`. This packet is
	// kept for compatibility with other server forks; it sends no
	// payload.
	class PartySearchRegisterPacket : public OutPacket
	{
	public:
		PartySearchRegisterPacket() : OutPacket(OutPacket::Opcode::PARTY_SEARCH_REGISTER) {}
	};

	// Start a party search. Leader-only; the server validates and
	// queues the leader with PartySearchCoordinator, which auto-pushes
	// `partySearchInvite` to matching candidates.
	//   int min_level   — minimum level filter
	//   int max_level   — maximum level filter (max-min must be ≤ 30)
	//   int members     — desired member count (Cosmic reads-but-ignores)
	//   int jobs_mask   — bitmask of accepted job categories
	class PartySearchStartPacket : public OutPacket
	{
	public:
		PartySearchStartPacket(int32_t min_level, int32_t max_level,
			int32_t members, int32_t jobs_mask)
			: OutPacket(OutPacket::Opcode::PARTY_SEARCH_START)
		{
			write_int(min_level);
			write_int(max_level);
			write_int(members);
			write_int(jobs_mask);
		}
	};

	// --- Report Packets ---

	// Report a player (type 0 = illegal program, type 1 = conversation)
	// Opcode: REPORT(106)
	class ReportPacket : public OutPacket
	{
	public:
		ReportPacket(int8_t type, const std::string& victim, int8_t reason, const std::string& description)
			: OutPacket(OutPacket::Opcode::REPORT)
		{
			write_byte(type);
			write_string(victim);
			write_byte(reason);
			write_string(description);
		}

		// Conversation report includes chat log
		ReportPacket(const std::string& victim, int8_t reason, const std::string& description, const std::string& chatlog)
			: OutPacket(OutPacket::Opcode::REPORT)
		{
			write_byte(1);  // conversation report
			write_string(victim);
			write_byte(reason);
			write_string(description);
			write_string(chatlog);
		}
	};

	// --- Minigame Packets ---

	// Place a piece (Omok) or flip a card (Memory)
	class MinigameMovePacket : public OutPacket
	{
	public:
		MinigameMovePacket(int32_t move_data) : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(42);  // GAME_MOVE
			write_int(move_data);
		}
	};

	// Ready/unready in minigame
	class MinigameReadyPacket : public OutPacket
	{
	public:
		MinigameReadyPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(40);  // READY
		}
	};

	// Start minigame (host only)
	class MinigameStartPacket : public OutPacket
	{
	public:
		MinigameStartPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(41);  // START
		}
	};

	// Forfeit/give up minigame
	class MinigameForfeitPacket : public OutPacket
	{
	public:
		MinigameForfeitPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(44);  // GIVE_UP
		}
	};

	// Exit minigame room
	class MinigameExitPacket : public OutPacket
	{
	public:
		MinigameExitPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(10);  // EXIT
		}
	};

	// Offer a draw in minigame
	class MinigameDrawPacket : public OutPacket
	{
	public:
		MinigameDrawPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(45);  // REQUEST_TIE
		}
	};

	// Skip turn in minigame
	class MinigameSkipPacket : public OutPacket
	{
	public:
		MinigameSkipPacket() : OutPacket(OutPacket::Opcode::PLAYER_INTERACTION)
		{
			write_byte(43);  // SKIP
		}
	};

	// --- Guild Packets ---

	// Guild operation base
	class GuildOperationPacket : public OutPacket
	{
	public:
		enum Operation : int8_t
		{
			CREATE = 2,
			INVITE = 5,
			ACCEPT = 6,
			LEAVE = 7,
			EXPEL = 8,
			CHANGE_RANK_TITLES = 9,
			CHANGE_MEMBER_RANK = 10,
			CHANGE_EMBLEM = 11,
			CHANGE_NOTICE = 13,
			REQUEST_BBS = 18
		};

	protected:
		GuildOperationPacket(Operation op) : OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(op);
		}
	};

	// Invite player to guild
	class GuildInvitePacket : public GuildOperationPacket
	{
	public:
		GuildInvitePacket(const std::string& name) : GuildOperationPacket(GuildOperationPacket::Operation::INVITE)
		{
			write_string(name);
		}
	};

	// Accept guild invite
	class AcceptGuildInvitePacket : public OutPacket
	{
	public:
		AcceptGuildInvitePacket(int32_t guild_id, int32_t char_id) : OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(GuildOperationPacket::Operation::ACCEPT);
			write_int(guild_id);
			write_int(char_id);
		}
	};

	// Deny guild invite
	class DenyGuildInvitePacket : public OutPacket
	{
	public:
		DenyGuildInvitePacket(const std::string& inviter) : OutPacket(OutPacket::Opcode::DENY_GUILD_REQUEST)
		{
			write_byte(0);
			write_string(inviter);
		}
	};

	// Leave guild
	class GuildLeavePacket : public GuildOperationPacket
	{
	public:
		GuildLeavePacket() : GuildOperationPacket(GuildOperationPacket::Operation::LEAVE) {}
	};

	// Expel member from guild
	class GuildExpelPacket : public GuildOperationPacket
	{
	public:
		GuildExpelPacket(int32_t cid, const std::string& name) : GuildOperationPacket(GuildOperationPacket::Operation::EXPEL)
		{
			write_int(cid);
			write_string(name);
		}
	};

	// Change guild notice
	class GuildChangeNoticePacket : public GuildOperationPacket
	{
	public:
		GuildChangeNoticePacket(const std::string& notice) : GuildOperationPacket(GuildOperationPacket::Operation::CHANGE_NOTICE)
		{
			write_string(notice);
		}
	};

	// Request guild BBS
	class GuildBBSRequestPacket : public GuildOperationPacket
	{
	public:
		GuildBBSRequestPacket() : GuildOperationPacket(GuildOperationPacket::Operation::REQUEST_BBS) {}
	};

	// Write a new post to guild BBS
	class GuildBBSWritePacket : public OutPacket
	{
	public:
		GuildBBSWritePacket(bool is_notice, const std::string& title, const std::string& content)
			: OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(18); // BBS operation
			write_byte(0);  // new post (not edit)
			write_byte(is_notice ? 1 : 0);
			write_string(title);
			write_string(content);
		}
	};

	// Delete a post from guild BBS
	class GuildBBSDeletePacket : public OutPacket
	{
	public:
		GuildBBSDeletePacket(int32_t post_id)
			: OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(18); // BBS operation
			write_byte(1);  // delete
			write_int(post_id);
		}
	};

	// Reply to a post on guild BBS
	class GuildBBSReplyPacket : public OutPacket
	{
	public:
		GuildBBSReplyPacket(int32_t post_id, const std::string& content)
			: OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(18); // BBS operation
			write_byte(2);  // reply
			write_int(post_id);
			write_string(content);
		}
	};

	// List posts on guild BBS with pagination
	class GuildBBSListPacket : public OutPacket
	{
	public:
		GuildBBSListPacket(int32_t start)
			: OutPacket(OutPacket::Opcode::GUILD_OPERATION)
		{
			write_byte(18); // BBS operation
			write_byte(4);  // list posts
			write_int(start);
		}
	};

	// --- Alliance Packets ---

	// Request alliance info (sub-op 0x01)
	class AllianceInfoRequestPacket : public OutPacket
	{
	public:
		AllianceInfoRequestPacket() : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x01);
		}
	};

	// Leave alliance (sub-op 0x02)
	class AllianceLeavePacket : public OutPacket
	{
	public:
		AllianceLeavePacket() : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x02);
		}
	};

	// Invite guild to alliance (sub-op 0x03)
	class AllianceInvitePacket : public OutPacket
	{
	public:
		AllianceInvitePacket(const std::string& guild_name) : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x03);
			write_string(guild_name);
		}
	};

	// Accept alliance invite (sub-op 0x04)
	class AllianceAcceptInvitePacket : public OutPacket
	{
	public:
		AllianceAcceptInvitePacket(int32_t alliance_id) : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x04);
			write_int(alliance_id);
		}
	};

	// Expel guild from alliance (sub-op 0x06)
	class AllianceExpelGuildPacket : public OutPacket
	{
	public:
		AllianceExpelGuildPacket(int32_t guild_id, int32_t alliance_id) : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x06);
			write_int(guild_id);
			write_int(alliance_id);
		}
	};

	// Change alliance rank titles (sub-op 0x08)
	class AllianceChangeRankTitlesPacket : public OutPacket
	{
	public:
		AllianceChangeRankTitlesPacket(const std::string ranks[5]) : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x08);
			for (int i = 0; i < 5; i++)
				write_string(ranks[i]);
		}
	};

	// Change alliance notice (sub-op 0x0A)
	class AllianceChangeNoticePacket : public OutPacket
	{
	public:
		AllianceChangeNoticePacket(const std::string& notice) : OutPacket(OutPacket::Opcode::ALLIANCE_OPERATION)
		{
			write_byte(0x0A);
			write_string(notice);
		}
	};

	// Deny alliance invite
	class DenyAllianceInvitePacket : public OutPacket
	{
	public:
		DenyAllianceInvitePacket(const std::string& inviter, const std::string& guild_name) : OutPacket(OutPacket::Opcode::DENY_ALLIANCE_REQUEST)
		{
			write_byte(0);
			write_string(inviter);
			write_string(guild_name);
		}
	};
}
