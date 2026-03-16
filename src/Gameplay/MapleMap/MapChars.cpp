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
#include "MapChars.h"

#include "../../IO/UI.h"
#include "../../IO/UITypes/UICharInfo.h"
#include "../../Net/Packets/PlayerInteractionPackets.h"

namespace ms
{
	Cursor::State MapChars::send_cursor(bool pressed, Point<int16_t> position, Point<int16_t> viewpos)
	{
		for (auto& map_object : chars)
		{
			OtherChar* ochar = static_cast<OtherChar*>(map_object.second.get());
			if (!ochar)
				continue;

			Point<int16_t> absp = ochar->get_position() + viewpos;

			// Use a reasonable hitbox for character sprites (~60 wide, ~80 tall)
			Rectangle<int16_t> bounds(
				absp.x() - 30,
				absp.x() + 30,
				absp.y() - 70,
				absp.y()
			);

			if (bounds.contains(position))
			{
				if (pressed)
				{
					// Request character info from the server
					CharInfoRequestPacket(ochar->get_oid()).dispatch();
					return Cursor::State::IDLE;
				}
				else
				{
					return Cursor::State::CANCLICK;
				}
			}
		}

		return Cursor::State::IDLE;
	}

	void MapChars::draw(Layer::Id layer, double viewx, double viewy, float alpha) const
	{
		chars.draw(layer, viewx, viewy, alpha);
	}

	void MapChars::update(const Physics& physics)
	{
		for (; !spawns.empty(); spawns.pop())
		{
			const CharSpawn& spawn = spawns.front();

			int32_t cid = spawn.get_cid();
			Optional<OtherChar> ochar = get_char(cid);

			if (ochar)
			{
				// No action needed
			}
			else
			{
				chars.add(spawn.instantiate());
			}
		}

		chars.update(physics);
	}

	void MapChars::spawn(CharSpawn&& spawn)
	{
		spawns.emplace(std::move(spawn));
	}

	void MapChars::remove(int32_t cid)
	{
		chars.remove(cid);
	}

	void MapChars::clear()
	{
		chars.clear();
	}

	MapObjects * MapChars::get_chars()
	{
		return &chars;
	}

	void MapChars::send_movement(int32_t cid, const std::vector<Movement>& movements)
	{
		if (Optional<OtherChar> otherchar = get_char(cid))
			otherchar->send_movement(movements);
	}

	void MapChars::update_look(int32_t cid, const LookEntry& look)
	{
		if (Optional<OtherChar> otherchar = get_char(cid))
			otherchar->update_look(look);
	}

	Optional<OtherChar> MapChars::get_char(int32_t cid)
	{
		return chars.get(cid);
	}
}