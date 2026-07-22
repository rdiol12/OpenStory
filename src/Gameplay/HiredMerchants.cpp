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
#include "HiredMerchants.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void HiredMerchants::add(int32_t owner_id, int32_t oid, int32_t item_id, Point<int16_t> pos,
		const std::string& owner, const std::string& desc, int8_t skin)
	{
		std::string strid = "0" + std::to_string(item_id);
		nl::node stand = nl::nx::item["Cash"]["0503.img"][strid]["employee"]["say"];

		Merchant m;
		m.oid = oid;
		m.item_id = item_id;
		m.pos = pos;
		m.owner = owner;
		m.desc = desc;
		m.skin = skin;
		m.stand = Animation(stand);
		merchants[owner_id] = std::move(m);
	}

	void HiredMerchants::remove(int32_t owner_id)
	{
		merchants.erase(owner_id);
	}

	void HiredMerchants::clear_all()
	{
		merchants.clear();
	}

	void HiredMerchants::draw(Point<int16_t> viewpos, float alpha) const
	{
		static Texture psskins[7];
		static bool skins_loaded = false;

		if (!skins_loaded)
		{
			skins_loaded = true;
			nl::node ps = nl::nx::ui["ChatBalloon.img"]["miniroom"]["PSSkin"];

			for (int i = 0; i < 7; i++)
				psskins[i] = ps[std::to_string(i)];
		}

		static Text shoptext(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);

		for (const auto& it : merchants)
		{
			const Merchant& m = it.second;
			Point<int16_t> absp = m.pos + viewpos;

			m.stand.draw(DrawArgument(absp), alpha);

			int8_t skin = (m.skin >= 0 && m.skin < 7 && psskins[m.skin].is_valid()) ? m.skin : 0;
			const Texture& sign = psskins[skin];
			int16_t w = sign.get_dimensions().x();
			int16_t h = sign.get_dimensions().y();
			Point<int16_t> tl(absp.x() - w / 2, absp.y() - 82 - h);

			sign.draw(DrawArgument(tl + sign.get_origin()));

			std::string d = m.desc.empty() ? m.owner : m.desc;
			if (d.size() > 14)
				d = d.substr(0, 14);
			shoptext.change_text(d);
			shoptext.draw(tl + Point<int16_t>(w / 2, skin == 0 ? 9 : 68));
		}
	}

	void HiredMerchants::update()
	{
		for (auto& it : merchants)
			it.second.stand.update();
	}

	const HiredMerchants::Merchant* HiredMerchants::find_at(Point<int16_t> mappos) const
	{
		for (const auto& it : merchants)
		{
			const Merchant& m = it.second;

			if (mappos.x() >= m.pos.x() - 28 && mappos.x() <= m.pos.x() + 28
				&& mappos.y() >= m.pos.y() - 85 && mappos.y() <= m.pos.y())
				return &m;
		}

		return nullptr;
	}
}
