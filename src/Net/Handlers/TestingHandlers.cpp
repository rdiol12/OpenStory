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
#include "TestingHandlers.h"

#include "../../Gameplay/Stage.h"
#include "../../Gameplay/MapleMap/Npc.h"
#include "../../IO/UI.h"

#include "../../Character/BuddyList.h"
#include "../../Character/OtherChar.h"
#include "../../Character/Party.h"
#include "../../IO/UITypes/UIClock.h"
#include "../../IO/UITypes/UILoginNotice.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIQuestLog.h"
#include "../../IO/UITypes/UIStorage.h"
#include "../../IO/UITypes/UITrade.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UIGuild.h"
#include "../../IO/UITypes/UIGuildBBS.h"
#include "../../IO/UITypes/UIMessenger.h"
#include "../../IO/UITypes/UIFamily.h"
#include "../../IO/UITypes/UIMonsterCarnival.h"
#include "../../IO/UITypes/UIMinigame.h"
#include "../../IO/UITypes/UIRPSGame.h"
#include "../../IO/UITypes/UIHiredMerchant.h"
#include "../../IO/UITypes/UIPersonalShop.h"
#include "../../IO/UITypes/UIWedding.h"
#include "../../IO/UITypes/UIPartySearch.h"
#include "../../IO/UITypes/UIAlliance.h"
#include "../../Net/Packets/TradePackets.h"
#include "../../IO/UITypes/UIWorldSelect.h"
#include "../../Net/Session.h"
#include "../../Configuration.h"
#include "Helpers/LoginParser.h"

#include <iomanip>
#include <sstream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void CheckSpwResultHandler::handle(InPacket& recv) const
	{
		int reason = recv.read_byte();

		if (reason == 0)
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::INCORRECT_PIC);
		else
			std::cout << "[CheckSpwResultHandler]: Unknown reason: [" << reason << "]" << std::endl;

		UI::get().enable();
	}

	void FieldEffectHandler::handle(InPacket& recv) const
	{
		int rand = recv.read_byte();

		// Effect
		if (rand == 3)
		{
			std::string path = recv.read_string();

			Stage::get().add_effect(path);

			return;
		}

		std::cout << "[FieldEffectHandler]: Unknown value: [" << rand << "]" << std::endl;
	}

	void UpdateQuestInfoHandler::handle(InPacket& recv) const
	{
		// Cosmic uses UPDATE_QUEST_INFO (0xD3) for NPC info, time limits, expiration
		// Quest state changes go through SHOW_STATUS_INFO (0x27) mode 1
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 6:
		{
			// Add quest time limit
			int16_t count = recv.read_short();
			for (int16_t i = 0; i < count; i++)
			{
				int16_t questid = recv.read_short();
				int32_t time_ms = recv.read_int();

				// Set countdown for the first (or most recent) quest timer
				Stage::get().set_countdown(time_ms / 1000);

				if (auto chatbar = UI::get().get_element<UIChatBar>())
					chatbar->send_chatline("[Quest] Time limit set: " + std::to_string(time_ms / 1000) + " seconds", UIChatBar::LineType::YELLOW);
			}
			break;
		}
		case 7:
		{
			// Remove quest time limit
			int16_t pos = recv.read_short();
			int16_t questid = recv.read_short();
			std::cout << "[UpdateQuestInfo] Remove time limit: quest " << questid << std::endl;
			break;
		}
		case 8:
		{
			// Quest NPC info (sent after quest start/complete)
			int16_t questid = recv.read_short();
			int32_t npcid = recv.read_int();
			int32_t zero = recv.read_int();
			std::cout << "[UpdateQuestInfo] NPC info: quest " << questid << " npc " << npcid << std::endl;

			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			break;
		}
		case 0x0F:
		{
			// Quest expired
			int16_t questid = recv.read_short();
			Stage::get().get_player().get_quests().forfeit(questid);
			std::cout << "[UpdateQuestInfo] Quest expired: " << questid << std::endl;

			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			// Refresh NPC quest marks
			Stage::get().get_npcs().refresh_quest_marks();

			break;
		}
		default:
			std::cout << "[UpdateQuestInfoHandler]: Unhandled mode: [" << (int)mode << "] remaining bytes: [" << recv.length() << "]" << std::endl;
			break;
		}
	}

	void QuestClearHandler::handle(InPacket& recv) const
	{
		// Cosmic sends: SHORT questId — visual quest completion notification
		if (recv.length() < 2)
			return;

		int16_t questid = recv.read_short();
		std::cout << "[QuestClear] Quest " << questid << " completed (visual)" << std::endl;

		// Refresh quest log UI if open
		if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
			questlog_ui->load_quests();
	}

	void RelogResponseHandler::handle(InPacket& recv) const
	{
		// v83: byte success (1 = ok to relog back to login screen)
		if (!recv.available())
			return;

		int8_t success = recv.read_byte();

		if (success == 1)
		{
			// Reconnect to the login server
			Session::get().reconnect();
		}
	}

	void FameResponseHandler::handle(InPacket& recv) const
	{
		// v83: byte mode
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (mode)
		{
		case 0:
		{
			// Successfully gave fame
			if (recv.available())
			{
				std::string charname = recv.read_string();
				int8_t type = recv.read_byte();
				int32_t new_fame = recv.available() ? recv.read_int() : 0;

				std::string msg = (type == 1)
					? "You have raised " + charname + "'s fame to " + std::to_string(new_fame) + "."
					: "You have lowered " + charname + "'s fame to " + std::to_string(new_fame) + ".";

				if (messenger)
					messenger->show_status(Color::Name::WHITE, msg);
			}
			break;
		}
		case 1:
		{
			// Error: already famed today
			if (messenger)
				messenger->show_status(Color::Name::RED, "You have already given fame to this character today.");
			break;
		}
		case 2:
		{
			// Error: level too low
			if (messenger)
				messenger->show_status(Color::Name::RED, "You must be at least level 15 to give fame.");
			break;
		}
		case 3:
		{
			// Error: can't fame yourself
			if (messenger)
				messenger->show_status(Color::Name::RED, "You can't raise or lower your own fame.");
			break;
		}
		case 4:
		{
			// Error: target not in the map
			if (messenger)
				messenger->show_status(Color::Name::RED, "That character is not in this map.");
			break;
		}
		case 5:
		{
			// Received fame from someone
			if (recv.available())
			{
				std::string charname = recv.read_string();
				int8_t type = recv.read_byte();

				std::string msg = (type == 1)
					? charname + " has raised your fame."
					: charname + " has lowered your fame.";

				if (messenger)
					messenger->show_status(Color::Name::YELLOW, msg);
			}
			break;
		}
		default:
			break;
		}
	}

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
			std::cout << "[BuddyListHandler]: Buddy list update, count: [" << (int)count << "]" << std::endl;

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

				std::cout << "  Buddy: [" << entry.name << "] cid: [" << entry.cid << "] status: [" << (int)entry.status << "] ch: [" << entry.channel << "] online: [" << entry.online() << "]" << std::endl;

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
				std::cout << "[BuddyListHandler]: Buddy capacity: [" << (int)capacity << "]" << std::endl;
			}
			break;
		}
		default:
			std::cout << "[BuddyListHandler]: operation: [" << (int)operation << "] remaining bytes: [" << recv.length() << "]" << std::endl;
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

	void ScriptProgressMessageHandler::handle(InPacket& recv) const
	{
		// v83 SCRIPT_PROGRESS_MESSAGE (opcode 122): string message
		// Shows a progress/info message during scripted events (quests, NPCs, etc.)
		if (!recv.available())
			return;

		std::string message = recv.read_string();

		if (!message.empty())
		{
			auto messenger = UI::get().get_element<UIStatusMessenger>();

			if (messenger)
				messenger->show_status(Color::Name::WHITE, message);
		}
	}

	void ClockHandler::handle(InPacket& recv) const
	{
		// v83: byte type
		// type 0: remove clock
		// type 1: map clock - byte hour, byte min, byte sec
		// type 2: countdown timer - int seconds
		// type 3: real-time clock (wedding, etc.) - byte hour, byte min, byte sec
		// Some servers also use this opcode for FORCED_MAP_EQUIP (type > 10)
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		switch (type)
		{
		case 0:
		{
			// Remove clock
			Stage::get().clear_clock();
			break;
		}
		case 1:
		case 3:
		{
			// Map world clock (1) or real-time clock (3)
			if (recv.length() < 3)
				break;

			int8_t hour = recv.read_byte();
			int8_t min = recv.read_byte();
			int8_t sec = recv.read_byte();

			Stage::get().set_clock(hour, min, sec);
			UI::get().emplace<UIClock>();
			break;
		}
		case 2:
		{
			// Countdown timer
			if (recv.length() < 4)
				break;

			int32_t seconds = recv.read_int();

			Stage::get().set_countdown(seconds);
			UI::get().emplace<UIClock>();
			break;
		}
		default:
			// Some servers reuse this opcode for other purposes (e.g. forced equip display)
			// Silently ignore unknown types
			break;
		}
	}

	void ForcedStatSetHandler::handle(InPacket& recv) const
	{
		// v83: int bitmask of forced stats, then for each set bit: short value
		// Bitmask bits: 0=STR, 1=DEX, 2=INT, 3=LUK, 4=PAD, 5=PDD, 6=MAD, 7=MDD,
		//               8=ACC, 9=EVA, 10=SPEED, 11=JUMP
		if (recv.length() < 4)
			return;

		int32_t mask = recv.read_int();
		CharStats& stats = Stage::get().get_player().get_stats();

		// Map of mask bit -> stat ID for the stats we can set
		struct ForcedStat
		{
			int bit;
			MapleStat::Id stat;
		};

		ForcedStat stat_map[] =
		{
			{ 0, MapleStat::Id::STR },
			{ 1, MapleStat::Id::DEX },
			{ 2, MapleStat::Id::INT },
			{ 3, MapleStat::Id::LUK },
		};

		for (int bit = 0; bit < 12 && recv.length() >= 2; bit++)
		{
			if (mask & (1 << bit))
			{
				int16_t value = recv.read_short();

				// Apply to the known stat mappings
				for (auto& sm : stat_map)
				{
					if (sm.bit == bit)
					{
						stats.set_stat(sm.stat, static_cast<uint16_t>(value));
						break;
					}
				}
			}
		}
	}

	void SetTractionHandler::handle(InPacket& recv) const
	{
		// v83: double traction (8 bytes)
		// Controls floor slipperiness (ice maps, etc.)
		// Low traction = icy/slippery floor
		if (recv.length() < 8)
			return;

		int64_t raw = recv.read_long();
		double traction;
		memcpy(&traction, &raw, sizeof(double));

		// Apply traction to the player's physics object
		// traction = 1.0 is normal, lower values = slippery (ice maps)
		PhysicsObject& phobj = Stage::get().get_player().get_phobj();
		phobj.traction = traction;
	}

	void NpcActionHandler::handle(InPacket& recv) const
	{
		// v83 NPC_ACTION: int oid, then remaining bytes are action data
		// The server periodically sends this to animate NPCs on the map
		if (recv.length() < 4)
			return;

		int32_t oid = recv.read_int();

		// Look up the NPC on the map
		MapObjects* npcs = Stage::get().get_npcs().get_npcs();
		if (!npcs)
			return;

		Optional<MapObject> obj = npcs->get(oid);
		if (!obj)
			return;

		Npc* npc = static_cast<Npc*>(obj.get());

		if (recv.available())
		{
			// Read the action bytes
			int8_t action = recv.read_byte();

			// v83 NPC actions: 0 = stand, 1 = move/walk, 2 = talk, etc.
			switch (action)
			{
			case 0:
				npc->set_stance("stand");
				break;
			case 1:
				npc->set_stance("move");
				break;
			case 2:
				npc->set_stance("say");
				break;
			default:
				npc->set_stance("stand");
				break;
			}
		}
	}

	void YellowTipHandler::handle(InPacket& recv) const
	{
		// v83: byte type, string message
		// Shows a yellow scrolling notice at the top of the screen
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		if (recv.available())
		{
			std::string message = recv.read_string();

			// Show as scrolling notice at top of screen
			UI::get().set_scrollnotice(message);
		}
	}

	void CatchMonsterHandler::handle(InPacket& recv) const
	{
		// v83: int moboid, byte success
		// Result of attempting to catch a monster (e.g. Monster Rider quest)
		if (recv.length() < 4)
			return;

		int32_t mob_oid = recv.read_int();
		int8_t success = recv.available() ? recv.read_byte() : 0;

		auto messenger = UI::get().get_element<UIStatusMessenger>();

		if (success == 1)
		{
			if (messenger)
				messenger->show_status(Color::Name::WHITE, "You have successfully caught the monster!");
		}
		else
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "Failed to catch the monster.");
		}
	}

	void BlowWeatherHandler::handle(InPacket& recv) const
	{
		// v83 BLOW_WEATHER: byte active, int itemid, string message
		// Controls map weather effects (snow, rain, etc.)
		// Weather item IDs map to Effect.wz paths
		if (!recv.available())
			return;

		int8_t active = recv.read_byte();

		if (active != 0 && recv.length() >= 4)
		{
			int32_t itemid = recv.read_int();
			std::string message = recv.available() ? recv.read_string() : "";

			// Map weather item IDs to Effect.wz paths
			// 5120000 = Snow, 5120001 = Rain, 5120002 = Goldfish, etc.
			std::string effect_path = "Map/MapHelper.img/weather/" + std::to_string(itemid);
			Stage::get().add_effect(effect_path);

			// Show the weather message if present
			if (!message.empty())
				UI::get().set_scrollnotice(message);
		}
	}

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
		uint8_t speed = 0;

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

		// Update the character's movement speed if a speed buff was present
		if (speed > 0)
		{
			Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);
			if (ochar)
				ochar->update_speed(speed);
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

		// If speed buff was cancelled (bit 7), reset speed
		if (buffmask & (1LL << 7))
		{
			Optional<OtherChar> ochar = Stage::get().get_chars().get_char(cid);
			if (ochar)
				ochar->update_speed(100); // default speed
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

	void QuickSlotInitHandler::handle(InPacket& recv) const
	{
		// Quick slot key init — read 8 int32 key bindings for the quick slot bar
		if (recv.available())
		{
			bool init = recv.read_bool();

			if (init)
			{
				for (int i = 0; i < 8; i++)
				{
					if (recv.available())
						recv.read_int(); // key binding for quick slot i
				}
			}
		}
	}

	void QueryCashResultHandler::handle(InPacket& recv) const
	{
		// Cash query result — returns the player's NX cash balance
		if (recv.available())
		{
			int32_t cash_nx = recv.read_int();
			int32_t maple_points = recv.read_int();
			int32_t cash_prepaid = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Cash] NX: " + std::to_string(cash_nx) + " | MaplePoints: " + std::to_string(maple_points) + " | Prepaid: " + std::to_string(cash_prepaid), UIChatBar::LineType::YELLOW);
		}
	}

	void LastConnectedWorldHandler::handle(InPacket& recv) const
	{
		// Last connected world — auto-select this world in world select UI
		if (!recv.available())
			return;

		int32_t worldid = recv.read_int();

		auto worldselect = UI::get().get_element<UIWorldSelect>();

		if (worldselect)
		{
			// Store the last connected world so auto-select works
			Configuration::get().set_worldid(static_cast<uint8_t>(worldid));
		}
	}

	void ClaimStatusChangedHandler::handle(InPacket& recv) const
	{
		// Enable/disable report button — just consume, no UI action needed
		if (recv.available())
			recv.read_byte(); // 1 = enabled
	}

	void SetGenderHandler::handle(InPacket& recv) const
	{
		// Gender confirmation from server after gender selection
		// Just consume — gender is already set in CharStats from login
		if (recv.available())
			recv.read_byte(); // gender (0=male, 1=female)
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

	void AdminResultHandler::handle(InPacket& recv) const
	{
		// GM command result — read type and mode
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		if (recv.available())
		{
			int8_t mode = recv.read_byte();
			(void)type;
			(void)mode;
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

	void SetNpcScriptableHandler::handle(InPacket& recv) const
	{
		// Marks NPCs as scriptable (quest NPCs, event NPCs, etc.)
		// Format: byte count, then for each: int npcid, string name, int start, int end
		if (!recv.available())
			return;

		int8_t count = recv.read_byte();

		for (int8_t i = 0; i < count && recv.available(); i++)
		{
			int32_t npcid = recv.read_int();
			std::string npc_name = recv.read_string();
			recv.read_int(); // start time (0)
			recv.read_int(); // end time (MAX_INT)

			(void)npcid;
			(void)npc_name;
			// NPCs marked as scriptable — in v83 this means they have a chat icon
			// The NPC's scripted flag is already set from NX data in Npc constructor
		}
	}

	void AutoHpPotHandler::handle(InPacket& recv) const
	{
		// Auto HP potion — server tells client which potion to auto-use
		if (!recv.available())
			return;

		int32_t itemid = recv.read_int();

		// Store in configuration for auto-use during gameplay
		Configuration::get().set_auto_hp_pot(itemid);
	}

	void AutoMpPotHandler::handle(InPacket& recv) const
	{
		// Auto MP potion — server tells client which potion to auto-use
		if (!recv.available())
			return;

		int32_t itemid = recv.read_int();

		// Store in configuration for auto-use during gameplay
		Configuration::get().set_auto_mp_pot(itemid);
	}

	namespace
	{
		// Parse a single storage item — same as addItemInfo on server side
		// Returns: invtype_byte, itemid, count
		struct ParsedStorageItem
		{
			int8_t invtype_byte;
			int32_t itemid;
			int16_t count;
		};

		ParsedStorageItem parse_storage_item(InPacket& recv)
		{
			ParsedStorageItem result = { 0, 0, 0 };

			int8_t type_byte = recv.read_byte(); // 1=equip, 2=use, etc.
			int32_t itemid = recv.read_int();
			result.itemid = itemid;

			// Determine inventory type from item ID
			int32_t prefix = itemid / 1000000;
			if (prefix == 1)
				result.invtype_byte = 1; // equip
			else if (prefix == 2)
				result.invtype_byte = 2; // use
			else if (prefix == 3)
				result.invtype_byte = 3; // setup
			else if (prefix == 4)
				result.invtype_byte = 4; // etc
			else if (prefix == 5)
				result.invtype_byte = 5; // cash
			else
				result.invtype_byte = type_byte;

			bool is_equip = (result.invtype_byte == 1);

			// Cash flag
			bool cash = recv.read_bool();
			if (cash)
				recv.skip(8); // unique ID

			// Expiration
			recv.skip(8); // expire timestamp

			if (is_equip)
			{
				// Equip data
				recv.read_byte();  // upgrade slots
				recv.read_byte();  // level

				// 14 equip stats (STR, DEX, INT, LUK, HP, MP, WATK, MATK, WDEF, MDEF, ACC, AVOID, HANDS, SPEED, JUMP)
				// EnumMap has exactly this many stats
				for (int i = 0; i < 15; i++)
					recv.read_short();

				recv.read_string(); // owner
				recv.read_short();  // flag

				if (cash)
				{
					recv.skip(10);
				}
				else
				{
					recv.read_byte();   // unk
					recv.read_byte();   // item level
					recv.read_short();  // unk
					recv.read_short();  // item exp
					recv.read_int();    // vicious
					recv.read_long();   // unk
				}

				recv.skip(12); // trailing data

				result.count = 1;
			}
			else if (itemid >= 5000000 && itemid <= 5000102)
			{
				// Pet
				std::string petname = recv.read_padded_string(13);
				recv.read_byte();  // pet level
				recv.read_short(); // closeness
				recv.read_byte();  // fullness
				recv.skip(18);     // unused
				result.count = 1;
			}
			else
			{
				// Regular item
				result.count = recv.read_short();
				recv.read_string(); // owner
				recv.read_short();  // flag

				// Rechargeable check
				if ((itemid / 10000 == 233) || (itemid / 10000 == 207))
					recv.skip(8);
			}

			return result;
		}
	}

	void StorageHandler::handle(InPacket& recv) const
	{
		// v83 STORAGE: byte mode, then mode-specific data
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();
		auto messenger = UI::get().get_element<UIStatusMessenger>();

		switch (mode)
		{
		case 0x16:
		{
			// Open storage: int npcid, byte slots, short 0x7E, short 0, int 0, int meso, short 0, byte count, items...
			if (recv.length() < 4)
				break;

			int32_t npcid = recv.read_int();
			int8_t slots = recv.read_byte();
			recv.read_short(); // flag (0x7E)
			recv.read_short(); // padding
			recv.read_int();   // padding
			int32_t meso = recv.read_int();
			recv.read_short(); // padding
			int8_t item_count = recv.read_byte();

			const Inventory& inv = Stage::get().get_player().get_inventory();
			UI::get().emplace<UIStorage>(inv);

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_storage(npcid, slots, meso);

				for (int8_t i = 0; i < item_count && recv.available(); i++)
				{
					ParsedStorageItem psi = parse_storage_item(recv);
					storage_ui->add_stored_item(psi.invtype_byte, i, psi.itemid, psi.count);
				}

				storage_ui->refresh_items();
			}

			// Consume remaining bytes (trailing padding)
			break;
		}
		case 0x09:
		{
			// Take out result: byte slots, short invtype_bitfield, short 0, int 0, byte count, items...
			if (recv.length() < 2)
				break;

			int8_t slots = recv.read_byte();
			recv.read_short(); // inv type bitfield
			recv.read_short(); // padding
			recv.read_int();   // padding
			int8_t count = recv.read_byte();

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				// Clear and re-add items
				storage_ui->set_storage(0, slots, 0);

				for (int8_t i = 0; i < count && recv.available(); i++)
				{
					ParsedStorageItem psi = parse_storage_item(recv);
					storage_ui->add_stored_item(psi.invtype_byte, i, psi.itemid, psi.count);
				}

				storage_ui->refresh_items();
			}
			break;
		}
		case 0x0D:
		{
			// Store result: byte slots, short invtype_bitfield, short 0, int 0, byte count, items...
			if (recv.length() < 2)
				break;

			int8_t slots = recv.read_byte();
			recv.read_short();
			recv.read_short();
			recv.read_int();
			int8_t count = recv.read_byte();

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_storage(0, slots, 0);

				for (int8_t i = 0; i < count && recv.available(); i++)
				{
					ParsedStorageItem psi = parse_storage_item(recv);
					storage_ui->add_stored_item(psi.invtype_byte, i, psi.itemid, psi.count);
				}

				storage_ui->refresh_items();
			}
			break;
		}
		case 0x13:
		{
			// Meso update: byte slots, short 2, short 0, int 0, int meso
			if (recv.length() < 2)
				break;

			recv.read_byte();  // slots
			recv.read_short(); // flag
			recv.read_short(); // padding
			recv.read_int();   // padding
			int32_t meso = recv.read_int();

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
				storage_ui->update_meso(meso);

			break;
		}
		case 0x0F:
		{
			// Arrange result: byte slots, byte 124, 10 bytes skip, byte count, items..., byte 0
			if (recv.length() < 2)
				break;

			int8_t slots = recv.read_byte();
			recv.read_byte(); // 124
			recv.skip(10);
			int8_t count = recv.read_byte();

			auto storage_ui = UI::get().get_element<UIStorage>();
			if (storage_ui)
			{
				storage_ui->set_storage(0, slots, 0);

				for (int8_t i = 0; i < count && recv.available(); i++)
				{
					ParsedStorageItem psi = parse_storage_item(recv);
					storage_ui->add_stored_item(psi.invtype_byte, i, psi.itemid, psi.count);
				}

				storage_ui->refresh_items();
			}

			if (recv.available())
				recv.read_byte(); // trailing 0

			break;
		}
		case 0x0A:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "Your inventory is full.");
			break;
		}
		case 0x0B:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "You do not have enough mesos.");
			break;
		}
		case 0x0C:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "You already have that one-of-a-kind item.");
			break;
		}
		case 0x11:
		{
			if (messenger)
				messenger->show_status(Color::Name::RED, "Storage is full.");
			break;
		}
		default:
			std::cout << "[StorageHandler]: mode " << (int)mode
				<< " remaining: " << recv.length() << std::endl;
			break;
		}
	}

	void FieldObstacleOnOffHandler::handle(InPacket& recv) const
	{
		// Field obstacle on/off list — controls gates, platforms, etc. in maps/PQs
		// Format: int count, then for each: string envName, int mode (0=stop/reset, 1=move)
		if (!recv.available())
			return;

		int32_t count = recv.read_int();

		for (int32_t i = 0; i < count && recv.available(); i++)
		{
			std::string env_name = recv.read_string();
			int32_t mode = recv.read_int();

			Stage::get().toggle_environment(env_name, mode);
		}
	}

	void PlayerInteractionHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t action = recv.read_byte();

		switch (action)
		{
		case 2:  // INVITE
		{
			int8_t type = recv.read_byte();
			std::string inviter = recv.read_string();

			// Read trailing bytes
			if (recv.available())
				recv.skip(4);

			if (type == 3) // Trade invite
			{
				// Show accept/decline dialog
				UI::get().emplace<UIYesNo>(
					"Trade request from " + inviter + ". Accept?",
					[](bool yes)
					{
						if (yes)
						{
							// Create and visit the trade
							TradeCreatePacket().dispatch();
						}
						else
						{
							TradeDeclinePacket().dispatch();
						}
					}
				);
			}
			break;
		}

		case 4:  // VISIT — partner joined the trade
		{
			uint8_t slot = recv.read_byte();

			// Skip partner appearance data (addCharLook)
			// This is complex; skip what we can
			// For now, just read the partner name at the end
			// We'll skip the look data by reading until we find the name

			// In v83: addCharLook writes gender(byte), skin(byte), face(int),
			// hair flag(byte), hair(int), then equipped items, then cash items,
			// then end marker -1, then pets
			// This is variable length, so we need to parse it

			int8_t gender = recv.read_byte();
			int8_t skin = recv.read_byte();
			int32_t face = recv.read_int();

			// mega flag
			recv.skip_byte();

			int32_t hair = recv.read_int();

			// Equipped items (slot, itemid pairs until slot == -1)
			while (recv.available())
			{
				int8_t equipped_slot = recv.read_byte();
				if (equipped_slot == -1)
					break;
				recv.skip_int(); // item id
			}

			// Cash items (slot, itemid pairs until slot == -1)
			while (recv.available())
			{
				int8_t cash_slot = recv.read_byte();
				if (cash_slot == -1)
					break;
				recv.skip_int(); // item id
			}

			// Skip remaining look data (weapon sticker, pets)
			recv.skip_int(); // weapon sticker

			for (int i = 0; i < 3; i++)
				recv.skip_int(); // pet ids

			std::string partner_name = recv.read_string();

			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_partner(slot, partner_name);

			break;
		}

		case 5:  // ROOM — trade room setup
		{
			int8_t room_type = recv.read_byte();

			if (room_type == 3) // Trade
			{
				int8_t room_mode = recv.read_byte();
				uint8_t player_num = recv.read_byte();

				// Create the trade UI
				UI::get().emplace<UITrade>();

				if (player_num == 1)
				{
					// Partner data follows
					recv.skip_byte(); // slot

					// Skip partner addCharLook
					recv.skip_byte(); // gender
					recv.skip_byte(); // skin
					recv.skip_int();  // face
					recv.skip_byte(); // mega flag
					recv.skip_int();  // hair

					// Equipped items
					while (recv.available())
					{
						int8_t eslot = recv.read_byte();
						if (eslot == -1) break;
						recv.skip_int();
					}

					// Cash items
					while (recv.available())
					{
						int8_t cslot = recv.read_byte();
						if (cslot == -1) break;
						recv.skip_int();
					}

					recv.skip_int(); // weapon sticker
					for (int i = 0; i < 3; i++)
						recv.skip_int(); // pet ids

					std::string partner_name = recv.read_string();

					if (auto trade = UI::get().get_element<UITrade>())
						trade->set_partner(1, partner_name);
				}

				// Self data
				uint8_t self_num = recv.read_byte();

				// Skip self addCharLook
				recv.skip_byte(); // gender
				recv.skip_byte(); // skin
				recv.skip_int();  // face
				recv.skip_byte(); // mega flag
				recv.skip_int();  // hair

				// Equipped items
				while (recv.available())
				{
					int8_t eslot = recv.read_byte();
					if (eslot == -1) break;
					recv.skip_int();
				}

				// Cash items
				while (recv.available())
				{
					int8_t cslot = recv.read_byte();
					if (cslot == -1) break;
					recv.skip_int();
				}

				recv.skip_int(); // weapon sticker
				for (int i = 0; i < 3; i++)
					recv.skip_int(); // pet ids

				recv.skip_string(); // self name

				// End marker
				if (recv.available())
					recv.skip_byte(); // 0xFF
			}
			break;
		}

		case 10: // EXIT — trade cancelled/completed
		{
			uint8_t player_num = recv.read_byte();
			uint8_t result = recv.read_byte();

			if (auto trade = UI::get().get_element<UITrade>())
			{
				trade->trade_result(result);

				// Close after showing result
				if (result == 7) // SUCCESS
				{
					// Trade completed successfully
				}
			}

			// Deactivate trade window
			if (auto trade = UI::get().get_element<UITrade>())
				trade->deactivate();

			break;
		}

		case 15: // SET_ITEMS — item added to trade
		{
			uint8_t player_num = recv.read_byte();
			uint8_t trade_slot = recv.read_byte();

			// Parse item info — simplified version
			// Full item info includes type detection, but for trade we read basics
			int8_t item_type = recv.read_byte(); // 1=equip, 2=use/etc

			if (item_type == 1)
			{
				// Equip item
				int32_t itemid = recv.read_int();

				// Skip equip stats (variable, but typically ~40+ bytes)
				// Flags
				bool has_uid = recv.read_bool();
				if (has_uid)
					recv.skip_long(); // unique id

				recv.skip_long(); // expiration

				// Equip stats: 15 shorts + owner string + flags
				for (int i = 0; i < 15; i++)
					recv.skip_short();

				recv.skip_string(); // owner
				recv.skip_byte();   // flag

				// Additional bytes depend on type
				if (recv.available())
				{
					recv.skip_byte(); // level up type
					recv.skip_byte(); // level
					recv.skip_int();  // exp
					recv.skip_int();  // vicious
					recv.skip_int();  // flag2
				}

				if (auto trade = UI::get().get_element<UITrade>())
					trade->set_item(player_num, trade_slot, itemid, 1);
			}
			else
			{
				// Regular item
				int32_t itemid = recv.read_int();

				bool has_uid = recv.read_bool();
				if (has_uid)
					recv.skip_long();

				recv.skip_long(); // expiration

				int16_t quantity = recv.read_short();
				recv.skip_string(); // owner
				recv.skip_short();  // flag

				// Rechargeable check
				if ((itemid / 10000) == 207 || (itemid / 10000) == 233)
					recv.skip_long(); // recharge bytes

				if (auto trade = UI::get().get_element<UITrade>())
					trade->set_item(player_num, trade_slot, itemid, quantity);
			}
			break;
		}

		case 16: // SET_MESO
		{
			uint8_t player_num = recv.read_byte();
			int32_t meso = recv.read_int();

			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_meso(player_num, meso);

			break;
		}

		case 17: // CONFIRM
		{
			if (auto trade = UI::get().get_element<UITrade>())
				trade->set_partner_confirmed();

			break;
		}

		case 6:  // CHAT
		{
			if (recv.available())
			{
				recv.skip_byte(); // CHAT_THING (0x08)
				uint8_t speaker = recv.read_byte();
				std::string message = recv.read_string();

				if (auto trade = UI::get().get_element<UITrade>())
					trade->add_chat(message);
			}
			break;
		}

		default:
			break;
		}
	}

	void GuildOperationHandler::handle(InPacket& recv) const
	{
		// v83 guild operation response
		// Opens or updates the guild UI based on the operation type
		int8_t type = recv.read_byte();

		switch (type)
		{
		case 0x26: // Guild info
		case 0x27: // Guild update
		{
			if (auto guild = UI::get().get_element<UIGuild>())
				guild->makeactive();
			else
				UI::get().emplace<UIGuild>();

			break;
		}
		default:
			std::cout << "[GuildOperation] Unhandled guild op type: " << (int)type << std::endl;
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

	// Opcode 196 (0xC4) — SHOW_CHAIR
	// Shows a chair being used by a foreign character on the map
	void ShowChairHandler::handle(InPacket& recv) const
	{
		int32_t charid = recv.read_int();
		int32_t itemid = recv.read_int();

		std::cout << "[ShowChair] charid=" << charid << " itemid=" << itemid << std::endl;

		if (auto other = Stage::get().get_character(charid))
		{
			// Set the character's chair state
			// itemid 0 means get up from chair
			if (itemid > 0)
				other->set_direction(false); // face right in chair by default
		}
	}

	// Opcode 306 (0x132) — CONFIRM_SHOP_TRANSACTION
	// Server confirms a shop buy/sell transaction result
	void ConfirmShopTransactionHandler::handle(InPacket& recv) const
	{
		int8_t code = recv.read_byte();

		std::cout << "[ConfirmShopTransaction] code=" << (int)code << std::endl;

		// code 0 = success (inventory already updated via MODIFY_INVENTORY)
		// code 1-3 = various errors (not enough mesos, inventory full, etc.)
		if (code != 0)
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "Transaction failed.");
		}
	}

	// Opcode 322 (0x142) — PARCEL (Duey delivery system)
	// Handles parcel/delivery notifications and operations
	void ParcelHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		std::cout << "[Parcel] operation=" << (int)operation << std::endl;

		// Duey operations:
		// 8 = Open Duey window
		// 5 = Parcel received notification
		// 7 = Package list
		// 4 = Remove package
		// Other codes exist for errors
	}

	void OpenUIHandler::handle(InPacket& recv) const
	{
		int8_t ui_type = recv.read_byte();

		std::cout << "[OpenUI] type=" << (int)ui_type << std::endl;

		// UI types from Cosmic:
		// 0x04 = Equipment Enhance (Star Force)
		// 0x08 = Item Maker
		// 0x12 = Monster Book
		// 0x16 = Equipment Enhance (Scroll)
		// 0x21 = Profession
		// Others are typically quest/event UIs
	}

	void LockUIHandler::handle(InPacket& recv) const
	{
		bool locked = recv.read_byte() != 0;

		if (locked)
			UI::get().disable();
		else
			UI::get().enable();
	}

	void DisableUIHandler::handle(InPacket& recv) const
	{
		bool disabled = recv.read_byte() != 0;

		if (disabled)
			UI::get().disable();
		else
			UI::get().enable();
	}

	void SpawnGuideHandler::handle(InPacket& recv) const
	{
		bool spawn = recv.read_bool();

		std::cout << "[SpawnGuide] spawn=" << spawn << std::endl;
		// Beginner guide NPC — would need a dedicated guide entity
	}

	void TalkGuideHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 0)
		{
			std::string text = recv.read_string();
			recv.skip(8); // two ints: display timing (200, 4000)

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guide] " + text, UIChatBar::LineType::YELLOW);
		}
		else if (mode == 1)
		{
			int32_t hint_id = recv.read_int();
			int32_t duration = recv.read_int();

			std::cout << "[TalkGuide] hint=" << hint_id << " duration=" << duration << std::endl;
		}
	}

	void ShowComboHandler::handle(InPacket& recv) const
	{
		int32_t count = recv.read_int();

		std::cout << "[ShowCombo] count=" << count << std::endl;
		// Would need a combo counter UI element
	}

	void ForcedMapEquipHandler::handle(InPacket& recv) const
	{
		if (recv.available())
		{
			int8_t team = recv.read_byte(); // 0 = red, 1 = blue
			std::cout << "[ForcedMapEquip] team=" << (int)team << std::endl;
		}
		// Forces the player to wear map-specific equipment (e.g., PQ outfits)
	}

	void SpouseChatHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte(); // 5 = spouse, 4 = engaged
		std::string name;

		if (mode == 5)
			name = recv.read_string();

		int8_t submode = recv.read_byte();
		std::string text = recv.read_string();

		std::string prefix = (mode == 5) ? "[Spouse] " : "[Fiance] ";
		std::string line = prefix + name + ": " + text;

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(line, UIChatBar::LineType::RED);
	}

	void ContiMoveHandler::handle(InPacket& recv) const
	{
		int8_t event_type = recv.read_byte(); // 10 = boat/crog
		int8_t move_type = recv.read_byte();  // 4 = start, 5 = stop

		std::cout << "[ContiMove] event=" << (int)event_type
				  << " type=" << (int)move_type << std::endl;
		// Would need a boat/ship travel visual system
	}

	void ContiStateHandler::handle(InPacket& recv) const
	{
		int8_t state = recv.read_byte(); // 1 = start, 2 = end
		recv.read_byte(); // padding

		std::cout << "[ContiState] state=" << (int)state << std::endl;
	}

	void MakerResultHandler::handle(InPacket& recv) const
	{
		int32_t result = recv.read_int(); // 0 = success, 1 = fail
		int32_t mode = recv.read_int();   // 1/2 = craft, 3 = crystal, 4 = desynth

		if (mode == 1 || mode == 2)
		{
			bool failed = recv.read_bool();

			if (!failed)
			{
				int32_t item_made = recv.read_int();
				int32_t count = recv.read_int();
			}

			int32_t num_consumed = recv.read_int();

			for (int32_t i = 0; i < num_consumed; i++)
			{
				recv.read_int(); // item id
				recv.read_int(); // quantity
			}

			int32_t num_gems = recv.read_int();

			for (int32_t i = 0; i < num_gems; i++)
				recv.read_int(); // gem id

			int8_t has_catalyst = recv.read_byte();

			if (has_catalyst)
				recv.read_int(); // catalyst id

			recv.read_int(); // mesos cost

			std::string msg = failed ? "Item creation failed." : "Item created successfully!";

			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(failed ? Color::Name::RED : Color::Name::WHITE, msg);
		}
		else if (mode == 3) // Monster Crystal
		{
			recv.read_int(); // item gained
			recv.read_int(); // item lost
		}
		else if (mode == 4) // Desynth
		{
			recv.read_int(); // item id
			int32_t num_gained = recv.read_int();

			for (int32_t i = 0; i < num_gained; i++)
			{
				recv.read_int(); // item id
				recv.read_int(); // quantity
			}

			recv.read_int(); // mesos
		}
	}

	void MonsterCarnivalStartHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();
		int16_t team_cp = recv.read_short();
		int16_t team_total_cp = recv.read_short();
		int16_t opp_cp = recv.read_short();
		int16_t opp_total_cp = recv.read_short();
		recv.read_short(); // unused
		recv.read_long();  // unused

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
		{
			carnival->set_team(team);
			carnival->set_cp(cp, total_cp, opp_cp, opp_total_cp);
		}
	}

	void MonsterCarnivalCPHandler::handle(InPacket& recv) const
	{
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
			carnival->set_cp(cp, total_cp, -1, -1);
	}

	void MonsterCarnivalPartyCPHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		int16_t cp = recv.read_short();
		int16_t total_cp = recv.read_short();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
		{
			// Team 0 = my team, team 1 = enemy
			if (team == 0)
				carnival->set_cp(cp, total_cp, -1, -1);
			else
				carnival->set_cp(-1, -1, cp, total_cp);
		}
	}

	void MonsterCarnivalSummonHandler::handle(InPacket& recv) const
	{
		int8_t tab = recv.read_byte();
		int8_t number = recv.read_byte();
		std::string name = recv.read_string();

		if (auto carnival = UI::get().get_element<UIMonsterCarnival>())
			carnival->set_summon_count(tab, number);

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + name + " summoned a monster!", UIChatBar::LineType::YELLOW);
	}

	void MonsterCarnivalMessageHandler::handle(InPacket& recv) const
	{
		int8_t message = recv.read_byte();

		std::string msg;
		switch (message)
		{
		case 1: msg = "You don't have enough CP!"; break;
		case 2: msg = "The summon limit has been reached!"; break;
		case 3: msg = "You cannot summon during this phase!"; break;
		case 4: msg = "The carnival is about to end!"; break;
		default: msg = "Carnival message: " + std::to_string(message); break;
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + msg, UIChatBar::LineType::YELLOW);
	}

	void MonsterCarnivalDiedHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte();
		std::string name = recv.read_string();
		int8_t lost_cp = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Carnival] " + name + " was eliminated! (-" + std::to_string(lost_cp) + " CP)", UIChatBar::LineType::RED);
	}

	void SpawnHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		int32_t item_id = recv.read_int();
		Point<int16_t> position = recv.read_point();
		recv.read_short(); // 0
		std::string owner_name = recv.read_string();
		recv.read_byte(); // 0x05
		int32_t object_id = recv.read_int();
		std::string description = recv.read_string();
		recv.read_byte(); // sprite index
		recv.read_byte(); // 1
		recv.read_byte(); // 4

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] " + owner_name + "'s shop: " + description, UIChatBar::LineType::YELLOW);
	}

	void DestroyHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] A hired merchant has been removed.", UIChatBar::LineType::YELLOW);
	}

	void UpdateHiredMerchantHandler::handle(InPacket& recv) const
	{
		int32_t owner_id = recv.read_int();
		recv.read_byte(); // 5 = hired merchant type
		int32_t object_id = recv.read_int();
		std::string description = recv.read_string();
		recv.read_byte(); // sprite index
		int8_t cur_visitors = recv.read_byte();
		int8_t max_visitors = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[HiredMerchant] " + description + " (" + std::to_string(cur_visitors) + "/" + std::to_string(max_visitors) + " visitors)", UIChatBar::LineType::YELLOW);
	}

	void FredrickMessageHandler::handle(InPacket& recv) const
	{
		int8_t operation = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Fredrick] Message (op=" + std::to_string(operation) + ")", UIChatBar::LineType::YELLOW);
	}

	void FredrickHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		// 0x23 = retrieve items, 0x24 = error
		// Full parsing would require ItemInfo deserialization
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Fredrick] Response (op=" + std::to_string(op) + ")", UIChatBar::LineType::YELLOW);
	}

	void StopClockHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 0

		Stage::get().clear_clock();
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
			std::cout << "[Messenger] mode=" << (int)mode << std::endl;
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

	void OXQuizHandler::handle(InPacket& recv) const
	{
		bool asking = recv.read_byte() != 0;
		int8_t question_set = recv.read_byte();
		int16_t question_id = recv.read_short();

		if (asking)
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[OX Quiz] Question " + std::to_string(question_set) + "-" + std::to_string(question_id) + ": Move to O or X!", UIChatBar::LineType::YELLOW);
		}
	}

	void SnowballStateHandler::handle(InPacket& recv) const
	{
		// First check if it's an enter-map packet (21 zero bytes) or state update
		int8_t state = recv.read_byte();

		if (state == 0) // Could be entermap zeroes
		{
			recv.skip(20); // rest of the 21 zeroes
		}
		else
		{
			int32_t ball0_hp = recv.read_int();
			int32_t ball1_hp = recv.read_int();
			recv.read_short(); // ball0 position
			recv.read_byte();  // -1
			recv.read_short(); // ball1 position
			recv.read_byte();  // -1

			std::cout << "[Snowball] state=" << (int)state
					  << " HP: " << ball0_hp << " vs " << ball1_hp << std::endl;
		}
	}

	void HitSnowballHandler::handle(InPacket& recv) const
	{
		int8_t what = recv.read_byte();
		int32_t damage = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Snowball] Hit! Damage: " + std::to_string(damage), UIChatBar::LineType::YELLOW);
	}

	void SnowballMessageHandler::handle(InPacket& recv) const
	{
		int8_t team = recv.read_byte(); // 0=down, 1=up
		int32_t message = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Snowball] Team " + std::to_string(team) + " event update!", UIChatBar::LineType::YELLOW);
	}

	void CoconutHitHandler::handle(InPacket& recv) const
	{
		int16_t id = recv.read_short();
		int16_t delay = recv.read_short();
		int8_t type = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Coconut] Hit id=" + std::to_string(id) + " type=" + std::to_string(type), UIChatBar::LineType::YELLOW);
	}

	void CoconutScoreHandler::handle(InPacket& recv) const
	{
		int16_t team1 = recv.read_short();
		int16_t team2 = recv.read_short();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Coconut] Score: Team 1 = " + std::to_string(team1) + " | Team 2 = " + std::to_string(team2), UIChatBar::LineType::YELLOW);
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

	void SendTVHandler::handle(InPacket& recv) const
	{
		int8_t has_partner = recv.read_byte(); // 3 = with partner, 1 = solo
		int8_t type = recv.read_byte(); // 0=normal, 1=star, 2=heart

		// CharLook + messages follow — complex parsing
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] Broadcast received.", UIChatBar::LineType::YELLOW);
	}

	void RemoveTVHandler::handle(InPacket& recv) const
	{
		// Empty packet body
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] Broadcast ended.", UIChatBar::LineType::YELLOW);
	}

	void EnableTVHandler::handle(InPacket& recv) const
	{
		recv.read_int(); // 0
		recv.read_byte(); // 0

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[MapleTV] MapleTV enabled.", UIChatBar::LineType::YELLOW);
	}

	void SetAvatarMegaphoneHandler::handle(InPacket& recv) const
	{
		int32_t item_id = recv.read_int();
		std::string name = recv.read_string();

		// Multiple message strings follow, then channel, ear flag, and CharLook
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[AvatarMegaphone] " + name, UIChatBar::LineType::YELLOW);
	}

	void ClearAvatarMegaphoneHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 1
	}

	void SpawnKiteHandler::handle(InPacket& recv) const
	{
		int32_t obj_id = recv.read_int();
		int32_t item_id = recv.read_int();
		std::string msg = recv.read_string();
		std::string name = recv.read_string();
		int16_t x = recv.read_short();
		int16_t fh = recv.read_short();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Kite] " + name + ": " + msg, UIChatBar::LineType::YELLOW);
	}

	void RemoveKiteHandler::handle(InPacket& recv) const
	{
		int8_t anim = recv.read_byte();
		int32_t obj_id = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Kite] A kite has been removed.", UIChatBar::LineType::YELLOW);
	}

	void CannotSpawnKiteHandler::handle(InPacket& recv) const
	{
		// Empty body
		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, "Cannot place a kite here.");
	}

	void AriantScoreHandler::handle(InPacket& recv) const
	{
		int8_t count = recv.read_byte();

		for (int8_t i = 0; i < count; i++)
		{
			std::string name = recv.read_string();
			int32_t score = recv.read_int();
		}
	}

	void AriantShowResultHandler::handle(InPacket& recv) const
	{
		// Empty body — triggers result dialog
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Ariant] Match results!", UIChatBar::LineType::YELLOW);
	}

	void PyramidGaugeHandler::handle(InPacket& recv) const
	{
		int32_t gauge = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pyramid] Gauge: " + std::to_string(gauge), UIChatBar::LineType::YELLOW);
	}

	void PyramidScoreHandler::handle(InPacket& recv) const
	{
		int8_t rank = recv.read_byte(); // 0=S, 1=A, 2=B, 3=C, 4=D
		int32_t exp = recv.read_int();

		std::string ranks[] = { "S", "A", "B", "C", "D" };
		std::string rank_str = (rank >= 0 && rank <= 4) ? ranks[rank] : "?";

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Pyramid] Rank " + rank_str + " - EXP: " + std::to_string(exp), UIChatBar::LineType::YELLOW);
	}

	void TournamentHandler::handle(InPacket& recv) const
	{
		int8_t state = recv.read_byte();
		int8_t sub_state = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] State update: " + std::to_string(state) + "/" + std::to_string(sub_state), UIChatBar::LineType::YELLOW);
	}

	void TournamentMatchTableHandler::handle(InPacket& recv) const
	{
		// Empty body — prompts match table dialog
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] Match table received.", UIChatBar::LineType::YELLOW);
	}

	void TournamentSetPrizeHandler::handle(InPacket& recv) const
	{
		int8_t set_prize = recv.read_byte();
		int8_t has_prize = recv.read_byte();

		if (has_prize)
		{
			int32_t item1 = recv.read_int();
			int32_t item2 = recv.read_int();
		}
	}

	void TournamentUEWHandler::handle(InPacket& recv) const
	{
		int8_t state = recv.read_byte(); // bitflags for round info

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Tournament] UEW state: " + std::to_string(state), UIChatBar::LineType::YELLOW);
	}

	void DojoWarpUpHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 0
		recv.read_byte(); // 6

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Dojo] Warping to next floor!", UIChatBar::LineType::YELLOW);
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

	void SkillLearnItemResultHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte(); // 1
		int32_t skill_id = recv.read_int();
		int32_t max_level = recv.read_int();
		bool can_use = recv.read_byte() != 0;
		bool success = recv.read_byte() != 0;

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
		{
			if (success)
				messenger->show_status(Color::Name::WHITE, "Skill book used successfully!");
			else
				messenger->show_status(Color::Name::RED, "Skill book usage failed.");
		}
	}

	void CatchMonsterWithItemHandler::handle(InPacket& recv) const
	{
		int32_t mob_oid = recv.read_int();
		int32_t item_id = recv.read_int();
		int8_t success = recv.read_byte();

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
		{
			if (success)
				messenger->show_status(Color::Name::WHITE, "Monster captured!");
			else
				messenger->show_status(Color::Name::RED, "Failed to capture the monster.");
		}
	}

	void SetBackEffectHandler::handle(InPacket& recv) const
	{
		bool remove = recv.read_bool();
		recv.read_int(); // unknown
		int8_t layer = recv.read_byte();
		int32_t transition = recv.read_int();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[BackEffect] " + std::string(remove ? "Removed" : "Applied") + " layer=" + std::to_string(layer), UIChatBar::LineType::YELLOW);
	}

	void BlockedMapHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();

		std::string messages[] = {
			"",
			"Equipment limitations prevent entry.",
			"One-handed weapon required.",
			"Level requirement not met.",
			"Skill requirement not met.",
			"Ground force map.",
			"Party members only.",
			"Cash Shop is currently unavailable."
		};

		std::string msg = (type >= 1 && type <= 7) ? messages[type] : "Cannot enter this map.";

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, msg);
	}

	void BlockedServerHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();

		std::string messages[] = {
			"",
			"Cannot change channel right now.",
			"Cannot access Cash Shop right now.",
			"Trading shop is unavailable.",
			"Trading shop user limit reached.",
			"Level requirement for trading shop not met."
		};

		std::string msg = (type >= 1 && type <= 5) ? messages[type] : "Action is currently blocked.";

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::RED, msg);
	}

	void SetExtraPendantSlotHandler::handle(InPacket& recv) const
	{
		bool enabled = recv.read_bool();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Equip] Extra pendant slot " + std::string(enabled ? "enabled" : "disabled") + ".", UIChatBar::LineType::YELLOW);
	}

	void ViciousHammerHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		if (op == 0x39)
		{
			recv.read_int(); // 0
			int32_t hammer_used = recv.read_int();
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[ViciousHammer] Used " + std::to_string(hammer_used) + " times.", UIChatBar::LineType::YELLOW);
		}
		else if (op == 0x3D)
		{
			recv.read_int(); // 0
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[ViciousHammer] Hammering complete!", UIChatBar::LineType::YELLOW);
		}
	}

	void VegaScrollHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[VegaScroll] Result (op=" + std::to_string(op) + ")", UIChatBar::LineType::YELLOW);
	}

	void NewYearCardHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		// Complex multi-mode packet — card data, errors, broadcasts
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[NewYearCard] Card update (mode=" + std::to_string(mode) + ")", UIChatBar::LineType::YELLOW);
	}

	void CashShopNameChangeHandler::handle(InPacket& recv) const
	{
		std::string name = recv.read_string();
		bool in_use = recv.read_bool();

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
		{
			if (in_use)
				messenger->show_status(Color::Name::RED, "Name '" + name + "' is already in use.");
			else
				messenger->show_status(Color::Name::WHITE, "Name '" + name + "' is available!");
		}
	}

	void CashShopNameChangePossibleHandler::handle(InPacket& recv) const
	{
		recv.read_int(); // 0
		int8_t error = recv.read_byte();
		recv.read_int(); // 0

		if (error == 0)
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::WHITE, "Name change is available.");
		}
		else
		{
			std::string msg;

			switch (error)
			{
			case 1: msg = "A name change has already been submitted."; break;
			case 2: msg = "Must wait at least one month between name changes."; break;
			case 3: msg = "Cannot change name due to a recent ban."; break;
			default: msg = "Name change is not available."; break;
			}

			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, msg);
		}
	}

	void CashShopTransferWorldHandler::handle(InPacket& recv) const
	{
		recv.read_int(); // 0
		int8_t error = recv.read_byte();
		recv.read_int(); // 0
		bool ok = recv.read_bool();

		if (ok)
		{
			int32_t world_count = recv.read_int();

			for (int32_t i = 0; i < world_count; i++)
				recv.read_string(); // world name
		}
		else if (error != 0)
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "World transfer failed (error=" + std::to_string(error) + ").");
		}
	}

	void CashGachaponResultHandler::handle(InPacket& recv) const
	{
		int8_t op = recv.read_byte();

		if (op == (int8_t)0xE4) // Open failed
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "Gachapon failed to open.");
		}
		else if (op == (int8_t)0xE5) // Open success
		{
			recv.read_long(); // box cash id
			int32_t remaining = recv.read_int();

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Gachapon] Opened! Remaining: " + std::to_string(remaining), UIChatBar::LineType::YELLOW);
			// CashItemInfo + reward data follows
		}
	}
}