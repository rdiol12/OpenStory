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
			BT_ENTERWORLD = 0,
			BT_WORLD0 = 1,
			BT_CHANNEL0 = 17,
			BT_GOWORLD = 37,
			BT_VIEWALL = 38,
			BT_VIEWCHOICE = 39,
			BT_SCROLLUP = 40,
			BT_SCROLLDOWN = 41,
			BT_ALERT_ARROWL = 42,
			BT_ALERT_ARROWR = 43,
			BT_ALERT_CHOICE = 44,
			BT_ALERT_CLOSE = 45
		};

		uint8_t worldid;
		uint8_t channelid;
		uint8_t worldcount;
		uint8_t region;

		std::vector<World> worlds;

		bool world_selected;
		bool show_alert;

		Texture channels_background;
		std::vector<Texture> world_textures;

		// Channel gauge (load indicator)
		Texture channel_gauge;

		// Channel background (alternative)
		Texture ch_backgrn;

		// Channel labels and decorations
		std::vector<Texture> channel_labels;
		Texture ch_event;
		Texture ch_select;
		Texture ch_gauge_bar;

		// World name label bitmaps
		std::vector<Texture> world_labels;

		// World notice
		Texture world_notice;

		// Tooltip textures
		std::vector<Texture> tooltip_textures;

		// Alert panel
		Texture alert_backgrd;

		// Hover tracking for world button tooltips
		int16_t hovered_world;

		// Active channel count for the selected world
		uint8_t active_channelcount;
	};
}
