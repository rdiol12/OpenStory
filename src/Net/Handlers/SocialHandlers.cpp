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
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIGuild.h"
#include "../../IO/UITypes/UIGuildBBS.h"
#include "../../IO/UITypes/UIMessenger.h"
#include "../../IO/UITypes/UIFamily.h"
#include "../../IO/UITypes/UIAlliance.h"
#include "../../IO/UITypes/UIWedding.h"
#include "../../IO/UITypes/UIRPSGame.h"
#include "Helpers/LoginParser.h"

namespace ms
{
	void BuddyListHandler::handle(InPacket& recv) const
	{
		// v83: byte operation
		// 7 = buddy list update
		// 10 = buddy channel change
		// 14 = buddy capacity
		// 18 = buddy added/online
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();

		switch (operation)
		{
		case 7:
		{
			// Buddy list update
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
				entry.group = recv.read_padded_string(17);

				entries[entry.cid] = entry;
			}

			Stage::get().get_player().get_buddylist().update(entries);
			break;
		}
		case 10:
		{
			// Buddy channel change — same format as op 7
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
				entry.group = recv.read_padded_string(17);
				entries[entry.cid] = entry;
			}

			Stage::get().get_player().get_buddylist().update(entries);
			break;
		}
		case 14:
		{
			// Buddy capacity
			if (recv.available())
			{
				int8_t capacity = recv.read_byte();
				Stage::get().get_player().get_buddylist().set_capacity(capacity);
			}
			break;
		}
		case 18:
		{
			// Buddy added / online notification — same format as op 7
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
				entry.group = recv.read_padded_string(17);
				entries[entry.cid] = entry;
			}

			Stage::get().get_player().get_buddylist().update(entries);

			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::WHITE, "Your buddy list has been updated.");

			break;
		}
		default:
			break;
		}
	}

	void FamilyHandler::handle(InPacket& recv) const
	{
		// v83 family packet - show family-related messages
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();
		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (mode)
		{
		case 0:
		{
			// Family info update - list of family members
			// int count, then for each: int cid, string name, short job, short level, int rep
			if (recv.length() < 4)
				break;

			int32_t count = recv.read_int();

			for (int32_t i = 0; i < count && recv.available(); i++)
			{
				recv.read_int();    // cid
				recv.read_string(); // name
				recv.read_short();  // job
				recv.read_short();  // level
				recv.read_int();    // reputation
			}
			break;
		}
		case 2:
		{
			// Family invitation
			if (recv.available())
			{
				std::string from_name = recv.read_string();

				if (messenger)
					messenger->show_status(Color::Name::YELLOW, from_name + " has invited you to join their family.");
			}
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
		// v83 party operations:
		// 7  = silent party update (full party data)
		// 11 = party created / you joined (full party data)
		// 12 = invite received
		// 13 = expel (int partyid, int target_cid, byte new_leader, then full party data if still in)
		// 14 = member left (int partyid, int target_cid, byte new_leader, then full party data if still in)
		// 15 = disband (int partyid, int leader_cid)
		// 22 = leader changed
		// 25 = member online/channel status changed (full party data)
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();
		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (operation)
		{
		case 7:
		case 11:
		case 25:
		{
			// Full party data update
			// Op 7 = silent refresh, 11 = joined/created, 25 = member status change
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			parse_party_data(recv, partyid);

			if (operation == 11 && messenger)
				messenger->show_status(Color::Name::WHITE, "You have joined the party.");

			break;
		}
		case 12:
		{
			// Invite received
			if (recv.length() < 4)
				break;

			int32_t from_cid = recv.read_int();
			std::string from_name = recv.read_string();

			if (messenger)
				messenger->show_status(Color::Name::YELLOW, from_name + " has invited you to a party.");

			break;
		}
		case 13:
		{
			// Member expelled
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			int32_t target_cid = recv.read_int();

			int32_t my_cid = Stage::get().get_player().get_oid();

			if (target_cid == my_cid)
			{
				Stage::get().get_player().get_party().clear();

				if (messenger)
					messenger->show_status(Color::Name::RED, "You have been expelled from the party.");
			}
			else
			{
				// Skip byte (leader_changed flag) and re-parse party data
				if (recv.available())
					recv.read_byte();

				if (recv.length() >= 4 * 6)
					parse_party_data(recv, partyid);
			}

			break;
		}
		case 14:
		{
			// Member left
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			int32_t target_cid = recv.read_int();

			int32_t my_cid = Stage::get().get_player().get_oid();

			if (target_cid == my_cid)
			{
				Stage::get().get_player().get_party().clear();

				if (messenger)
					messenger->show_status(Color::Name::WHITE, "You have left the party.");
			}
			else
			{
				if (recv.available())
					recv.read_byte();

				if (recv.length() >= 4 * 6)
					parse_party_data(recv, partyid);
			}

			break;
		}
		case 15:
		{
			// Party disbanded
			if (recv.length() < 8)
				break;

			recv.read_int(); // partyid
			recv.read_int(); // leader_cid

			Stage::get().get_player().get_party().clear();

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The party has been disbanded.");

			break;
		}
		case 22:
		{
			// Leader changed
			if (recv.length() < 4)
				break;

			int32_t new_leader_cid = recv.read_int();

			Party& party = Stage::get().get_player().get_party();
			int32_t partyid = party.get_id();

			// Re-parse with same partyid if full data follows
			if (recv.length() >= 4 * 6)
			{
				parse_party_data(recv, partyid);
			}

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The party leader has changed.");

			break;
		}
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
		case 0x26: // Full guild info (show guild window)
		{
			parse_guild_info(recv);
			break;
		}
		case 0x27: // Guild update (member join)
		{
			parse_guild_info(recv);

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "A new member has joined the guild.");
			break;
		}
		case 0x28: // Member left
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			std::string name = recv.read_string();

			if (messenger)
				messenger->show_status(Color::Name::WHITE, name + " has left the guild.");
			break;
		}
		case 0x29: // Member expelled
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			std::string name = recv.read_string();

			if (messenger)
				messenger->show_status(Color::Name::RED, name + " has been expelled from the guild.");
			break;
		}
		case 0x2A: // Guild disbanded
		{
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
		case 0x2C: // Guild invitation
		{
			int32_t guild_id = recv.read_int();
			std::string inviter = recv.read_string();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guild] " + inviter + " has invited you to join their guild.", UIChatBar::LineType::YELLOW);
			break;
		}
		case 0x32: // Rank title change
		{
			// Re-parse full guild data
			parse_guild_info(recv);
			break;
		}
		case 0x3C: // Member online/offline status change
		{
			int32_t guild_id = recv.read_int();
			int32_t cid = recv.read_int();
			bool online = recv.read_byte() != 0;
			// Full guild re-request would be needed for UI update
			break;
		}
		case 0x3E: // Guild notice changed
		{
			std::string new_notice = recv.read_string();

			if (auto guild = UI::get().get_element<UIGuild>())
				guild->set_guild_info("", new_notice, 0, 0);

			if (messenger)
				messenger->show_status(Color::Name::WHITE, "The guild notice has been updated.");
			break;
		}
		case 0x44: // Guild GP changed
		{
			int32_t guild_id = recv.read_int();
			int32_t gp = recv.read_int();
			break;
		}
		case 0x48: // Guild level up
		{
			int32_t guild_id = recv.read_int();
			int32_t new_level = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guild] The guild has leveled up to Lv." + std::to_string(new_level) + "!", UIChatBar::LineType::YELLOW);
			break;
		}
		default:
			// Unhandled guild operation type
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
		int8_t op = recv.read_byte();

		switch (op)
		{
		case 0x0C: // Alliance info
		{
			if (recv.length() >= 4)
			{
				int8_t rank = recv.read_byte();
				int8_t guild_count = recv.read_byte();

				UI::get().emplace<UIAlliance>();

				if (auto alliance = UI::get().get_element<UIAlliance>())
				{
					alliance->clear_guilds();
					for (int8_t i = 0; i < guild_count; i++)
					{
						if (recv.available())
						{
							int32_t guild_id = recv.read_int();
							alliance->add_guild("Guild " + std::to_string(guild_id), guild_id);
						}
					}
				}
			}
			break;
		}
		case 0x0D: // Alliance notice
		{
			if (recv.available())
			{
				std::string notice = recv.read_string();
				if (auto alliance = UI::get().get_element<UIAlliance>())
					alliance->set_notice(notice);
			}
			break;
		}
		default:
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Alliance] Operation update (code: " + std::to_string(op) + ")", UIChatBar::LineType::YELLOW);
			break;
		}
		}
	}

	void MessengerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0x00: // Add player
		{
			int8_t slot = recv.read_byte();
			// Parse CharLook data (skip it - we don't render avatars in messenger)
			LoginParser::parse_look(recv);
			std::string name = recv.read_string();
			recv.read_byte(); // channel
			recv.read_byte(); // whisper flag

			if (auto msger = UI::get().get_element<UIMessenger>())
				msger->add_player(slot, name);
			break;
		}
		case 0x01: // Join / self position
		{
			int8_t slot = recv.read_byte();
			UI::get().emplace<UIMessenger>();

			if (auto msger = UI::get().get_element<UIMessenger>())
				msger->add_player(slot, Stage::get().get_player().get_name());
			break;
		}
		case 0x02: // Remove player
		{
			int8_t slot = recv.read_byte();

			if (auto msger = UI::get().get_element<UIMessenger>())
				msger->remove_player(slot);
			break;
		}
		case 0x03: // Invite
		{
			std::string from = recv.read_string();
			recv.read_byte(); // 0
			int32_t messenger_id = recv.read_int();
			recv.read_byte(); // 0

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(from + " has invited you to a Messenger conversation.", UIChatBar::LineType::YELLOW);
			break;
		}
		case 0x06: // Chat
		{
			std::string text = recv.read_string();

			if (auto msger = UI::get().get_element<UIMessenger>())
			{
				// Text format is "name : message"
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
			// Parse entries - each has: cid, parent_id, name, job info
			for (int32_t i = 0; i < entry_count && recv.available(); i++)
			{
				int32_t cid = recv.read_int();
				int32_t parent_id = recv.read_int();
				std::string name = recv.read_string();

				// Add as junior if not the viewed player
				if (cid != viewed_id)
					family->add_junior(name);
			}
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Family] Family chart loaded with " + std::to_string(entry_count) + " members.", UIChatBar::LineType::YELLOW);
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