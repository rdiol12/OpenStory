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
#include "FieldHandlers.h"
#include "../../Audio/Audio.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIStatsInfo.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../Character/MapleStat.h"
#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
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
			std::string path = recv.read_string();
			Stage::get().add_effect("Map/Effect.img/" + path);
			break;
		}
		case 1:
		{
			// Boss HP bar / map boss spawn effect
			// No additional data to read for this type
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
			// e.g., "quest/party/clear", "quest/party/wrong_kor"
			std::string path = recv.read_string();
			Stage::get().add_effect(path);
			break;
		}
		case 4:
		{
			// Sound effect — path like "Party1/Clear" -> Sound.nx/Party1.img/Clear
			std::string path = recv.read_string();

			// Split "Category/Name" into img lookup
			size_t slash = path.find('/');
			if (slash != std::string::npos)
			{
				std::string category = path.substr(0, slash);
				std::string name = path.substr(slash + 1);
				nl::node snd = nl::nx::sound[category + ".img"][name];
				if (snd)
					Sound(snd).play();
			}
			break;
		}
		case 6:
		{
			// Music change
			std::string path = recv.read_string();
			Music(path).play();
			break;
		}
		default:
			break;
		}
	}

	void ForcedStatSetHandler::handle(InPacket& recv) const
	{
		// v83: int bitmask of forced stats, then for each set bit: short value
		// (bits 0-9) or byte value (bits 10-11, SPEED/JUMP — see Cosmic's
		// aranGodlyStats payload).
		// Bitmask bits: 0=STR, 1=DEX, 2=INT, 3=LUK, 4=PAD, 5=PDD, 6=MAD, 7=MDD,
		//               8=ACC, 9=EVA, 10=SPEED, 11=JUMP
		if (recv.length() < 4)
			return;

		int32_t mask = recv.read_int();
		CharStats& stats = Stage::get().get_player().get_stats();

		for (int bit = 0; bit < 12 && recv.available(); bit++)
		{
			if (!(mask & (1 << bit)))
				continue;

			// SPEED and JUMP are single unsigned bytes (e.g. 140), everything
			// else is a short.
			int16_t value = (bit >= 10)
				? static_cast<uint8_t>(recv.read_byte())
				: recv.read_short();

			switch (bit)
			{
			case 0: // STR
				stats.set_stat(MapleStat::Id::STR, static_cast<uint16_t>(value));
				break;
			case 1: // DEX
				stats.set_stat(MapleStat::Id::DEX, static_cast<uint16_t>(value));
				break;
			case 2: // INT
				stats.set_stat(MapleStat::Id::INT, static_cast<uint16_t>(value));
				break;
			case 3: // LUK
				stats.set_stat(MapleStat::Id::LUK, static_cast<uint16_t>(value));
				break;
			case 4: // PAD
				stats.set_total(EquipStat::Id::WATK, value);
				break;
			case 5: // PDD
				stats.set_total(EquipStat::Id::WDEF, value);
				break;
			case 6: // MAD
				stats.set_total(EquipStat::Id::MAGIC, value);
				break;
			case 7: // MDD
				stats.set_total(EquipStat::Id::MDEF, value);
				break;
			case 8: // ACC
				stats.set_total(EquipStat::Id::ACC, value);
				break;
			case 9: // EVA
				stats.set_total(EquipStat::Id::AVOID, value);
				break;
			case 10: // SPEED
				stats.set_total(EquipStat::Id::SPEED, value);
				break;
			case 11: // JUMP
				stats.set_total(EquipStat::Id::JUMP, value);
				break;
			}
		}

		// Refresh the stats window so the forced values show immediately.
		if (auto statsinfo = UI::get().get_element<UIStatsInfo>())
			statsinfo->update_all_stats();
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

	void ForcedMapEquipHandler::handle(InPacket& recv) const
	{
		// v83: Empty packet body — server tells client to re-equip based on map restrictions
		// This triggers when entering maps that force specific equipment (PQ maps, event maps)
		// The equip data is already applied server-side via MODIFY_INVENTORY before this packet

		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(Color::Name::WHITE, "Map equipment restrictions applied.");
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

		chat::log("[Ship] Status: " + state_str, chat::LineType::YELLOW);
	}
}
