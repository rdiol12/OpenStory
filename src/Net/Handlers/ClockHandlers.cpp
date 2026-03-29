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
#include "ClockHandlers.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/UIClock.h"

namespace ms
{
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

	void StopClockHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // 0

		Stage::get().clear_clock();
	}
}
