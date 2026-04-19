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
#include "SocialHandlers.h"

#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"

#include "../../Character/BuddyList.h"
#include "../../Character/Party.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIStatusBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

#include "../Packets/GameplayPackets.h"
#include "../Packets/SocialPackets.h"
#include "../Packets/BuddyPackets.h"
#include "../../IO/UITypes/UIGuild.h"
#include "../../IO/UITypes/UIGuildBBS.h"
#include "../../IO/UITypes/UIMessenger.h"
#include "../../IO/UITypes/UIFamily.h"
#include "../../IO/UITypes/UIAlliance.h"
#include "../../IO/UITypes/UIWedding.h"
#include "../../IO/UITypes/UIRPSGame.h"
#include "Helpers/LoginParser.h"
#include "../../Configuration.h"

namespace ms
{
	void BuddyListHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();

		switch (operation)
		{
		case 7:
		{
			// Full buddy list update
			if (!recv.available())
				break;

			int8_t count = recv.read_byte();
			std::map<int32_t, BuddyEntry> entries;

			for (int8_t i = 0; i < count; i++)
			{
				if (recv.length() < 4)
					break;

				BuddyEntry entry;
				entry.cid = recv.read_int();
				entry.name = recv.read_padded_string(13);
				entry.status = recv.read_byte();
				entry.channel = recv.read_int();
				entry.group = recv.read_padded_string(13);
				recv.read_int(); // padding (0)

				entries[entry.cid] = entry;
			}

			// Skip mapId block
			for (int8_t i = 0; i < count; i++)
			{
				if (recv.length() >= 4)
					recv.read_int();
			}

			Stage::get().get_player().get_buddylist().update(entries);
			break;
		}
		case 9:
		{
			// Incoming friend request — show Yes/No popup
			if (recv.length() < 4)
				break;

			int32_t from_cid = recv.read_int();
			std::string from_name = recv.read_string();

			UI::get().emplace<UIYesNo>(
				from_name + " wants to be your buddy.",
				[from_cid](bool yes)
				{
					if (yes)
						AcceptBuddyPacket(from_cid).dispatch();
					// Decline = do nothing (request stays pending)
				}
			);

			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();

			break;
		}
		case 0x14:
		{
			// Single buddy channel update
			if (recv.length() < 4)
				break;

			int32_t cid = recv.read_int();
			recv.read_byte(); // 0
			int32_t channel = recv.read_int();

			// TODO: update buddy's channel in the list and show login/logout message
			break;
		}
		case 0x15:
		{
			// Buddy capacity changed
			if (recv.available())
			{
				int8_t capacity = recv.read_byte();
				Stage::get().get_player().get_buddylist().set_capacity(capacity);
			}
			break;
		}
		default:
			break;
		}
	}

	void FamilyHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x00: // Family chart — tree of members
		{
			// charId, name, job, level, reputation, totalJuniors per member
			// Just consume the data
			while (recv.available())
				recv.read_byte();

			break;
		}
		case 0x01: // Family info — senior, juniors, rep
		{
			while (recv.available())
				recv.read_byte();

			break;
		}
		case 0x02: // Use entitlement result
		{
			while (recv.available())
				recv.read_byte();

			break;
		}
		case 0x07: // Invite
		{
			if (!Setting<AllowFamilyInvite>::get().load())
				break;

			std::string senior_name = recv.read_string();
			int32_t senior_cid = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(senior_name + " has invited you to join their family.", UIChatBar::LineType::YELLOW);

			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();

			break;
		}
		case 0x0B: // Buff notification from family member
		{
			while (recv.available())
				recv.read_byte();

			break;
		}
		default:
			break;
		}
	}

	namespace
	{
		void parse_party_data(InPacket& recv, int32_t partyid)
		{
			// v83 full party data block:
			// 6x int cid, 6x padded(13) name, 6x int job, 6x int level,
			// 6x int channel, 6x int mapid, int leader_cid,
			// then door data and misc we skip
			if (recv.length() < 4 * 6)
				return;

			int32_t cids[6];
			for (int i = 0; i < 6; i++)
				cids[i] = recv.read_int();

			std::string names[6];
			for (int i = 0; i < 6; i++)
				names[i] = recv.read_padded_string(13);

			int32_t jobs[6];
			for (int i = 0; i < 6; i++)
				jobs[i] = recv.read_int();

			int32_t levels[6];
			for (int i = 0; i < 6; i++)
				levels[i] = recv.read_int();

			int32_t channels[6];
			for (int i = 0; i < 6; i++)
				channels[i] = recv.read_int();

			int32_t mapids[6];
			for (int i = 0; i < 6; i++)
				mapids[i] = recv.read_int();

			int32_t leader_cid = recv.read_int();

			std::vector<PartyMember> members;

			for (int i = 0; i < 6; i++)
			{
				if (cids[i] == 0)
					continue;

				PartyMember member;
				member.cid = cids[i];
				member.name = names[i];
				member.job = static_cast<int16_t>(jobs[i]);
				member.level = static_cast<int16_t>(levels[i]);
				member.channel = channels[i];
				member.mapid = mapids[i];
				member.online = channels[i] >= 0;
				members.push_back(member);
			}

			Stage::get().get_player().get_party().update(partyid, members, leader_cid);
		}
	}

	void PartyOperationHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();
		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (operation)
		{
		case 4:
		{
			// Invite received — show Yes/No popup
			if (!Setting<AllowPartyInvite>::get().load())
				break;

			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			std::string from_name = recv.read_string();

			// Strip "PS: " prefix from party search invites
			std::string display_name = from_name;
			if (display_name.substr(0, 4) == "PS: ")
				display_name = display_name.substr(4);

			UI::get().emplace<UIYesNo>(
				display_name + " has invited you to their party.",
				[partyid, from_name](bool yes)
				{
					if (yes)
						JoinPartyPacket(partyid).dispatch();
					else
						DenyPartyInvitePacket(from_name).dispatch();
				}
			);

			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();

			break;
		}
		case 7:
		{
			// Silent update / log on-off
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			parse_party_data(recv, partyid);
			break;
		}
		case 8:
		{
			// Party created
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			// 4x int door data
			recv.read_int(); // townMap
			recv.read_int(); // areaMap
			recv.read_int(); // posX
			recv.read_int(); // posY

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "You have created a party.");

			break;
		}
		case 0x0C:
		{
			// Leave / Expel / Disband
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			int32_t target_cid = recv.read_int();
			bool disbanded = recv.read_byte() != 0;
			bool expelled = recv.read_byte() != 0;
			std::string target_name = recv.read_string();

			int32_t my_cid = Stage::get().get_player().get_oid();

			if (disbanded)
			{
				Stage::get().get_player().get_party().clear();

				if (messenger)
					messenger->show_status(Color::Name::WHITE, "The party has been disbanded.");
			}
			else if (expelled && target_cid == my_cid)
			{
				Stage::get().get_player().get_party().clear();

				if (messenger)
					messenger->show_status(Color::Name::RED, "You have been expelled from the party.");
			}
			else if (expelled)
			{
				if (recv.length() >= 4 * 6)
					parse_party_data(recv, partyid);

				if (messenger)
					messenger->show_status(Color::Name::WHITE, target_name + " has been expelled.");
			}
			else if (target_cid == my_cid)
			{
				Stage::get().get_player().get_party().clear();

				if (messenger)
					messenger->show_status(Color::Name::WHITE, "You have left the party.");
			}
			else
			{
				if (recv.length() >= 4 * 6)
					parse_party_data(recv, partyid);

				if (messenger)
					messenger->show_status(Color::Name::WHITE, target_name + " has left the party.");
			}

			break;
		}
		case 0x0F:
		{
			// Player joined
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			std::string new_member = recv.read_string();

			if (recv.length() >= 4 * 6)
				parse_party_data(recv, partyid);

			if (messenger)
				messenger->show_status(Color::Name::WHITE, new_member + " has joined the party.");

			break;
		}
		case 0x1B:
		{
			// Leader changed
			if (recv.length() < 4)
				break;

			int32_t new_leader_cid = recv.read_int();
			recv.read_byte(); // always 0

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The party leader has changed.");

			break;
		}
		case 0x23:
		{
			// Door/portal update
			if (recv.available())
				recv.read_short();
			break;
		}
		// Error/status codes
		case 10:
			if (messenger) messenger->show_status(Color::Name::RED, "A beginner can't create a party.");
			break;
		// Note: error code 12 (0x0C) is handled by the Leave/Expel/Disband case above
		case 13:
			if (messenger) messenger->show_status(Color::Name::RED, "You are not in a party.");
			break;
		case 16:
			if (messenger) messenger->show_status(Color::Name::RED, "You are already in a party.");
			break;
		case 17:
			if (messenger) messenger->show_status(Color::Name::RED, "The party is full.");
			break;
		case 19:
			if (messenger) messenger->show_status(Color::Name::RED, "Unable to find the player in this channel.");
			break;
		case 21:
			if (messenger) messenger->show_status(Color::Name::RED, "This player is blocking party invitations.");
			break;
		case 22:
		{
			std::string name = recv.available() ? recv.read_string() : "";
			if (messenger) messenger->show_status(Color::Name::RED, name + " is already handling another invitation.");
			break;
		}
		case 23:
		{
			std::string name = recv.available() ? recv.read_string() : "";
			if (messenger) messenger->show_status(Color::Name::WHITE, name + " has denied the invitation.");
			break;
		}
		case 25:
			if (messenger) messenger->show_status(Color::Name::RED, "You cannot kick in this map.");
			break;
		case 28:
		case 29:
			if (messenger) messenger->show_status(Color::Name::RED, "Leader change only to a nearby party member.");
			break;
		case 30:
			if (messenger) messenger->show_status(Color::Name::RED, "Leader change only on the same channel.");
			break;
		default:
			break;
		}
	}

	namespace
	{
		// Rank title names (Jr. Master, etc.) — 5 rank slots
		static const std::string default_ranks[] = { "Master", "Jr. Master", "Member", "Member", "Member" };

		std::string get_rank_name(int32_t rank)
		{
			if (rank >= 1 && rank <= 5)
				return default_ranks[rank - 1];

			return "Member";
		}

		void parse_guild_info(InPacket& recv)
		{
			// v83 full guild info block (Cosmic MapleGuildResponse.guildInfo)
			// int guild_id
			// string guild_name
			// 5x string rank_titles
			// member block: int count, then per member:
			//   int cid, short job, short level, int rank, int online, int signature, int alliance_rank
			// int capacity
			// short logo_bg, byte logo_bg_color, short logo, byte logo_color
			// string notice
			// int gp, int alliance_id

			if (recv.length() < 4)
				return;

			int32_t guild_id = recv.read_int();
			std::string guild_name = recv.read_string();

			// Read 5 rank titles
			std::string rank_titles[5];
			for (int i = 0; i < 5; i++)
				rank_titles[i] = recv.read_string();

			// Member count
			int8_t member_count = recv.read_byte();

			// Read member CIDs first (one int per member)
			std::vector<int32_t> member_cids;
			for (int i = 0; i < member_count; i++)
				member_cids.push_back(recv.read_int());

			// Now read member details (one block per member, in same order)
			struct GuildMemberData
			{
				int32_t cid;
				int16_t job;
				int16_t level;
				int32_t rank;
				int32_t online;
				int32_t signature;
				int32_t alliance_rank;
				std::string name;
			};

			std::vector<GuildMemberData> member_data;
			for (int i = 0; i < member_count; i++)
			{
				GuildMemberData m;
				m.cid = member_cids[i];
				// Each member block: padded(13) name, int job, int level, int rank, int online, int signature, int alliance_rank
				m.name = recv.read_padded_string(13);
				m.job = static_cast<int16_t>(recv.read_int());
				m.level = static_cast<int16_t>(recv.read_int());
				m.rank = recv.read_int();
				m.online = recv.read_int();
				m.signature = recv.read_int();
				m.alliance_rank = recv.read_int();

				member_data.push_back(m);
			}

			// Capacity
			int32_t capacity = recv.read_int();

			// Guild mark/logo
			int16_t logo_bg = recv.read_short();
			int8_t logo_bg_color = recv.read_byte();
			int16_t logo = recv.read_short();
			int8_t logo_color = recv.read_byte();

			// Notice
			std::string notice = recv.read_string();

			// GP and alliance
			int32_t gp = recv.read_int();
			int32_t alliance_id = recv.read_int();

			// Now populate the UI
			if (!UI::get().get_element<UIGuild>())
				UI::get().emplace<UIGuild>();

			if (auto guild = UI::get().get_element<UIGuild>())
			{
				guild->clear_members();

				// Calculate guild "level" from GP (approximate v83 formula)
				int16_t guild_level = 1;
				if (gp >= 15000) guild_level = 5;
				else if (gp >= 5000) guild_level = 4;
				else if (gp >= 2000) guild_level = 3;
				else if (gp >= 500) guild_level = 2;

				guild->set_guild_info(guild_name, notice, guild_level, static_cast<int16_t>(capacity));

				for (const auto& m : member_data)
				{
					std::string rank_name = (m.rank >= 1 && m.rank <= 5) ? rank_titles[m.rank - 1] : "Member";
					guild->add_member(m.name, rank_name, m.level, m.job, m.online != 0);
				}

				// Update player's own guild label
				Stage::get().get_player().set_guild(guild_name);
			}
		}
	}

	void GuildOperationHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (type)
		{
		case 0x05: // Invite received — show Yes/No popup
		{
			if (!Setting<AllowGuildInvite>::get().load())
				break;

			int32_t guild_id = recv.read_int();
			std::string inviter = recv.read_string();

			int32_t my_cid = Stage::get().get_player().get_oid();

			UI::get().emplace<UIYesNo>(
				inviter + " has invited you to their guild.",
				[guild_id, my_cid, inviter](bool yes)
				{
					if (yes)
						AcceptGuildInvitePacket(guild_id, my_cid).dispatch();
					else
						DenyGuildInvitePacket(inviter).dispatch();
				}
			);

			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();
			break;
		}
		case 0x1A: // Full guild info
		{
			if (!recv.available())
				break;

			int8_t in_guild = recv.read_byte();

			if (in_guild == 0)
			{
				// Not in a guild — clear UI
				if (auto guild = UI::get().get_element<UIGuild>())
				{
					guild->clear_members();
					guild->set_guild_info("No Guild", "", 1, 0);
				}

				Stage::get().get_player().set_guild("");
			}
			else
			{
				parse_guild_info(recv);
			}
			break;
		}
		case 0x27: // New member joined
		{
			int32_t guild_id = recv.read_int();
			// Member info follows — re-parse full guild for simplicity
			// In a full implementation, we'd just append the new member
			if (recv.length() >= 4)
				parse_guild_info(recv);

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "A new member has joined the guild.");
			break;
		}
		case 0x2C: // Member left
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			std::string name = recv.read_string();

			int32_t my_cid = Stage::get().get_player().get_oid();

			if (cid == my_cid)
			{
				if (auto guild = UI::get().get_element<UIGuild>())
				{
					guild->clear_members();
					guild->set_guild_info("No Guild", "", 1, 0);
					guild->deactivate();
				}
				Stage::get().get_player().set_guild("");

				if (messenger)
					messenger->show_status(Color::Name::WHITE, "You have left the guild.");
			}
			else
			{
				if (messenger)
					messenger->show_status(Color::Name::WHITE, name + " has left the guild.");
			}
			break;
		}
		case 0x2F: // Member expelled
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			std::string name = recv.read_string();

			int32_t my_cid = Stage::get().get_player().get_oid();

			if (cid == my_cid)
			{
				if (auto guild = UI::get().get_element<UIGuild>())
				{
					guild->clear_members();
					guild->set_guild_info("No Guild", "", 1, 0);
					guild->deactivate();
				}
				Stage::get().get_player().set_guild("");

				if (messenger)
					messenger->show_status(Color::Name::RED, "You have been expelled from the guild.");
			}
			else
			{
				if (messenger)
					messenger->show_status(Color::Name::RED, name + " has been expelled from the guild.");
			}
			break;
		}
		case 0x32: // Guild disbanded
		{
			int32_t guild_id = recv.read_int();
			recv.read_byte(); // always 1

			if (auto guild = UI::get().get_element<UIGuild>())
			{
				guild->clear_members();
				guild->set_guild_info("No Guild", "", 1, 0);
				guild->deactivate();
			}

			Stage::get().get_player().set_guild("");

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The guild has been disbanded.");
			break;
		}
		case 0x3A: // Capacity changed
		{
			int32_t guild_id = recv.read_int();
			int32_t new_capacity = recv.read_int();
			break;
		}
		case 0x3C: // Member level/job update
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			int32_t level = recv.read_int();
			int32_t job_id = recv.read_int();
			break;
		}
		case 0x3D: // Member online/offline
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			bool online = recv.read_byte() != 0;
			break;
		}
		case 0x3E: // Rank titles changed
		{
			int32_t guild_id = recv.read_int();
			for (int i = 0; i < 5; i++)
				recv.read_string();
			break;
		}
		case 0x40: // Member rank changed
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			int8_t new_rank = recv.read_byte();
			break;
		}
		case 0x42: // Emblem changed
		{
			int32_t guild_id = recv.read_int();
			recv.read_short(); // bg
			recv.read_byte();  // bgColor
			recv.read_short(); // logo
			recv.read_byte();  // logoColor
			break;
		}
		case 0x44: // Notice changed
		{
			int32_t guild_id = recv.read_int();
			std::string notice = recv.read_string();

			if (auto guild = UI::get().get_element<UIGuild>())
				guild->set_guild_info("", notice, 0, 0);

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The guild notice has been updated.");
			break;
		}
		case 0x48: // GP updated
		{
			int32_t guild_id = recv.read_int();
			int32_t gp = recv.read_int();
			break;
		}
		// Error codes
		case 0x28:
			if (messenger) messenger->show_status(Color::Name::RED, "The player is already in a guild.");
			break;
		case 0x2A:
			if (messenger) messenger->show_status(Color::Name::RED, "The player is not in this channel.");
			break;
		case 0x2D:
			if (messenger) messenger->show_status(Color::Name::RED, "You are not in a guild.");
			break;
		case 0x2E:
			if (messenger) messenger->show_status(Color::Name::RED, "Guild invite not found.");
			break;
		case 0x36:
		{
			std::string name = recv.available() ? recv.read_string() : "";
			if (messenger) messenger->show_status(Color::Name::RED, name + " is already handling another invitation.");
			break;
		}
		case 0x37:
		{
			std::string name = recv.available() ? recv.read_string() : "";
			if (messenger) messenger->show_status(Color::Name::WHITE, name + " has denied the invitation.");
			break;
		}
		default:
			break;
		}
	}

	void GuildBBSHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();

		if (!UI::get().get_element<UIGuildBBS>())
			UI::get().emplace<UIGuildBBS>();

		auto bbs = UI::get().get_element<UIGuildBBS>();
		if (!bbs) return;

		if (type == 0x06) // Post list
		{
			bbs->clear_posts();

			if (!recv.available()) return;

			int8_t count = recv.read_byte();

			for (int8_t i = 0; i < count && recv.available(); i++)
			{
				int32_t post_id = recv.read_int();
				std::string author = recv.read_string();
				std::string title = recv.read_string();
				int64_t timestamp = recv.read_long();
				int32_t reply_count = recv.read_int();

				bbs->add_post(post_id, author, title, timestamp, reply_count);
			}
		}
		else
		{
			bbs->makeactive();
		}
	}

	void AllianceOperationHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t op = recv.read_byte();

		switch (op)
		{
		case 0x03: // Alliance invite
		{
			if (!Setting<AllowAllianceInvite>::get().load())
				break;

			if (recv.available())
			{
				int32_t alliance_id = recv.read_int();
				std::string inviter = recv.read_string();
				recv.skip(2); // padding short

				UI::get().emplace<UIYesNo>(
					inviter + " has invited your guild to join their alliance.",
					[alliance_id](bool yes)
					{
						if (yes)
							AllianceAcceptInvitePacket(alliance_id).dispatch();
					}
				);

				if (auto statusbar = UI::get().get_element<UIStatusBar>())
					statusbar->notify();
			}
			break;
		}
		case 0x0C: // getAllianceInfo — full alliance data
		{
			if (!recv.available())
				break;

			int8_t exists = recv.read_byte();

			if (exists != 1)
				break;

			int32_t alliance_id = recv.read_int();
			std::string alliance_name = recv.read_string();

			// 5 rank titles
			std::string rank_titles[5];
			for (int i = 0; i < 5; i++)
				rank_titles[i] = recv.read_string();

			int8_t guild_count = recv.read_byte();
			int32_t capacity = recv.read_int();

			std::vector<int32_t> guild_ids;
			for (int8_t i = 0; i < guild_count; i++)
				guild_ids.push_back(recv.read_int());

			std::string notice = recv.read_string();

			UI::get().emplace<UIAlliance>();

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->set_alliance_info(alliance_name, 0, capacity);
				alliance->set_notice(notice);
				alliance->clear_guilds();

				for (auto gid : guild_ids)
					alliance->add_guild("Guild " + std::to_string(gid), gid);
			}
			break;
		}
		case 0x0D: // getGuildAlliances — full guild info blocks
		{
			if (!recv.available())
				break;

			int32_t guild_count = recv.read_int();

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->clear_guilds();

				for (int32_t i = 0; i < guild_count; i++)
				{
					if (!recv.available())
						break;

					int32_t guild_id = recv.read_int();
					std::string guild_name = recv.read_string();

					// Skip guild rank titles (5)
					for (int r = 0; r < 5; r++)
						recv.read_string();

					// Skip member data
					int8_t member_count = recv.read_byte();

					// Member character IDs
					for (int8_t m = 0; m < member_count; m++)
						recv.read_int();

					// Member details: name(13 padded) + job(4) + level(4) + guildRank(4) + online(4) + sig(4) + allianceRank(4)
					for (int8_t m = 0; m < member_count; m++)
					{
						recv.skip(13); // padded name
						recv.skip(4 * 6); // job, level, guildRank, online, sig, allianceRank
					}

					recv.read_int();    // capacity
					recv.read_short();  // logoBG
					recv.read_byte();   // logoBGColor
					recv.read_short();  // logo
					recv.read_byte();   // logoColor
					recv.read_string(); // guild notice
					recv.read_int();    // GP
					recv.read_int();    // allianceId

					alliance->add_guild(guild_name, guild_id);
				}
			}
			break;
		}
		case 0x0E: // allianceMemberOnline
		{
			int32_t alliance_id = recv.read_int();
			int32_t guild_id = recv.read_int();
			int32_t character_id = recv.read_int();
			bool online = recv.read_bool();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
			{
				std::string status = online ? "logged in" : "logged out";
				chatbar->send_chatline("[Alliance] A member has " + status + ".", UIChatBar::LineType::YELLOW);
			}
			break;
		}
		case 0x0F: // updateAllianceInfo — refresh all info
		{
			if (!recv.available())
				break;

			int32_t alliance_id = recv.read_int();
			std::string alliance_name = recv.read_string();

			// 5 rank titles
			for (int i = 0; i < 5; i++)
				recv.read_string();

			int8_t guild_count = recv.read_byte();

			std::vector<int32_t> guild_ids;
			for (int8_t i = 0; i < guild_count; i++)
				guild_ids.push_back(recv.read_int());

			int32_t capacity = recv.read_int();
			recv.skip(2); // padding short

			// Skip guild info blocks
			for (int8_t i = 0; i < guild_count; i++)
			{
				if (!recv.available())
					break;

				recv.read_int();    // guildId
				recv.read_string(); // guildName

				for (int r = 0; r < 5; r++)
					recv.read_string(); // rank titles

				int8_t member_count = recv.read_byte();
				for (int8_t m = 0; m < member_count; m++)
					recv.read_int(); // charIds

				for (int8_t m = 0; m < member_count; m++)
				{
					recv.skip(13);
					recv.skip(4 * 6);
				}

				recv.read_int();
				recv.read_short();
				recv.read_byte();
				recv.read_short();
				recv.read_byte();
				recv.read_string();
				recv.read_int();
				recv.read_int();
			}

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->set_alliance_info(alliance_name, 0, static_cast<int8_t>(capacity));
				alliance->clear_guilds();

				for (auto gid : guild_ids)
					alliance->add_guild("Guild " + std::to_string(gid), gid);
			}
			break;
		}
		case 0x10: // removeGuildFromAlliance
		{
			// Re-sends full alliance header + expelled guild info
			int32_t alliance_id = recv.read_int();
			std::string alliance_name = recv.read_string();

			for (int i = 0; i < 5; i++)
				recv.read_string(); // rank titles

			int8_t guild_count = recv.read_byte();
			std::vector<int32_t> guild_ids;
			for (int8_t i = 0; i < guild_count; i++)
				guild_ids.push_back(recv.read_int());

			int32_t capacity = recv.read_int();
			std::string notice = recv.read_string();

			// Expelled guild id + guild info block + trailing byte
			int32_t expelled_guild_id = recv.read_int();
			// Consume remaining data
			while (recv.available())
				recv.read_byte();

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->set_alliance_info(alliance_name, 0, static_cast<int8_t>(capacity));
				alliance->set_notice(notice);
				alliance->clear_guilds();

				for (auto gid : guild_ids)
				{
					if (gid != expelled_guild_id)
						alliance->add_guild("Guild " + std::to_string(gid), gid);
				}
			}

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] A guild has been removed from the alliance.", UIChatBar::LineType::YELLOW);

			break;
		}
		case 0x12: // addGuildToAlliance
		{
			int32_t alliance_id = recv.read_int();
			std::string alliance_name = recv.read_string();

			for (int i = 0; i < 5; i++)
				recv.read_string();

			int8_t guild_count = recv.read_byte();
			std::vector<int32_t> guild_ids;
			for (int8_t i = 0; i < guild_count; i++)
				guild_ids.push_back(recv.read_int());

			int32_t capacity = recv.read_int();
			std::string notice = recv.read_string();

			// New guild id + guild info block
			int32_t new_guild_id = recv.read_int();
			std::string new_guild_name = "Guild " + std::to_string(new_guild_id);

			// Try to read guild name from the info block
			if (recv.available())
			{
				recv.read_int(); // guildId (same as new_guild_id)
				new_guild_name = recv.read_string();
				// Consume remaining
				while (recv.available())
					recv.read_byte();
			}

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->set_alliance_info(alliance_name, 0, static_cast<int8_t>(capacity));
				alliance->set_notice(notice);
				alliance->clear_guilds();

				for (auto gid : guild_ids)
				{
					if (gid == new_guild_id)
						alliance->add_guild(new_guild_name, gid);
					else
						alliance->add_guild("Guild " + std::to_string(gid), gid);
				}
			}

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] A new guild has joined the alliance.", UIChatBar::LineType::YELLOW);

			break;
		}
		case 0x18: // updateAllianceJobLevel — member job/level changed
		{
			recv.read_int(); // allianceId
			recv.read_int(); // guildId
			recv.read_int(); // characterId
			recv.read_int(); // level
			recv.read_int(); // jobId
			break;
		}
		case 0x1A: // changeAllianceRankTitle
		{
			recv.read_int(); // allianceId
			for (int i = 0; i < 5; i++)
				recv.read_string(); // rank titles

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Rank titles have been updated.", UIChatBar::LineType::YELLOW);

			break;
		}
		case 0x1C: // allianceNotice
		{
			recv.read_int(); // allianceId
			std::string notice = recv.read_string();

			if (auto alliance = UI::get().get_element<UIAlliance>())
				alliance->set_notice(notice);

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Notice updated.", UIChatBar::LineType::YELLOW);

			break;
		}
		case 0x1D: // disbandAlliance
		{
			recv.read_int(); // allianceId

			if (auto alliance = UI::get().get_element<UIAlliance>())
			{
				alliance->set_alliance_info("", 0, 0);
				alliance->clear_guilds();
				alliance->set_notice("");
			}

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] The alliance has been disbanded.", UIChatBar::LineType::RED);

			break;
		}
		default:
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Operation (code: " + std::to_string(op) + ")", UIChatBar::LineType::YELLOW);
			break;
		}
		}
	}

	void MessengerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x00: // addMessengerPlayer — player joined the messenger
		{
			int8_t slot = recv.read_byte();
			// Parse CharLook data
			LoginParser::parse_look(recv);
			std::string name = recv.read_string();
			recv.read_byte(); // channel
			recv.read_byte(); // 0

			if (auto msger = UI::get().get_element<UIMessenger>())
				msger->add_player(slot, name);

			break;
		}
		case 0x01: // joinMessenger — position assignment
		{
			int8_t slot = recv.read_byte();
			break;
		}
		case 0x02: // removeMessengerPlayer — leave
		{
			int8_t slot = recv.read_byte();

			if (auto msger = UI::get().get_element<UIMessenger>())
				msger->remove_player(slot);

			break;
		}
		case 0x03: // messengerInvite — invite received
		{
			if (!Setting<AllowChatInvite>::get().load())
				break;

			std::string from = recv.read_string();
			recv.read_byte(); // 0
			int32_t messenger_id = recv.read_int();
			recv.read_byte(); // 0

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(from + " has invited you to a Messenger conversation.", UIChatBar::LineType::YELLOW);

			if (auto statusbar = UI::get().get_element<UIStatusBar>())
				statusbar->notify();

			break;
		}
		case 0x05: // messengerNote — decline/note
		{
			std::string text = recv.read_string();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(text, UIChatBar::LineType::RED);

			break;
		}
		case 0x06: // messengerChat — chat message
		{
			std::string text = recv.read_string();

			if (auto msger = UI::get().get_element<UIMessenger>())
			{
				size_t sep = text.find(" : ");

				if (sep != std::string::npos)
					msger->add_chat(text.substr(0, sep), text.substr(sep + 3));
				else
					msger->add_chat("", text);
			}

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Messenger] " + text, UIChatBar::LineType::BLUE);

			break;
		}
		default:
			break;
		}
	}

	void NotifyLevelUpHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte(); // 0=family, 2=guild
		int32_t level = recv.read_int();
		std::string name = recv.read_string();

		std::string prefix = (type == 2) ? "[Guild] " : "[Family] ";

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(prefix + name + " has reached level " + std::to_string(level) + "!", UIChatBar::LineType::YELLOW);
	}

	void NotifyJobChangeHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte(); // 0=guild, 1=family
		int32_t job = recv.read_int();
		std::string name = recv.read_string();

		std::string prefix = (type == 0) ? "[Guild] " : "[Family] ";

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(prefix + name + " has made a job advancement!", UIChatBar::LineType::YELLOW);
	}

	void WeddingProgressHandler::handle(InPacket& recv) const
	{
		int8_t step = recv.read_byte();
		int32_t groom_id = recv.read_int();
		int32_t bride_id = recv.read_int();

		if (auto wedding = UI::get().get_element<UIWedding>())
			wedding->set_countdown(step);
	}

	void WeddingCeremonyEndHandler::handle(InPacket& recv) const
	{
		int32_t groom_id = recv.read_int();
		int32_t bride_id = recv.read_int();

		if (auto wedding = UI::get().get_element<UIWedding>())
			wedding->set_countdown(0);

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Wedding] The ceremony has ended!", UIChatBar::LineType::YELLOW);
	}

	void MarriageRequestHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 9) // Wishlist
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Marriage] Wishlist prompt received.", UIChatBar::LineType::YELLOW);
		}
		else // mode 0=engage, 1=cancel, 2=answer
		{
			std::string name = recv.read_string();
			int32_t player_id = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Marriage] " + name + " has sent you a proposal!", UIChatBar::LineType::YELLOW);
		}
	}

	void MarriageResultHandler::handle(InPacket& recv) const
	{
		int8_t msg = recv.read_byte();

		std::string text;
		switch (msg)
		{
		case 11: text = "Marriage ceremony completed!"; break;
		case 15: text = "You have received a wedding invitation!"; break;
		case 36: text = "You are now engaged!"; break;
		default: text = "Marriage update (code: " + std::to_string(msg) + ")"; break;
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Marriage] " + text, UIChatBar::LineType::YELLOW);
	}

	void WeddingGiftResultHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
		{
			if (mode == 0xA)
				chatbar->send_chatline("[Wedding] Gift sent successfully!", UIChatBar::LineType::YELLOW);
			else
				chatbar->send_chatline("[Wedding] Gift result: " + std::to_string(mode), UIChatBar::LineType::YELLOW);
		}
	}

	void NotifyMarriedPartnerHandler::handle(InPacket& recv) const
	{
		int32_t map_id = recv.read_int();
		int32_t partner_id = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Marriage] Your partner has moved to another map.", UIChatBar::LineType::YELLOW);
	}

	void FamilyChartResultHandler::handle(InPacket& recv) const
	{
		int32_t viewed_id = recv.read_int();
		int32_t entry_count = recv.read_int();

		if (auto family = UI::get().get_element<UIFamily>())
		{
			for (int32_t i = 0; i < entry_count && recv.available(); i++)
			{
				int32_t cid = recv.read_int();
				int32_t parent_id = recv.read_int();
				recv.read_short();  // job id
				recv.read_byte();   // level
				recv.read_bool();   // isOnline
				recv.read_int();    // current rep
				recv.read_int();    // total rep
				recv.read_int();    // reps to senior
				recv.read_int();    // todays rep
				recv.read_int();    // channel
				recv.read_int();    // time online (minutes)
				std::string name = recv.read_string();

				if (cid != viewed_id)
					family->add_junior(name);
			}
		}

		// Post-entry data: member info counts
		if (recv.available())
		{
			int32_t member_info_count = recv.read_int();
			for (int32_t i = 0; i < member_info_count && recv.available(); i++)
			{
				recv.read_int(); // id or flag
				recv.read_int(); // count
			}
			// Entitlements used loop
			if (recv.available())
			{
				int32_t entitlement_count = recv.read_int();
				for (int32_t i = 0; i < entitlement_count && recv.available(); i++)
				{
					recv.read_int(); // entitlement index
					recv.read_int(); // times used
				}
			}
			if (recv.available())
				recv.read_short(); // add button flag
		}
	}

	void FamilyInfoResultHandler::handle(InPacket& recv) const
	{
		int32_t current_rep = recv.read_int();
		int32_t total_rep = recv.read_int();
		int32_t todays_rep = recv.read_int();
		int16_t junior_count = recv.read_short();
		recv.read_short(); // juniors allowed
		recv.read_short(); // unknown
		int32_t leader_id = recv.read_int();
		std::string family_name = recv.read_string();
		std::string message = recv.read_string();

		// Entitlement data
		if (recv.available())
		{
			int32_t entitlement_count = recv.read_int();
			for (int32_t i = 0; i < entitlement_count && recv.available(); i++)
			{
				recv.read_int(); // entitlement ordinal
				recv.read_int(); // used count
			}
		}

		if (auto family = UI::get().get_element<UIFamily>())
			family->set_family_info(family_name, current_rep, total_rep);
	}

	void FamilyResultHandler::handle(InPacket& recv) const
	{
		int32_t type = recv.read_int();
		int32_t mesos = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] Result type " + std::to_string(type) + " (mesos: " + std::to_string(mesos) + ")", UIChatBar::LineType::YELLOW);
	}

	void FamilyJoinRequestHandler::handle(InPacket& recv) const
	{
		int32_t player_id = recv.read_int();
		std::string name = recv.read_string();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] " + name + " wants to join your family!", UIChatBar::LineType::YELLOW);
	}

	void FamilyJoinRequestResultHandler::handle(InPacket& recv) const
	{
		bool accepted = recv.read_byte() != 0;
		std::string name = recv.read_string();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
		{
			if (accepted)
				chatbar->send_chatline("[Family] " + name + " accepted your request.", UIChatBar::LineType::YELLOW);
			else
				chatbar->send_chatline("[Family] " + name + " declined your request.", UIChatBar::LineType::RED);
		}
	}

	void FamilyJoinAcceptedHandler::handle(InPacket& recv) const
	{
		std::string name = recv.read_string();
		recv.read_int(); // 0

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] " + name + " has joined your family!", UIChatBar::LineType::YELLOW);
	}

	void FamilyRepGainHandler::handle(InPacket& recv) const
	{
		int32_t gain = recv.read_int();
		std::string from = recv.read_string();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] +" + std::to_string(gain) + " rep from " + from, UIChatBar::LineType::YELLOW);
	}

	void FamilyLoginLogoutHandler::handle(InPacket& recv) const
	{
		bool logged_in = recv.read_bool();
		std::string name = recv.read_string();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
		{
			std::string msg = logged_in ? " has logged in." : " has logged out.";
			chatbar->send_chatline("[Family] " + name + msg, UIChatBar::LineType::YELLOW);
		}
	}

	void FamilySummonRequestHandler::handle(InPacket& recv) const
	{
		std::string from = recv.read_string();
		std::string family_name = recv.read_string();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] " + from + " is summoning you!", UIChatBar::LineType::YELLOW);
	}

	void FamilyPrivilegeListHandler::handle(InPacket& recv) const
	{
		// Family privilege list — read and consume all entries
		if (!recv.available())
			return;

		int32_t count = recv.read_int();

		for (int32_t i = 0; i < count && recv.available(); i++)
		{
			recv.read_byte();   // type (1 or 2)
			recv.read_int();    // reputation cost
			recv.read_int();    // usage limit
			recv.read_string(); // privilege name
			recv.read_string(); // privilege description
		}
	}

	void RPSGameHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 8) // Open NPC
		{
			int32_t npc_id = recv.read_int();
			UI::get().emplace<UIRPSGame>();
		}
		else if (mode == 0x0B) // Selection result
		{
			int8_t selection = recv.read_byte();
			int8_t answer = recv.read_byte();

			// Determine result: 0=win, 1=lose, 2=tie
			int8_t result;
			if (selection == answer)
				result = 2; // tie
			else if ((selection == 0 && answer == 2) || (selection == 1 && answer == 0) || (selection == 2 && answer == 1))
				result = 0; // win
			else
				result = 1; // lose

			if (auto rps = UI::get().get_element<UIRPSGame>())
				rps->show_result(selection, answer, result);
		}
		else if (mode == 0x06) // Meso error
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "Not enough mesos for Rock-Paper-Scissors.");
		}
		else if (mode == 0x0D) // Game over / exit
		{
			UI::get().remove(UIElement::Type::RPSGAME);
		}
	}
}