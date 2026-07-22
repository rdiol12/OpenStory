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

#include "../UIElement.h"

#include "../../Graphics/Animation.h"
#include "../../Graphics/Text.h"
#include "../../Net/Login.h"

namespace ms
{
	class UIWorldSelect : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::WORLDSELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIWorldSelect();

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void draw_world();
		void add_world(World world);
		void add_recommended_world(RecommendedWorld world);
		void change_world(World selectedWorld);
		void remove_selected();
		void set_region(uint8_t value);
		void refresh_worlds();
		uint16_t get_worldbyid(uint16_t worldid);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void enter_world();
		void clear_selected_world();

		enum Buttons : uint16_t
		{
			BT_WORLD0 = 0,
			BT_CHANNEL0 = 5,
			BT_ENTERWORLD = 25,
			BT_VIEWALL = 26,
			BT_VIEWCHOICE = 27,
			BT_QUITGAME = 28,
			BT_CHANGEREGION = 29
		};

		// The whole 800x600 design is rendered scaled by a uniform factor,
		// centered in the view; lay() maps design coords to screen coords.
		Point<int16_t> lay(int16_t x, int16_t y) const;
		Point<int16_t> lay(Point<int16_t> p) const;

		uint8_t worldid;
		uint8_t channelid;
		uint8_t worldcount;
		uint8_t region;

		std::vector<World> worlds;

		bool world_selected;

		// NX node references (kept for draw_world/set_region)
		nl::node world_select;
		nl::node world_src;
		nl::node channel_src;

		// Uniform content scale and centered content-box origin
		float ui_scale;
		Point<int16_t> box;

		// Design-space anchors
		Point<int16_t> world_pos;

		// Full-view city backdrop (single cover-scaled copy of the scene)
		Texture backdrop;
		DrawArgument backdrop_args;

		// The v83 scroll: closed frame, open/close animations (scroll/0 and
		// scroll/1), and the held-open frame the channel grid draws on
		int8_t scroll_state;
		Texture scroll_closed_tex;
		Texture scroll_open_tex;
		Animation scroll_opening;
		Animation scroll_closing;

		// Selected world's title decoration on the open parchment, and the
		// divider sheet it sits on
		nl::node world_title_src;
		Texture world_title;
		Texture chback;

		// Rope hanger and step indicator, drawn over the scroll
		Texture hanger;
		Texture step_tex;

		// Population gauge drawn over each channel button's bar, scaled by
		// the channel load from the serverlist packet
		Texture channel_gauge;

		// World plank focused by keyboard navigation (-1 = none)
		int16_t focused_world;

		// Activate the channel grid once the scroll has fully opened
		void show_channels();

		// Channel selection highlight
		Animation channel_selected;

		// Bouncing EVENT balloon for event-flagged worlds
		Animation event_badge;

		// Version text
		Text version;
	};
}
