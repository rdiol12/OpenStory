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
#include "MiscHandlers.h"
#include "../../Gameplay/Stage.h"
#include "../../Gameplay/MapleMap/Npc.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../Configuration.h"
#include "../Packets/NpcInteractionPackets.h"

#include <vector>

namespace ms
{
	void NpcActionHandler::handle(InPacket& recv) const
	{
		// v83 NPC_ACTION: int oid, then remaining bytes are action data.
		// The server periodically sends this to animate NPCs on the map.
		if (recv.length() < 4)
			return;

		int32_t oid = recv.read_int();

		// Capture every action byte the server sent — we mirror the
		// payload byte-for-byte in the outbound NPC_ACTION echo so
		// the wire-level handshake matches v83 GMS exactly.
		std::vector<int8_t> action_bytes;
		action_bytes.reserve(recv.length());
		while (recv.available())
			action_bytes.push_back(recv.read_byte());

		// Apply the first action byte (if any) to the NPC's local
		// stance. Subsequent bytes carry positional data we don't
		// render but still echo back unchanged.
		if (auto npcs = Stage::get().get_npcs().get_npcs())
		{
			if (auto obj = npcs->get(oid))
			{
				auto* npc = static_cast<Npc*>(obj.get());
				if (!action_bytes.empty())
				{
					switch (action_bytes[0])
					{
					case 0: npc->set_stance("stand"); break;
					case 1: npc->set_stance("move");  break;
					case 2: npc->set_stance("say");   break;
					default: npc->set_stance("stand"); break;
					}
				}
			}
		}

		// Echo of inbound NPC_ACTION bytes is intentionally NOT sent.
		// Cosmic / HeavenMS ignore the outbound NPC_ACTION packet
		// entirely, and dispatching one per server animation tick was
		// flooding the outbound socket — the server saw a Connection
		// reset shortly after each NPC was clicked. The wire-parity
		// note in Packets/NpcInteractionPackets.h kept the packet
		// class around for future use; we just don't emit it.
	}

	void YellowTipHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		std::string message = recv.read_string();

		// Show yellow text notification at bottom of screen (like quest updates)
		chat::log(message, chat::LineType::YELLOW);
	}

	void BroadcastMsgHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();
		std::string message = recv.read_string();

		// GM notices, event announcements
		chat::log(message, chat::LineType::YELLOW);
	}

	void AdminResultHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		if (recv.available())
		{
			int8_t mode = recv.read_byte();

			// type 0x10 = GM hide toggle, mode 1 = hidden, mode 0 = visible
			if (type == 0x10)
				Stage::get().get_player().set_hidden(mode == 1);
		}
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

	void QuickSlotInitHandler::handle(InPacket& recv) const
	{
		if (recv.available())
		{
			bool init = recv.read_bool();

			if (init)
			{
				for (int i = 0; i < 8; i++)
				{
					if (recv.available())
						recv.read_int();
				}
			}
		}
	}

	void ClaimStatusChangedHandler::handle(InPacket& recv) const
	{
		// Enable/disable report button — just consume, no UI action needed
		if (recv.available())
			recv.read_byte(); // 1 = enabled
	}

	void SueCharacterResultHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t mode = recv.read_byte();

		std::string message;
		switch (mode)
		{
		case 0: message = "You have successfully reported the user."; break;
		case 1: message = "Unable to locate the user."; break;
		case 2: message = "You may only report users 10 times a day."; break;
		case 3: message = "You have been reported to the GMs by a user."; break;
		default: message = "Your report request did not go through."; break;
		}

		UI::get().emplace<UIOk>(message, [](bool) {});
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

		chat::log(line, chat::LineType::RED);
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
		if (!recv.available())
			return;

		bool enabled = recv.read_bool();

		Configuration::get().set_extra_pendant_slot(enabled);

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
		{
			if (enabled)
				messenger->show_status(Color::Name::WHITE, "Extra pendant slot has been activated.");
			else
				messenger->show_status(Color::Name::WHITE, "Extra pendant slot has been deactivated.");
		}
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

	void NewYearCardHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		// Complex multi-mode packet — card data, errors, broadcasts
		chat::log("[NewYearCard] Card update (mode=" + std::to_string(mode) + ")", chat::LineType::YELLOW);
	}
}
