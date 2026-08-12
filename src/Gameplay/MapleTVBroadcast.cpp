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

#include "../Graphics/Text.h"

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
		has_partner_look_ = false;
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

		if (active_)
		{
			int theme = type_ % 4;

			if (theme == 1 || theme == 2)
				tv_show_[theme - 1].update();

			// A default-constructed CharLook has no Face; updating it derefs null.
			if (has_look_)
				sender_avatar_.update(Constants::TIMESTEP);

			if (has_partner_look_)
				partner_avatar_.update(Constants::TIMESTEP);

			update();
		}
	}

	void MapleTVBroadcast::set_look(const LookEntry& entry)
	{
		sender_look_ = entry;
		sender_avatar_ = CharLook(entry);
		has_look_ = true;
	}

	void MapleTVBroadcast::set_partner_look(const LookEntry& entry)
	{
		partner_avatar_ = CharLook(entry);
		has_partner_look_ = true;
	}

	void MapleTVBroadcast::draw_screen(Point<int16_t> ad_anchor, Point<int16_t> msg_anchor, float alpha) const
	{
		load_anims();

		// The authored anchors are each screen's top-left; frame origins
		// are cancelled so the art lands inside the screens
		tv_basic_.draw(DrawArgument(msg_anchor + tv_basic_.get_origin()));

		// The ad reel has its own lower screen and keeps running during a broadcast;
		// only the hosts pause, since they share the upper screen with the message.
		tv_ads_[ad_index_].draw(DrawArgument(ad_anchor + tv_ads_[ad_index_].get_origin()), alpha);

		if (!active_)
			return;

		// TVchat1/TVchat2 are the star and heart themes named in the item tooltips, not
		// idle content. They are centre-anchored, so they land on the screen midpoint.
		int theme = type_ % 4;
		bool themed = (theme == 1 || theme == 2);

		if (themed)
			tv_show_[theme - 1].draw(DrawArgument(msg_anchor + Point<int16_t>(120, 45)), alpha);

		// Screen is 240x90. Avatars stand on the left with their feet near the bottom;
		// the message runs to their right.
		int16_t text_left = msg_anchor.x() + 26;

		if (has_look_)
		{
			sender_avatar_.draw(msg_anchor + Point<int16_t>(30, 80), true,
				Stance::Id::STAND1, Expression::Id::DEFAULT);
			text_left = msg_anchor.x() + 66;

			if (has_partner_look_)
			{
				partner_avatar_.draw(msg_anchor + Point<int16_t>(72, 80), true,
					Stance::Id::STAND1, Expression::Id::DEFAULT);
				text_left = msg_anchor.x() + 106;
			}
		}

		constexpr float TEXT_SCALE = 1.5f;

		int16_t y = msg_anchor.y() + 14;

		for (const std::string& ln : lines_)
		{
			if (ln.empty())
				continue;

			Text t(Text::Font::A11M, Text::Alignment::LEFT,
				themed ? Color::Name::WHITE : Color::Name::BLACK, ln,
				static_cast<uint16_t>((msg_anchor.x() + 234 - text_left) / TEXT_SCALE));
			t.draw(DrawArgument(Point<int16_t>(text_left, y), TEXT_SCALE, TEXT_SCALE));
			y += static_cast<int16_t>(13 * TEXT_SCALE);
		}
	}
}
