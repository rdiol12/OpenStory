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
#include "MessagingHandlers.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Character/QuestLog.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"

#include "../../IO/UITypes/UIChatBar.h"
#include "../../IO/UITypes/UIQuestHelper.h"
#include "../../IO/UITypes/UIQuestLog.h"
#include "../../IO/UITypes/UIStatusMessenger.h"
#include "../../IO/UITypes/UIWhisper.h"

namespace ms
{
	// Modes:
	// 0 - Item(0) or Meso(1) 
	// 3 - Exp gain
	// 4 - Fame
	// 5 - Mesos
	// 6 - Guild points
	void ShowStatusInfoHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 0)
		{
			int8_t mode2 = recv.read_byte();

			if (mode2 == -1)
			{
				show_status(Color::Name::WHITE, "You can't get anymore items.");
			}
			else if (mode2 == 0)
			{
				int32_t itemid = recv.read_int();
				int32_t qty = recv.read_int();

				const ItemData& idata = ItemData::get(itemid);

				if (!idata.is_valid())
					return;

				std::string name = idata.get_name();

				if (name.length() > 21)
				{
					name.substr(0, 21);
					name += "..";
				}

				InventoryType::Id type = InventoryType::by_item_id(itemid);

				std::string tab = "";

				switch (type)
				{
				case InventoryType::Id::EQUIP:
					tab = "Eqp";
					break;
				case InventoryType::Id::USE:
					tab = "Use";
					break;
				case InventoryType::Id::SETUP:
					tab = "Setup";
					break;
				case InventoryType::Id::ETC:
					tab = "Etc";
					break;
				case InventoryType::Id::CASH:
					tab = "Cash";
					break;
				default:
					tab = "UNKNOWN";
					break;
				}

				if (qty < -1)
					show_status(Color::Name::WHITE, "You have lost items in the " + tab + " tab (" + name + " " + std::to_string(qty) + ")");
				else if (qty == -1)
					show_status(Color::Name::WHITE, "You have lost an item in the " + tab + " tab (" + name + ")");
				else if (qty == 1)
					show_status(Color::Name::WHITE, "You have gained an item in the " + tab + " tab (" + name + ")");
				else
					show_status(Color::Name::WHITE, "You have gained items in the " + tab + " tab (" + name + " " + std::to_string(qty) + ")");
			}
			else if (mode2 == 1)
			{
				recv.skip(1);

				int32_t gain = recv.read_int();
				std::string sign = (gain < 0) ? "-" : "+";

				show_status(Color::Name::WHITE, "You have gained mesos (" + sign + std::to_string(gain) + ")");
			}
			else
			{
				show_status(Color::Name::RED, "Mode: 0, Mode 2: " + std::to_string(mode2) + " is not handled.");
			}
		}
		else if (mode == 1)
		{
			int16_t qid = recv.read_short();
			int8_t status = recv.read_byte();

			QuestLog& quests = Stage::get().get_player().get_quests();

			if (status == 0)
			{
				// Quest forfeited / removed
				quests.forfeit(qid);
				show_status(Color::Name::WHITE, "Quest forfeited.");
			}
			else if (status == 1)
			{
				std::string qdata = recv.read_string();
				// Skip 5 trailing bytes (Cosmic sends 5 zero bytes after progress)
				if (recv.available() && recv.length() >= 5)
					recv.skip(5);
				quests.add_started(qid, qdata);
				show_status(Color::Name::WHITE, "Quest updated.");
			}
			else if (status == 2)
			{
				int64_t time = recv.read_long();
				quests.add_completed(qid, time);
				show_status(Color::Name::WHITE, "Quest completed!");
				Sound(Sound::Name::QUESTCOMPLETE).play();
			}

			// Refresh NPC quest marks after any quest state change
			Stage::get().get_npcs().refresh_quest_marks();

			// Refresh quest log UI if open
			if (auto questlog_ui = UI::get().get_element<UIQuestLog>())
				questlog_ui->load_quests();

			// Auto-track newly started quest in quest helper
			if (status == 1)
			{
				if (auto helper = UI::get().get_element<UIQuestHelper>())
					helper->track_quest(qid);
			}
		}
		else if (mode == 3)
		{
			bool white = recv.read_bool();
			int32_t gain = recv.read_int();
			bool inchat = recv.read_bool();
			int32_t bonus1 = recv.read_int();

			recv.read_short();
			recv.read_int();	// bonus 2
			recv.read_bool();	// 'event or party'
			recv.read_int();	// bonus 3
			recv.read_int();	// bonus 4
			recv.read_int();	// bonus 5

			std::string message = "You have gained experience (+" + std::to_string(gain) + ")";

			show_status(white ? Color::Name::WHITE : Color::Name::YELLOW, message);

			if (bonus1 > 0)
				show_status(Color::Name::YELLOW, "+ Bonus EXP (+" + std::to_string(bonus1) + ")");
		}
		else if (mode == 4)
		{
			int32_t gain = recv.read_int();
			std::string sign = (gain < 0) ? "-" : "+";

			// Negative gain values indicate fame loss
			if (gain < 0)
				show_status(Color::Name::WHITE, "You have lost fame. (" + sign + std::to_string(-gain) + ")");
			else
				show_status(Color::Name::WHITE, "You have gained fame. (" + sign + std::to_string(gain) + ")");
		}
		else
		{
			show_status(Color::Name::RED, "Mode: " + std::to_string(mode) + " is not handled.");
		}
	}

	void ShowStatusInfoHandler::show_status(Color::Name color, const std::string& message) const
	{
		if (auto messenger = UI::get().get_element<UIStatusMessenger>())
			messenger->show_status(color, message);
	}

	void ServerMessageHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte();
		bool servermessage = recv.inspect_bool();

		if (servermessage)
			recv.skip(1);

		std::string message = recv.read_string();

		auto chatbar = UI::get().get_element<UIChatBar>();

		switch (type)
		{
		case 0: // Notice (blue text in chat)
			if (chatbar)
				chatbar->send_chatline("[Notice] " + message, UIChatBar::LineType::BLUE);
			break;
		case 1: // Popup notice (server-side popup, also show in chat)
			if (chatbar)
				chatbar->send_chatline("[Notice] " + message, UIChatBar::LineType::YELLOW);
			break;
		case 2: // Megaphone (visible only on current map)
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::BLUE);
			break;
		case 3: // Super Megaphone (visible on all channels)
		{
			recv.read_byte(); // channel
			recv.read_bool(); // megaphone ear (whisper icon)

			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
			break;
		}
		case 4: // Scroll notice (top of screen ticker)
			UI::get().set_scrollnotice(message);
			break;
		case 5: // Pink text server message
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::PINK);
			break;
		case 6: // Light blue text server message
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::LIGHTBLUE);
			break;
		case 7: // NPC broadcast (item megaphone with NPC)
			recv.read_int(); // npcid
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
			break;
		case 8: // Item megaphone
		{
			recv.read_byte(); // channel
			recv.read_bool(); // megaphone ear (whisper icon)

			// Item data follows (1 byte hasItem, then item info if present)
			if (recv.available())
			{
				bool has_item = recv.read_bool();
				if (has_item && recv.available())
					recv.skip(recv.length()); // skip item display data
			}

			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::GREEN);
			break;
		}
		case 10: // Triple megaphone (multi-line)
		{
			// Message already read above; there may be additional lines
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
			break;
		}
		default:
			if (chatbar)
				chatbar->send_chatline(message, UIChatBar::LineType::YELLOW);
			break;
		}
	}

	void WeekEventMessageHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // Always 0xFF (padding byte)

		std::string message = recv.read_string();

		static const std::string MAPLETIP = "[MapleTip]";

		if (message.substr(0, MAPLETIP.length()).compare("[MapleTip]"))
			message = "[Notice] " + message;

		UI::get().get_element<UIChatBar>()->send_chatline(message, UIChatBar::LineType::YELLOW);
	}

	void ChatReceivedHandler::handle(InPacket& recv) const
	{
		int32_t charid = recv.read_int();
		bool gm = recv.read_bool();

		std::string message = recv.read_string();
		int8_t type = recv.read_byte();

		if (auto character = Stage::get().get_character(charid))
		{
			message = character->get_name() + ": " + message;
			character->speak(message);
		}

		auto linetype = gm ? UIChatBar::LineType::WHITE : static_cast<UIChatBar::LineType>(type);

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(message, linetype);
	}

	void ScrollResultHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		bool success = recv.read_bool();
		bool destroyed = recv.read_bool();

		recv.read_short(); // Legendary spirit if 1

		CharEffect::Id effect;
		Messages::Type message;

		if (success)
		{
			effect = CharEffect::Id::SCROLL_SUCCESS;
			message = Messages::Type::SCROLL_SUCCESS;
		}
		else
		{
			effect = CharEffect::Id::SCROLL_FAILURE;

			if (destroyed)
				message = Messages::Type::SCROLL_DESTROYED;
			else
				message = Messages::Type::SCROLL_FAILURE;
		}

		Stage::get().show_character_effect(cid, effect);

		if (Stage::get().is_player(cid))
		{
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->display_message(message, UIChatBar::LineType::RED);

			UI::get().enable();
		}
	}

	void ShowItemGainInChatHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		switch (mode)
		{
		case 0: // Level up
			break;
		case 1: // Skill/buff use effect
		{
			if (recv.length() < 4)
				return;

			int32_t skillid = recv.read_int();
			Stage::get().get_combat().show_player_buff(skillid);
			break;
		}
		case 3: // Item gain in chat
		{
			int8_t mode2 = recv.read_byte();

			if (mode2 == 1)
			{
				int32_t itemid = recv.read_int();
				int32_t qty = recv.read_int();

				const ItemData& idata = ItemData::get(itemid);

				if (!idata.is_valid())
					return;

				std::string name = idata.get_name();
				std::string sign = (qty < 0) ? "-" : "+";
				std::string message = "Gained an item: " + name + " (" + sign + std::to_string(qty) + ")";

				if (auto chatbar = UI::get().get_element<UIChatBar>())
					chatbar->send_chatline(message, UIChatBar::LineType::BLUE);
			}

			break;
		}
		case 4: // Pet level up
			recv.read_byte(); // 0
			recv.read_byte(); // pet index
			break;
		case 6: // Safety charm (exp not lost)
		case 7: // Enter portal sound
		case 8: // Job change
		case 9: // Quest complete
			break;
		case 10: // Recovery
			recv.read_byte(); // heal amount
			break;
		case 11: // Buff effect (no data)
			break;
		case 13: // Monster book card
			Stage::get().get_player().show_effect_id(CharEffect::Id::MONSTER_CARD);
			break;
		case 14: // Monster book pickup
		case 15: // Equipment level up
			break;
		case 16: // Maker skill result
			recv.read_int(); // 0=success, 1=fail
			break;
		case 17: // Buff effect with SFX
			recv.read_string(); // path
			recv.read_int();
			break;
		case 18: // Show intro
			recv.read_string(); // path
			break;
		case 21: // Wheel of Destiny
			recv.read_byte(); // wheels left
			break;
		case 23: // Show info
			recv.read_string(); // path
			recv.read_int();
			break;
		default:
			break;
		}
	}

	void WhisperHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();

		if (mode == 9 || mode == 18) // Whisper received
		{
			std::string from = recv.read_string();
			recv.read_byte(); // channel
			bool from_admin = recv.read_bool();
			std::string message = recv.read_string();

			std::string line = from + " <Whisper>: " + message;

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(line, from_admin ? UIChatBar::LineType::WHITE : UIChatBar::LineType::RED);

			if (auto whisper = UI::get().get_element<UIWhisper>())
				whisper->recv_whisper(from, message, from_admin);
		}
		else if (mode == 10) // Find reply — "X is on channel Y"
		{
			std::string name = recv.read_string();
			int8_t result = recv.read_byte();

			std::string line;

			if (result == -1)
				line = name + " is not online.";
			else if (result == 0)
				line = name + " is in the same channel but on a different map.";
			else
				line = name + " is on Channel " + std::to_string(result) + ".";

			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_chatline(line, UIChatBar::LineType::YELLOW);
		}
	}

	void MultichatHandler::handle(InPacket& recv) const
	{
		int8_t type = recv.read_byte(); // 0 = buddy, 1 = party, 2 = guild, 3 = alliance
		std::string from = recv.read_string();
		std::string message = recv.read_string();

		std::string prefix;

		switch (type)
		{
		case 0: prefix = "[Buddy] "; break;
		case 1: prefix = "[Party] "; break;
		case 2: prefix = "[Guild] "; break;
		case 3: prefix = "[Alliance] "; break;
		default: prefix = "[Chat] "; break;
		}

		std::string line = prefix + from + ": " + message;

		if (auto chatbar = UI::get().get_element<UIChatBar>())
			chatbar->send_chatline(line, UIChatBar::LineType::BLUE);
	}
}