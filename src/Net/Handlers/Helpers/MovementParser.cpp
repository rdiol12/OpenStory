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
#include "MovementParser.h"

namespace ms
{
	std::vector<Movement> MovementParser::parse_movements(InPacket& recv)
	{
		std::vector<Movement> movements;
		uint8_t length = recv.read_byte();

		// Byte layouts must match exactly what the server sends (see
		// Cosmic's AbstractMovementPacketHandler.parseMovement). Reading
		// the wrong number of bytes for ANY command desyncs every later
		// fragment in the packet, leaving foreign players and mobs at
		// garbage positions/stances.
		for (uint8_t i = 0; i < length; ++i)
		{
			Movement fragment;
			fragment.command = recv.read_byte();

			switch (fragment.command)
			{
			case 0:  // normal move
			case 5:
			case 17: // float
				// Absolute — 13 bytes.
				fragment.type = Movement::ABSOLUTE;
				fragment.xpos = recv.read_short();
				fragment.ypos = recv.read_short();
				fragment.lastx = recv.read_short();  // x wobble
				fragment.lasty = recv.read_short();  // y wobble
				fragment.fh = recv.read_short();
				fragment.newstate = recv.read_byte();
				fragment.duration = recv.read_short();
				break;
			case 1:  // jump
			case 2:  // knockback
			case 6:  // flash jump
			case 12:
			case 13:
			case 16: // float
			case 18:
			case 19: // springs on maps
			case 20: // Aran combat step
			case 22:
				// Relative — 7 bytes.
				fragment.type = Movement::RELATIVE;
				fragment.xpos = recv.read_short();
				fragment.ypos = recv.read_short();
				fragment.newstate = recv.read_byte();
				fragment.duration = recv.read_short();
				break;
			case 3:  // teleport disappear
			case 4:  // teleport appear
			case 7:  // assaulter
			case 8:  // assassinate
			case 9:  // rush
			case 11: // chair
				// Teleport-style — 9 bytes, absolute position, no duration.
				fragment.type = Movement::ABSOLUTE;
				fragment.xpos = recv.read_short();
				fragment.ypos = recv.read_short();
				fragment.lastx = recv.read_short();  // x wobble
				fragment.lasty = recv.read_short();  // y wobble
				fragment.newstate = recv.read_byte();
				break;
			case 15:
				// Jump down — 15 bytes.
				fragment.type = Movement::JUMPDOWN;
				fragment.xpos = recv.read_short();
				fragment.ypos = recv.read_short();
				fragment.lastx = recv.read_short();  // x wobble
				fragment.lasty = recv.read_short();  // y wobble
				fragment.fh = recv.read_short();
				recv.skip(2);                        // origin foothold
				fragment.newstate = recv.read_byte();
				fragment.duration = recv.read_short();
				break;
			case 10:
				// Change equip — 1 byte, no position.
				fragment.type = Movement::NONE;
				recv.skip(1);
				break;
			case 14:
				// Jump-down variant — 9 bytes, no usable position.
				fragment.type = Movement::NONE;
				recv.skip(9);
				break;
			case 21:
				// Aran-related — 3 bytes.
				fragment.type = Movement::NONE;
				recv.skip(3);
				break;
			default:
				// Unknown command: its size is unknown, so continuing would
				// desync the rest of the packet. Stop here with what we have.
				return movements;
			}

			movements.push_back(fragment);
		}

		return movements;
	}
}