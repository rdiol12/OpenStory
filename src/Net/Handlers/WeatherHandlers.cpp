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
#include "WeatherHandlers.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
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
}
