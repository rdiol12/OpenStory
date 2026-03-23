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
	// Opcode: QUEST_ACTION(107)
	// Action 1: Start quest
	class StartQuestPacket : public OutPacket
	{
	public:
		StartQuestPacket(int16_t questid, int32_t npcid) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(1);
			write_short(questid);
			write_int(npcid);
		}
	};

	// Action 2: Complete quest
	class CompleteQuestPacket : public OutPacket
	{
	public:
		CompleteQuestPacket(int16_t questid, int32_t npcid, int16_t selection = -1) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(2);
			write_short(questid);
			write_int(npcid);

			if (selection >= 0)
				write_short(selection);
		}
	};

	// Action 3: Forfeit quest
	class ForfeitQuestPacket : public OutPacket
	{
	public:
		ForfeitQuestPacket(int16_t questid) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(3);
			write_short(questid);
		}
	};

	// Action 0: Restore lost item
	class RestoreLostItemPacket : public OutPacket
	{
	public:
		RestoreLostItemPacket(int16_t questid, int32_t itemid) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(0);
			write_short(questid);
			write_int(0);
			write_int(itemid);
		}
	};

	// Action 4: Scripted quest start
	class ScriptedStartQuestPacket : public OutPacket
	{
	public:
		ScriptedStartQuestPacket(int16_t questid, int32_t npcid) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(4);
			write_short(questid);
			write_int(npcid);
		}
	};

	// Action 5: Scripted quest end
	class ScriptedEndQuestPacket : public OutPacket
	{
	public:
		ScriptedEndQuestPacket(int16_t questid, int32_t npcid) : OutPacket(OutPacket::Opcode::QUEST_ACTION)
		{
			write_byte(5);
			write_short(questid);
			write_int(npcid);
		}
	};
}
