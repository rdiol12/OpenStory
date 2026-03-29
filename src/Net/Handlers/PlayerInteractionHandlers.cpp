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
#include "PlayerInteractionHandlers.h"

#include "../../IO/UI.h"

#include "../../IO/UITypes/UICharInfo.h"
#include "../../IO/UITypes/UIStatusMessenger.h"

namespace ms
{
	void CharInfoHandler::handle(InPacket& recv) const
	{
		int32_t character_id = recv.read_int();
		int8_t character_level = recv.read_byte();
		int16_t character_job_id = recv.read_short();
		int16_t character_fame = recv.read_short();
		recv.read_byte(); // married (0/1)

		std::string guild_name = recv.read_string();
		std::string alliance_name = recv.read_string();
		recv.read_byte(); // pMedalInfo

		// Pets — sentinel-terminated (byte != 0 means pet follows, byte 0 ends list)
		while (recv.read_byte() != 0)
		{
			recv.read_int();    // petItemId
			recv.read_string(); // petName
			recv.read_byte();   // petLevel
			recv.read_short();  // petTameness
			recv.read_byte();   // petFullness
			recv.read_short();  // unknown (0)
			recv.read_int();    // label ring itemId
		}

		int8_t mount_equipped = recv.read_byte();

		if (mount_equipped != 0)
		{
			recv.read_int(); // mountLevel
			recv.read_int(); // mountExp
			recv.read_int(); // mountFatigue
		}

		int8_t wishlist_count = recv.read_byte();

		for (int8_t w = 0; w < wishlist_count; w++)
			recv.read_int(); // itemId

		recv.read_int(); // monsterBookLevel
		recv.read_int(); // monsterBookNormals
		recv.read_int(); // monsterBookSpecials
		recv.read_int(); // monsterBookTotalCards
		recv.read_int(); // monsterBookCover

		// Medal info
		if (recv.available())
		{
			recv.read_int(); // medal

			int16_t medal_quests_size = recv.read_short();

			for (int16_t s = 0; s < medal_quests_size; s++)
				recv.read_short();
		}

		// Reuse existing active window for same character, otherwise recreate
		auto existing = UI::get().get_element<UICharInfo>();

		if (existing && (existing->get_char_id() != character_id || !existing->is_active()))
		{
			UI::get().remove(UIElement::Type::CHARINFO);
			existing = nullptr;
		}

		if (!existing)
			UI::get().emplace<UICharInfo>(character_id);

		if (auto charinfo = UI::get().get_element<UICharInfo>())
			charinfo->update_stats(character_id, character_job_id, character_level, character_fame, guild_name, alliance_name);
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
				int16_t new_fame = recv.available() ? recv.read_short() : 0;
				if (recv.available())
					recv.read_short(); // padding

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
}