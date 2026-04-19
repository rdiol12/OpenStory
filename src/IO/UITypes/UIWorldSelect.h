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
#include "../../Gameplay/MapleMap/MapBackgrounds.h"
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

		uint8_t worldid;
		uint8_t channelid;
		uint8_t worldcount;
		uint8_t region;

		std::vector<World> worlds;

		bool world_selected;
		bool draw_chatballoon;

		// NX node references (kept for draw_world/set_region)
		nl::node world_select;
		nl::node world_src;
		nl::node channel_src;

		// Panel positions
		Point<int16_t> channel_pos;
		Point<int16_t> world_pos;

		// Textures
		Texture channels_background;
		Texture worlds_background;
		Texture channel_gauge;
		Texture scroll_closed;
		Texture scroll_open;
		std::vector<Texture> world_textures;

		// Channel selection highlight
		Animation channel_selected;

		// Scroll/springboard position
		Point<int16_t> scroll_pos;

		// Channel panel top-left (cached for drawing)
		Point<int16_t> ch_panel_tl;

		// Map backgrounds (from MapLogin.img)
		MapBackgrounds map_backgrounds;

		// Version text
		Text version;
	};
}
