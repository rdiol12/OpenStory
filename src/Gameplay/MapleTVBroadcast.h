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

#include "../Template/Singleton.h"

#include <cstdint>
#include "../Graphics/Animation.h"
#include "../Graphics/Texture.h"
#include "../Net/Login.h"

#include <string>
#include <vector>

namespace ms
{
	// Tracks the world-wide MapleTV broadcast that's currently on the air.
	// Map-embedded TV sprites read this state to decide what avatar/message
	// to composite onto themselves.
	class MapleTVBroadcast : public Singleton<MapleTVBroadcast>
	{
	public:
		void start(const std::string& sender_name,
			const std::vector<std::string>& lines,
			const std::string& victim_name,
			int32_t duration_ms);

		void stop();
		void update();

		// Per-frame tick from Stage: advances the off-air loop and the
		// broadcast countdown
		void tick();
		// Paints both TV screens: the top show screen at the msg anchor
		// (TVbasic frame + chatting hosts when idle, backdrop when live)
		// and the ad reel at the ad anchor
		void draw_screen(Point<int16_t> ad_anchor, Point<int16_t> msg_anchor, float alpha) const;

		void set_look(const LookEntry& entry) { sender_look_ = entry; has_look_ = true; }
		bool has_look() const { return has_look_; }
		const LookEntry& get_look() const { return sender_look_; }
		int32_t serial() const { return serial_; }

		bool active() const { return active_; }
		const std::string& sender_name() const { return sender_name_; }
		const std::string& victim_name() const { return victim_name_; }
		const std::vector<std::string>& lines() const { return lines_; }
		int32_t remaining_ms() const { return remaining_ms_; }

	private:
		bool active_ = false;
		LookEntry sender_look_;
		bool has_look_ = false;
		int32_t serial_ = 0;
		mutable Animation tv_ads_[3];
		mutable int ad_index_ = 0;
		mutable Texture tv_basic_;
		mutable Animation tv_show_[2];
		mutable int show_index_ = 0;
		mutable bool anims_loaded_ = false;
		void load_anims() const;
		std::string sender_name_;
		std::string victim_name_;
		std::vector<std::string> lines_;
		int32_t remaining_ms_ = 0;
	};
}
