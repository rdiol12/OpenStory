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

#include "../../Audio/Audio.h"
#include "../../Character/Inventory/Inventory.h"

namespace ms
{
	// Packet which requests that the inventory is sorted
	// Opcode: GATHER_ITEMS(69)
	class GatherItemsPacket : public OutPacket
	{
	public:
		GatherItemsPacket(InventoryType::Id type) : OutPacket(OutPacket::Opcode::GATHER_ITEMS)
		{
			write_time();
			write_byte(type);
		}
	};

	// Packet which requests that the inventory is sorted
	// Opcode: SORT_ITEMS(70)
	class SortItemsPacket : public OutPacket
	{
	public:
		SortItemsPacket(InventoryType::Id type) : OutPacket(OutPacket::Opcode::SORT_ITEMS)
		{
			write_time();
			write_byte(type);
		}
	};

	// Packet which requests that an item is moved
	// Opcode: MOVE_ITEM(71)
	class MoveItemPacket : public OutPacket
	{
	public:
		MoveItemPacket(InventoryType::Id type, int16_t slot, int16_t action, int16_t qty) : OutPacket(OutPacket::Opcode::MOVE_ITEM)
		{
			write_time();
			write_byte(type);
			write_short(slot);
			write_short(action);
			write_short(qty);
		}
	};

	// Packet which requests that an item is equipped
	// Opcode: MOVE_ITEM(71)
	class EquipItemPacket : public MoveItemPacket
	{
	public:
		EquipItemPacket(int16_t src, EquipSlot::Id dest) : MoveItemPacket(InventoryType::Id::EQUIP, src, -dest, 1) {}
	};

	// Packet which requests that an item is unequipped
	// Opcode: MOVE_ITEM(71)
	class UnequipItemPacket : public MoveItemPacket
	{
	public:
		UnequipItemPacket(int16_t src, int16_t dest) : MoveItemPacket(InventoryType::Id::EQUIPPED, -src, dest, 1) {}
	};

	// A packet which requests that an 'USE' item is used
	// Opcode: USE_ITEM(72)
	class UseItemPacket : public OutPacket
	{
	public:
		UseItemPacket(int16_t slot, int32_t itemid) : OutPacket(OutPacket::Opcode::USE_ITEM)
		{
			Sound(itemid).play();

			write_time();
			write_short(slot);
			write_int(itemid);
		}
	};

	// Use a megaphone / speaker cash item.
	// Opcode: USE_CASH_ITEM(79)
	//
	// Payload (v83):
	//   short slot
	//   int itemid
	//   string message              (always)
	//   [for Super/Item megaphone (5072/5076):] bool whisper_icon
	//   [for Item megaphone (5076):]           bool has_item; if true: byte src_tab, short src_slot
	//   [for Triple megaphone (5077):]         byte line_count (1-3); strings line2..lineN; bool whisper_icon
	class UseMegaphonePacket : public OutPacket
	{
	public:
		// Regular / super / NPC — single line. whisper only used by super.
		UseMegaphonePacket(int16_t slot, int32_t itemid, const std::string& message, bool whisper)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(itemid);
			write_string(message);

			int32_t subtype = (itemid / 1000) % 10;
			if (subtype == 2) // Super Megaphone
				write_byte(whisper ? 1 : 0);
		}

		// Item megaphone (5076) — attaches an inventory item for preview.
		UseMegaphonePacket(int16_t slot, int32_t itemid, const std::string& message,
			bool whisper, int8_t src_tab, int16_t src_slot)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(itemid);
			write_string(message);
			write_byte(whisper ? 1 : 0);
			bool has_item = (src_tab > 0 && src_slot != 0);
			write_byte(has_item ? 1 : 0);
			if (has_item)
			{
				write_byte(src_tab);
				write_short(src_slot);
			}
		}

		// Triple megaphone (5077) — 1..3 lines, world-wide.
		UseMegaphonePacket(int16_t slot, int32_t itemid,
			const std::vector<std::string>& lines, bool whisper)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(itemid);

			uint8_t count = static_cast<uint8_t>(lines.size());
			if (count < 1) count = 1;
			if (count > 3) count = 3;

			write_string(lines.empty() ? std::string() : lines[0]);
			write_byte(count);
			for (uint8_t i = 1; i < count; i++)
				write_string(lines[i]);
			write_byte(whisper ? 1 : 0);
		}
	};

	// Use an Avatar Megaphone cash item (539xxxx). Payload:
	//   short slot; int itemid; 4 strings (messages); byte whisper_ear.
	// Server broadcasts SET_AVATAR_MEGAPHONE with sender CharLook + lines
	// to every client; current-channel info is filled in server-side.
	class UseAvatarMegaphonePacket : public OutPacket
	{
	public:
		UseAvatarMegaphonePacket(int16_t slot, int32_t itemid,
			const std::vector<std::string>& lines, bool whisper)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(itemid);
			for (int i = 0; i < 4; i++)
				write_string(i < static_cast<int>(lines.size()) ? lines[i] : std::string());
			write_byte(whisper ? 1 : 0);
		}
	};

	// Use a Maple TV cash item (5075xxx). Same opcode as megaphones.
	// Payload: short slot; int itemid; string victim_name (empty if none);
	//          5 strings (message lines, pad with empty); bool megassenger.
	class UseMapleTVPacket : public OutPacket
	{
	public:
		UseMapleTVPacket(int16_t slot, int32_t itemid,
			const std::string& victim_name,
			const std::vector<std::string>& lines,
			bool megassenger)
			: OutPacket(OutPacket::Opcode::USE_CASH_ITEM)
		{
			write_short(slot);
			write_int(itemid);
			write_string(victim_name);

			for (int i = 0; i < 5; i++)
				write_string(i < static_cast<int>(lines.size()) ? lines[i] : std::string());

			write_byte(megassenger ? 1 : 0);
		}
	};

	// A packet which requests sitting in a chair
	// Opcode: USE_CHAIR(43)
	class UseChairPacket : public OutPacket
	{
	public:
		UseChairPacket(int32_t itemid) : OutPacket(OutPacket::Opcode::USE_CHAIR)
		{
			write_int(itemid);
		}
	};

	// A packet which requests getting up from a chair
	// Opcode: CANCEL_CHAIR(42)
	class CancelChairPacket : public OutPacket
	{
	public:
		CancelChairPacket() : OutPacket(OutPacket::Opcode::CANCEL_CHAIR)
		{
			write_short(-1);
		}
	};

	// Requests using a scroll on an equip 
	// Opcode: SCROLL_EQUIP(86)
	class ScrollEquipPacket : public OutPacket
	{
	public:
		enum Flag : uint8_t
		{
			NONE = 0x00,
			UNKNOWN = 0x01,
			WHITESCROLL = 0x02
		};

		ScrollEquipPacket(int16_t source, EquipSlot::Id target, uint8_t flags) : OutPacket(OutPacket::Opcode::SCROLL_EQUIP)
		{
			write_time();
			write_short(source);
			write_short(-target);
			write_short(flags);
		}

		ScrollEquipPacket(int16_t source, EquipSlot::Id target) : ScrollEquipPacket(source, target, 0) {}
	};
}