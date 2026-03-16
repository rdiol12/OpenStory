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

#include "../../IO/UITypes/UILoginNotice.h"

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
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0:
		{
			// Quest forfeit/removed
			if (recv.length() < 2)
				return;

			int16_t questid = recv.read_short();
			Stage::get().get_player().get_quests().forfeit(questid);
			break;
		}
		case 1:
		{
			// Quest started or progress updated
			if (recv.length() < 2)
				return;

			int16_t questid = recv.read_short();

			if (recv.available())
			{
				std::string quest_data = recv.read_string();
				Stage::get().get_player().get_quests().add_started(questid, quest_data);
			}
			break;
		}
		case 2:
		{
			// Quest completed
			if (recv.length() < 2)
				return;

			int16_t questid = recv.read_short();

			if (recv.length() >= 8)
			{
				int64_t time = recv.read_long();
				Stage::get().get_player().get_quests().add_completed(questid, time);
			}
			break;
		}
		default:
			std::cout << "[UpdateQuestInfoHandler]: Unhandled mode: [" << (int)mode << "] remaining bytes: [" << recv.length() << "]" << std::endl;
			break;
		}
	}

	void RelogResponseHandler::handle(InPacket& recv) const
	{
		// v83: byte success (1 = ok to relog)
		if (!recv.available())
			return;

		int8_t success = recv.read_byte();
		std::cout << "[RelogResponseHandler]: success: [" << (int)success << "]" << std::endl;
	}

	void FameResponseHandler::handle(InPacket& recv) const
	{
		// v83: byte mode
		// mode 0: fame change response - byte type (increase/decrease result)
		// mode 1: gave fame notification - string charname, byte type
		// mode 5: received fame notification - string charname, byte type
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0:
		{
			// Response to fame give attempt
			if (recv.available())
			{
				std::string charname = recv.read_string();
				int8_t type = recv.read_byte();
				int32_t new_fame = recv.available() ? recv.read_int() : 0;
				std::cout << "[FameResponseHandler]: Gave fame to [" << charname << "] type: [" << (int)type << "] new fame: [" << new_fame << "]" << std::endl;
			}
			break;
		}
		case 5:
		{
			// Already famed this person today / too low level / etc
			std::cout << "[FameResponseHandler]: Fame error, mode 5" << std::endl;
			break;
		}
		default:
			std::cout << "[FameResponseHandler]: mode: [" << (int)mode << "] remaining bytes: [" << recv.length() << "]" << std::endl;
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

			for (int8_t i = 0; i < count; i++)
			{
				if (recv.length() < 4)
					break;

				int32_t buddy_cid = recv.read_int();
				std::string buddy_name = recv.read_padded_string(13);
				int8_t buddy_status = recv.read_byte();
				int32_t buddy_channel = recv.read_int();
				std::string buddy_group = recv.read_padded_string(17);

				std::cout << "  Buddy: [" << buddy_name << "] cid: [" << buddy_cid << "] status: [" << (int)buddy_status << "] ch: [" << buddy_channel << "]" << std::endl;
			}
			break;
		}
		case 14:
		{
			// Buddy capacity
			if (recv.available())
			{
				int8_t capacity = recv.read_byte();
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
		// v83 family packet - log for debugging
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();
		std::cout << "[FamilyHandler]: mode: [" << (int)mode << "] remaining bytes: [" << recv.length() << "]" << std::endl;
	}

	void PartyOperationHandler::handle(InPacket& recv) const
	{
		// v83: byte operation
		// 7 = party created/updated (full party info)
		// 15 = party disbanded/left
		// others = various party updates
		if (!recv.available())
			return;

		int8_t operation = recv.read_byte();

		switch (operation)
		{
		case 7:
		{
			// Full party info update
			if (recv.length() < 4)
				break;

			int32_t partyid = recv.read_int();
			std::cout << "[PartyOperationHandler]: Party update, id: [" << partyid << "] remaining bytes: [" << recv.length() << "]" << std::endl;

			// v83 party packet contains up to 6 members with:
			// 6x int cid, 6x padded_string(13) name, 6x int job, 6x int level,
			// 6x int channel, 6x int mapid, int leader_cid, ...
			// For now log the data, full parsing needs a Party data structure
			for (int i = 0; i < 6; i++)
			{
				if (recv.length() < 4)
					break;
				int32_t member_cid = recv.read_int();
				if (member_cid != 0)
					std::cout << "  Party member cid: [" << member_cid << "]" << std::endl;
			}
			break;
		}
		case 12:
		{
			// Invite
			if (recv.length() < 4)
				break;

			int32_t from_cid = recv.read_int();
			std::string from_name = recv.read_string();
			std::cout << "[PartyOperationHandler]: Party invite from [" << from_name << "] cid: [" << from_cid << "]" << std::endl;
			break;
		}
		case 15:
		{
			// Party disbanded or member left
			int32_t partyid = recv.read_int();
			int32_t target_cid = recv.read_int();
			std::cout << "[PartyOperationHandler]: Member left/expelled, party: [" << partyid << "] cid: [" << target_cid << "]" << std::endl;
			break;
		}
		default:
			std::cout << "[PartyOperationHandler]: operation: [" << (int)operation << "] remaining bytes: [" << recv.length() << "]" << std::endl;
			break;
		}
	}

	void ClockHandler::handle(InPacket& recv) const
	{
		// v83: byte type
		// type 1: map clock - byte hour, byte min, byte sec
		// type 2: countdown timer - int seconds
		// type 3: real world clock (wedding, etc)
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		switch (type)
		{
		case 1:
		{
			// Map world clock
			if (recv.length() < 3)
				break;

			int8_t hour = recv.read_byte();
			int8_t min = recv.read_byte();
			int8_t sec = recv.read_byte();
			std::cout << "[ClockHandler]: World clock: " << (int)hour << ":" << (int)min << ":" << (int)sec << std::endl;
			break;
		}
		case 2:
		{
			// Countdown timer
			if (recv.length() < 4)
				break;

			int32_t seconds = recv.read_int();
			std::cout << "[ClockHandler]: Countdown timer: " << seconds << " seconds" << std::endl;
			break;
		}
		default:
			std::cout << "[ClockHandler]: type: [" << (int)type << "] remaining bytes: [" << recv.length() << "]" << std::endl;
			break;
		}
	}

	void ForcedStatSetHandler::handle(InPacket& recv) const
	{
		// v83: int bitmask of forced stats, then for each set bit: short value
		if (recv.length() < 4)
		{
			std::cout << "[ForcedStatSetHandler]: remaining bytes: [" << recv.length() << "]" << std::endl;
			return;
		}

		int32_t mask = recv.read_int();
		std::cout << "[ForcedStatSetHandler]: mask: [" << mask << "] remaining bytes: [" << recv.length() << "]" << std::endl;
	}

	void SetTractionHandler::handle(InPacket& recv) const
	{
		// v83: byte value (ice/slippery floor traction modifier)
		if (!recv.available())
			return;

		// Traction percentage (0-100)
		int8_t traction = recv.read_byte();
		std::cout << "[SetTractionHandler]: traction: [" << (int)traction << "]" << std::endl;
	}

	void NpcActionHandler::handle(InPacket& recv) const
	{
		// v83 NPC_ACTION: int oid, byte[] action data
		if (recv.length() < 4)
			return;

		int32_t oid = recv.read_int();

		// Remaining bytes are the NPC's action/animation data
		// Try to get the NPC and update its stance
		if (recv.available())
		{
			int8_t action = recv.read_byte();

			// Look up the NPC on the map
			MapObjects* npcs = Stage::get().get_npcs().get_npcs();
			if (npcs)
			{
				Optional<MapObject> obj = npcs->get(oid);
				if (obj)
				{
					Npc* npc = static_cast<Npc*>(obj.get());

					// Action byte determines the stance
					// Common: 0 = stand, 1 = move, etc.
					if (action == 1)
						npc->set_stance("move");
					else
						npc->set_stance("stand");
				}
			}
		}
	}

	void YellowTipHandler::handle(InPacket& recv) const
	{
		// v83: byte type, string message
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		if (recv.available())
		{
			std::string message = recv.read_string();
			std::cout << "[YellowTipHandler]: type: [" << (int)type << "] message: [" << message << "]" << std::endl;
		}
		else
		{
			std::cout << "[YellowTipHandler]: type: [" << (int)type << "] no message" << std::endl;
		}
	}

	void CatchMonsterHandler::handle(InPacket& recv) const
	{
		// v83: int moboid, byte itemid_related
		if (recv.length() < 4)
			return;

		int32_t mob_oid = recv.read_int();
		int8_t result = recv.available() ? recv.read_byte() : 0;
		std::cout << "[CatchMonsterHandler]: mob_oid: [" << mob_oid << "] result: [" << (int)result << "]" << std::endl;
	}

	void BlowWeatherHandler::handle(InPacket& recv) const
	{
		// v83 BLOW_WEATHER: byte active, int itemid, string message
		// Controls map weather effects (snow, rain, etc.)
		if (!recv.available())
			return;

		int8_t active = recv.read_byte();

		if (active != 0 && recv.length() >= 4)
		{
			int32_t itemid = recv.read_int();
			std::string message = recv.available() ? recv.read_string() : "";
			std::cout << "[BlowWeatherHandler]: Weather ON, itemid: [" << itemid << "] message: [" << message << "]" << std::endl;
		}
		else
		{
			std::cout << "[BlowWeatherHandler]: Weather OFF" << std::endl;
		}
	}
}