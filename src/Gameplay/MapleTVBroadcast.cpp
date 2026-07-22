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
#include "MapleTVBroadcast.h"

#include "../Constants.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void MapleTVBroadcast::start(const std::string& sender_name,
		const std::vector<std::string>& lines,
		const std::string& victim_name,
		int32_t duration_ms)
	{
		sender_name_ = sender_name;
		victim_name_ = victim_name;
		lines_ = lines;
		remaining_ms_ = duration_ms > 0 ? duration_ms : 15000;
		active_ = true;
		serial_++;
	}

	void MapleTVBroadcast::stop()
	{
		active_ = false;
		sender_name_.clear();
		victim_name_.clear();
		lines_.clear();
		remaining_ms_ = 0;
	}

	void MapleTVBroadcast::update()
	{
		if (!active_) return;
		remaining_ms_ -= static_cast<int32_t>(Constants::TIMESTEP);
		if (remaining_ms_ <= 0)
			stop();
	}
}

namespace ms
{
	void MapleTVBroadcast::load_anims() const
	{
		if (anims_loaded_)
			return;

		anims_loaded_ = true;

		nl::node tv = nl::nx::ui["MapleTV.img"];
		nl::node media = tv["TVmedia"];

		for (int i = 0; i < 3; i++)
			tv_ads_[i] = Animation(media[std::to_string(i)]);

		tv_basic_ = tv["TVbasic"]["0"];
		tv_show_[0] = Animation(tv["TVchat1"]);
		tv_show_[1] = Animation(tv["TVchat2"]);
	}

	void MapleTVBroadcast::tick()
	{
		load_anims();

		// Idle programming: cycle through the three ad reels and the two
		// host-chat shows on the top screen
		if (tv_ads_[ad_index_].update())
			ad_index_ = (ad_index_ + 1) % 3;

		if (tv_show_[show_index_].update())
			show_index_ = (show_index_ + 1) % 2;

		if (active_)
			update();
	}

	void MapleTVBroadcast::draw_screen(Point<int16_t> ad_anchor, Point<int16_t> msg_anchor, float alpha) const
	{
		load_anims();

		// The authored anchors are each screen's top-left; frame origins
		// are cancelled so the art lands inside the screens
		tv_basic_.draw(DrawArgument(msg_anchor + tv_basic_.get_origin()));

		if (!active_)
		{
			tv_ads_[ad_index_].draw(DrawArgument(ad_anchor + tv_ads_[ad_index_].get_origin()), alpha);

			// Chatting hosts centered on the top screen
			tv_show_[show_index_].draw(DrawArgument(msg_anchor + Point<int16_t>(120, 45)), alpha);
		}
	}
}
