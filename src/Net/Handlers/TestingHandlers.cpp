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

#include "../../Audio/Audio.h"
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
#include "../../IO/UITypes/UIStatusBar.h"
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
#include "../../IO/UITypes/UIEquipInventory.h"
#include "../../IO/UITypes/UIStatsInfo.h"
#include "../../IO/UITypes/UISkillBook.h"
#include "../../IO/UITypes/UIBuddyList.h"
#include "../../IO/UITypes/UIWorldMap.h"
#include "../../IO/UITypes/UIKeyConfig.h"
#include "../../IO/UITypes/UIMonsterBook.h"
#include "../../IO/UITypes/UIComboCounter.h"
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
			// Unknown SPW reason

		UI::get().enable();
	}

	void FieldEffectHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		switch (type)
		{
		case 0:
		{
			// Summon message — map-specific event string
			// e.g., "Eos Tower" summon messages
			std::string path = recv.read_string();
			Stage::get().add_effect("Map/Effect.img/" + path);
			break;
		}
		case 1:
		{
			// Boss HP bar / map boss spawn effect
			// Cosmic: MapEffect.MapBossHPBar
			Stage::get().add_effect("Map/Effect.img/quest/party/clear");
			break;
		}
		case 2:
		{
			// Map effect with WZ path
			std::string path = recv.read_string();
			Stage::get().add_effect("Map/Effect.img/" + path);
			break;
		}
		case 3:
		{
			// Screen effect (full path provided)
			std::string path = recv.read_string();
			Stage::get().add_effect(path);
			break;
		}
		case 4:
		{
			// Sound effect — path to Sound.wz entry
			// We don't have a sound system hooked up, so just consume
			std::string path = recv.read_string();
			break;
		}
		default:
			break;
		}
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
			(void)questid;
			break;
		}
		case 8:
		{
			// Quest NPC info (sent after quest start/complete)
			int16_t questid = recv.read_short();
			int32_t npcid = recv.read_int();
			int32_t zero = recv.read_int();
			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			break;
		}
		case 0x0A:
		{
			// Quest error — requirements not met
			int16_t questid = recv.read_short();
			(void)questid;

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Quest] You don't meet the requirements for this quest.", UIChatBar::LineType::RED);

			break;
		}
		case 0x0B:
		{
			// Quest failure — not enough mesos
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Quest] You don't have enough mesos.", UIChatBar::LineType::RED);

			break;
		}
		case 0x0D:
		{
			// Quest failure — equipment is currently worn
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Quest] Item is currently worn by character.", UIChatBar::LineType::RED);

			break;
		}
		case 0x0E:
		{
			// Quest failure — missing required item
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Quest] You don't have the required item.", UIChatBar::LineType::RED);

			break;
		}
		case 0x0F:
		{
			// Quest expired
			int16_t questid = recv.read_short();
			Stage::get().get_player().get_quests().forfeit(questid);

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Quest] The quest has expired.", UIChatBar::LineType::RED);

			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			// Refresh NPC quest marks
			Stage::get().get_npcs().refresh_quest_marks();

			break;
		}
		default:
			// Unhandled quest info mode
			break;
		}
	}

	void QuestClearHandler::handle(InPacket& recv) const
	{
		// Cosmic sends: SHORT questId — visual quest completion notification
		if (recv.length() < 2)
			return;

		int16_t questid = recv.read_short();
		(void)questid;

		// Play quest clear effect on player (light pillar)
		Stage::get().get_player().show_effect_id(CharEffect::Id::QUEST_CLEAR);

		// Play quest complete sound
		Sound(Sound::Name::QUESTCOMPLETE).play();

		// Show notice button alert
		if (auto statusbar = UI::get().get_element<UIStatusBar>())
			statusbar->notify();

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
			// Error: can't raise/lower own fame
			if (messenger)
				messenger->show_status(Color::Name::RED, "You can't raise or lower your own fame.");
			break;
		}
		case 2:
		{
			// Error: already famed this person today
			if (messenger)
				messenger->show_status(Color::Name::RED, "You have already given fame to this character today.");
			break;
		}
		case 3:
		{
			// Error: target level too low
			if (messenger)
				messenger->show_status(Color::Name::RED, "That character's level is too low.");
			break;
		}
		case 5:
		{
			// Error: too low level to fame
			if (messenger)
				messenger->show_status(Color::Name::RED, "You must be at least level 15 to give fame.");
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
		if (!recv.available())
			return;

		int8_t type = recv.read_byte();

		switch (type)
		{
		case 1: // Game clock — byte hour, byte min, byte sec
		{
			if (recv.length() < 3)
				break;

			int8_t hour = recv.read_byte();
			int8_t min = recv.read_byte();
			int8_t sec = recv.read_byte();

			Stage::get().set_clock(hour, min, sec);
			UI::get().emplace<UIClock>();
			break;
		}
		case 2: // Countdown timer — int secondsRemaining
		{
			if (recv.length() < 4)
				break;

			int32_t seconds = recv.read_int();

			Stage::get().set_countdown(seconds);
			UI::get().emplace<UIClock>();
			break;
		}
		case 3: // Remove timer
		{
			Stage::get().clear_clock();
			break;
		}
		default:
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
		if (!recv.available())
			return;

		std::string message = recv.read_string();

		// Show yellow text notification at bottom of screen (like quest updates)
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
	}

	void BroadcastMsgHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();
		std::string message = recv.read_string();

		// GM notices, event announcements
		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
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
		if (!recv.available())
			return;

		int8_t type = recv.read_byte(); // 0=no weather, 1=snow, 2=rain
		int32_t itemid = recv.read_int(); // 0 if none, or cash weather item
		std::string message;

		if (recv.available())
			message = recv.read_string();

		if (type == 0 || itemid == 0)
		{
			Stage::get().clear_weather();
			return;
		}

		std::string id_str = "0" + std::to_string(itemid);
		nl::node item_node = nl::nx::item["Cash"]["0512.img"][id_str];
		std::string path = item_node["info"]["path"];

		if (!path.empty())
		{
			std::string resolve_path = path;

			if (resolve_path.substr(0, 4) == "Map/")
				resolve_path = resolve_path.substr(4);

			Stage::get().set_weather(resolve_path, message);
		}

		if (!message.empty())
			UI::get().set_scrollnotice(message);
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

	void SetGenderHandler::handle(InPacket& recv) const
	{
		// Gender confirmation from server after gender selection
		// Just consume — gender is already set in CharStats from login
		if (recv.available())
			recv.read_byte(); // gender (0=male, 1=female)
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
	void OpenUIHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t ui_type = recv.read_byte();

		Player& player = Stage::get().get_player();

		// v83 OpenUI types — server requests client to open a specific window
		switch (ui_type)
		{
		case 0x00: // Equipment inventory
			UI::get().emplace<UIEquipInventory>(player.get_inventory());
			break;
		case 0x01: // Stats
			UI::get().emplace<UIStatsInfo>(player.get_stats());
			break;
		case 0x02: // Skill book
			UI::get().emplace<UISkillBook>(player.get_stats(), player.get_skills());
			break;
		case 0x03: // Buddy list
			UI::get().emplace<UIBuddyList>();
			break;
		case 0x04: // World map
			UI::get().emplace<UIWorldMap>();
			break;
		case 0x05: // Messenger
			UI::get().emplace<UIMessenger>();
			break;
		case 0x09: // Quest log
			UI::get().emplace<UIQuestLog>(player.get_quests());
			break;
		case 0x0A: // Key config
			UI::get().emplace<UIKeyConfig>(player.get_inventory(), player.get_skills());
			break;
		case 0x12: // Monster book
			UI::get().emplace<UIMonsterBook>();
			break;
		case 0x16: // Party search
			UI::get().emplace<UIPartySearch>();
			break;
		default:
			break;
		}
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
		if (!recv.available())
			return;

		bool spawn = recv.read_bool();

		// Beginner guide NPC — show helpful message when spawned
		if (spawn)
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline("[Guide] Maple Guide has appeared! Press the guide key for help.", UIChatBar::LineType::YELLOW);
		}
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

			(void)hint_id;
			(void)duration;
		}
	}

	void ShowComboHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int32_t count = recv.read_int();

		Stage::get().set_combo(count);

		if (count > 0)
		{
			if (!UI::get().get_element<UIComboCounter>())
				UI::get().emplace<UIComboCounter>();
		}
	}

	void ForcedMapEquipHandler::handle(InPacket& recv) const
	{
		// v83: Empty packet body — server tells client to re-equip based on map restrictions
		// This triggers when entering maps that force specific equipment (PQ maps, event maps)
		// The equip data is already applied server-side via MODIFY_INVENTORY before this packet

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::WHITE, "Map equipment restrictions applied.");
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
		if (!recv.available())
			return;

		int8_t event_type = recv.read_byte(); // 10 = boat/crog
		int8_t move_type = recv.read_byte();  // 4 = start, 5 = stop

		// The ship is a real map — player is already on it via SET_FIELD.
		// ContiMove just signals departure/arrival for the map's scrolling backgrounds.
		if (auto chatbar = UI::get().get_element<UIChatBar>())
		{
			if (move_type == 4)
				chatbar->send_chatline("The ship has departed.", UIChatBar::LineType::YELLOW);
			else if (move_type == 5)
				chatbar->send_chatline("The ship has arrived at its destination.", UIChatBar::LineType::YELLOW);
		}
	}

	void ContiStateHandler::handle(InPacket& recv) const
	{
		if (!recv.available())
			return;

		int8_t state = recv.read_byte(); // 0 = waiting, 1 = in transit, 2 = arrived
		if (recv.available())
			recv.read_byte(); // padding

		std::string state_str;
		switch (state)
		{
		case 0: state_str = "Waiting"; break;
		case 1: state_str = "In Transit"; break;
		case 2: state_str = "Arrived"; break;
		default: state_str = "Unknown (" + std::to_string(state) + ")"; break;
		}

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline("[Ship] Status: " + state_str, UIChatBar::LineType::YELLOW);
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
	void StopClockHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 0

		Stage::get().clear_clock();
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
		// v83: string path — sets the background effect for the map
		// Used for boss arenas, special event maps, etc.
		if (!recv.available())
			return;

		std::string path = recv.read_string();

		if (!path.empty())
			Stage::get().add_effect(path);
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
			std::string world_list = "[CashShop] Available worlds: ";

			for (int32_t i = 0; i < world_count; i++)
			{
				std::string world_name = recv.read_string();

				if (i > 0)
					world_list += ", ";

				world_list += world_name;
			}

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(world_list, UIChatBar::LineType::YELLOW);
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