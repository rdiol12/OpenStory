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
#include "UIWorldSelect.h"

#include "UILoginWait.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Audio/Audio.h"
#include "../../Net/Packets/LoginPackets.h"


#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIWorldSelect::UIWorldSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600))
	{
		worldid = Setting<DefaultWorld>::get().load();
		channelid = Setting<DefaultChannel>::get().load();
		region = Setting<DefaultRegion>::get().load();
		worldcount = 0;
		world_selected = false;
		show_alert = false;
		hovered_world = -1;
		active_channelcount = 0;

		nl::node back = nl::nx::map["Back"]["login.img"]["back"];
		nl::node ws = nl::nx::ui["Login.img"]["WorldSelect"];
		nl::node worldsrc = ws["BtWorld"]["release"];
		nl::node channelsrc = ws["BtChannel"];

		// === Background sprites ===
		sprites.emplace_back(back["11"], Point<int16_t>(370, 300));
		sprites.emplace_back(worldsrc["layer:bg"], Point<int16_t>(650, 45));

		// === Enter world button ===
		buttons[BT_ENTERWORLD] = std::make_unique<MapleButton>(
			channelsrc["button:GoWorld"],
			Point<int16_t>(200, 170)
		);
		buttons[BT_ENTERWORLD]->set_active(false);

		// === Channel background and gauge ===
		channels_background = channelsrc["layer:bg"];
		channel_gauge = Texture(channelsrc["gauge"]);

		// === Alternative channel background ===
		ch_backgrn = Texture(ws["chBackgrn"]);

		// === Channel labels (0-19) ===
		nl::node channel_node = ws["channel"];
		for (uint8_t i = 0; i < 20; ++i)
		{
			nl::node ch_label = channel_node[std::to_string(i)];
			if (ch_label)
				channel_labels.emplace_back(ch_label);
		}

		// Channel decorations
		ch_event = Texture(channel_node["chEvent"]);
		ch_select = Texture(channel_node["chSelect"]);
		ch_gauge_bar = Texture(channel_node["chgauge"]);

		// === Channel buttons (up to 20 channels) ===
		for (uint8_t i = 0; i < 20; ++i)
		{
			nl::node chnode = channelsrc["button:" + std::to_string(i)];

			if (chnode)
			{
				buttons[BT_CHANNEL0 + i] = std::make_unique<MapleButton>(
					chnode, Point<int16_t>(200, 170)
				);
				buttons[BT_CHANNEL0 + i]->set_active(false);
			}
		}

		// === BtGoworld (standalone go-to-world button) ===
		buttons[BT_GOWORLD] = std::make_unique<MapleButton>(ws["BtGoworld"]);
		buttons[BT_GOWORLD]->set_active(false);

		// === BtViewAll / BtViewChoice (world list view toggles) ===
		buttons[BT_VIEWALL] = std::make_unique<MapleButton>(ws["BtViewAll"]);
		buttons[BT_VIEWCHOICE] = std::make_unique<MapleButton>(ws["BtViewChoice"]);
		buttons[BT_VIEWALL]->set_active(true);
		buttons[BT_VIEWCHOICE]->set_active(true);

		// === Scroll buttons ===
		nl::node scroll_node = ws["scroll"];
		buttons[BT_SCROLLUP] = std::make_unique<MapleButton>(scroll_node["0"]);
		buttons[BT_SCROLLDOWN] = std::make_unique<MapleButton>(scroll_node["1"]);

		// === Alert panel buttons ===
		nl::node alert_node = ws["alert"];
		alert_backgrd = Texture(alert_node["backgrd"]);
		buttons[BT_ALERT_ARROWL] = std::make_unique<MapleButton>(alert_node["BtArrowL"]);
		buttons[BT_ALERT_ARROWR] = std::make_unique<MapleButton>(alert_node["BtArrowR"]);
		buttons[BT_ALERT_CHOICE] = std::make_unique<MapleButton>(alert_node["BtChoice"]);
		buttons[BT_ALERT_CLOSE] = std::make_unique<MapleButton>(alert_node["BtClose"]);
		buttons[BT_ALERT_ARROWL]->set_active(false);
		buttons[BT_ALERT_ARROWR]->set_active(false);
		buttons[BT_ALERT_CHOICE]->set_active(false);
		buttons[BT_ALERT_CLOSE]->set_active(false);

		// === Tooltip textures ===
		nl::node tooltip_node = ws["tooltip"];
		for (int i = 0; i < 3; ++i)
		{
			nl::node tt = tooltip_node[std::to_string(i)];
			if (tt)
				tooltip_textures.emplace_back(tt);
		}
		// Also load release tooltip
		nl::node tt_release = tooltip_node["release"];
		if (tt_release)
			tooltip_textures.emplace_back(tt_release);

		// === World name label bitmaps ===
		nl::node world_label_node = ws["world"];
		for (int i = 0; i < 44; ++i)
		{
			nl::node wl = world_label_node[std::to_string(i)];
			if (wl)
				world_labels.emplace_back(wl);
			else
				world_labels.emplace_back(Texture());
		}

		// === World notice ===
		world_notice = Texture(ws["worldNotice"]["0"]);
	}

	void UIWorldSelect::draw(float alpha) const
	{
		// Draw sprites (backgrounds) first
		UIElement::draw_sprites(alpha);

		// Draw world notice (NX origin positions it)
		if (world_notice.is_valid())
			world_notice.draw(DrawArgument(position));

		// Draw channel panel AFTER sprites but BEFORE buttons
		if (world_selected)
		{
			// Channel background (NX origin positions it)
			if (ch_backgrn.is_valid())
				ch_backgrn.draw(DrawArgument(position));

			channels_background.draw(position + Point<int16_t>(200, 170));

			// World texture layer
			if (worldid < world_textures.size())
				world_textures[worldid].draw(position + Point<int16_t>(200, 170));

			// World name label (NX origin positions it)
			if (worldid < world_labels.size() && world_labels[worldid].is_valid())
				world_labels[worldid].draw(DrawArgument(position));

			// Channel selection highlight (NX origin positions it relative to channel area)
			if (ch_select.is_valid() && channelid < 20)
				ch_select.draw(DrawArgument(position));

			// Channel labels (only draw for active channels)
			for (size_t i = 0; i < active_channelcount && i < channel_labels.size(); ++i)
			{
				if (channel_labels[i].is_valid())
					channel_labels[i].draw(DrawArgument(position));
			}

			// Channel gauge (load indicator - NX origin)
			if (channel_gauge.is_valid())
				channel_gauge.draw(DrawArgument(position));

			// Channel load gauge bars (NX origin)
			if (ch_gauge_bar.is_valid())
				ch_gauge_bar.draw(DrawArgument(position));

			// Channel event marker (NX origin)
			if (ch_event.is_valid())
				ch_event.draw(DrawArgument(position));
		}

		// Draw alert panel (NX origin positions it)
		if (show_alert)
		{
			if (alert_backgrd.is_valid())
				alert_backgrd.draw(DrawArgument(position));
		}

		// Draw buttons last (on top of everything)
		UIElement::draw_buttons(alpha);

		// Draw tooltip over hovered world button
		if (hovered_world >= 0 && hovered_world < static_cast<int16_t>(tooltip_textures.size()))
		{
			auto btn_it = buttons.find(BT_WORLD0 + hovered_world);

			if (btn_it != buttons.end() && btn_it->second->is_active())
			{
				// Draw tooltip at the world button's position (NX origin handles offset)
				tooltip_textures[hovered_world].draw(DrawArgument(position));
			}
		}
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;

		// Track world button hover for tooltips
		hovered_world = -1;

		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				// Track which world button is hovered
				if (btit.first >= BT_WORLD0 && btit.first < BT_CHANNEL0)
					hovered_world = static_cast<int16_t>(btit.first - BT_WORLD0);

				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();
					btit.second->set_state(Button::State::MOUSEOVER);
					ret = Cursor::State::CANCLICK;
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(button_pressed(btit.first));
						ret = Cursor::State::IDLE;
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
				else if (btit.second->get_state() == Button::State::PRESSED)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(button_pressed(btit.first));
						ret = Cursor::State::IDLE;
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return ret;
	}

	void UIWorldSelect::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				if (show_alert)
				{
					show_alert = false;
					buttons[BT_ALERT_ARROWL]->set_active(false);
					buttons[BT_ALERT_ARROWR]->set_active(false);
					buttons[BT_ALERT_CHOICE]->set_active(false);
					buttons[BT_ALERT_CLOSE]->set_active(false);
				}
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				if (world_selected)
					enter_world();
			}
		}
	}

	UIElement::Type UIWorldSelect::get_type() const
	{
		return TYPE;
	}

	void UIWorldSelect::draw_world()
	{
		if (worldcount <= 0)
			return;

		nl::node worldsrc = nl::nx::ui["Login.img"]["WorldSelect"]["BtWorld"]["release"];

		for (auto& world : worlds)
		{
			nl::node worldbtn = worldsrc["button:" + std::to_string(world.wid)];

			if (worldbtn)
			{
				buttons[BT_WORLD0 + world.wid] = std::make_unique<MapleButton>(
					worldbtn,
					Point<int16_t>(650, 20)
				);
				buttons[BT_WORLD0 + world.wid]->set_active(true);
			}
		}

		// Show GoWorld button when worlds are available
		buttons[BT_GOWORLD]->set_active(true);

		// Auto-select first world
		if (!worlds.empty())
		{
			worldid = worlds[0].wid;
			buttons[BT_WORLD0 + worldid]->set_state(Button::State::PRESSED);
			world_selected = true;
			change_world(worlds[0]);
		}
	}

	void UIWorldSelect::add_world(World world)
	{
		nl::node channelsrc = nl::nx::ui["Login.img"]["WorldSelect"]["BtChannel"];
		std::string layer_name = "layer:" + std::to_string(world.wid);
		nl::node layer_node = channelsrc["release"][layer_name];
		world_textures.emplace_back(layer_node);

		worlds.emplace_back(std::move(world));
		worldcount++;
	}

	void UIWorldSelect::add_recommended_world(RecommendedWorld)
	{
		// Not used in v83
	}

	void UIWorldSelect::change_world(World selectedWorld)
	{
		clear_selected_world();
		active_channelcount = selectedWorld.channelcount;

		for (size_t i = 0; i < selectedWorld.channelcount; ++i)
		{
			if (buttons.count(BT_CHANNEL0 + i))
			{
				buttons[BT_CHANNEL0 + i]->set_active(true);

				if (i == channelid)
					buttons[BT_CHANNEL0 + i]->set_state(Button::State::PRESSED);
			}
		}

		buttons[BT_ENTERWORLD]->set_active(true);
	}

	void UIWorldSelect::remove_selected()
	{
		deactivate();

		world_selected = false;
		clear_selected_world();
	}

	void UIWorldSelect::set_region(uint8_t value)
	{
		if (region != value)
		{
			region = value;
			Setting<DefaultRegion>::get().save(region);
			refresh_worlds();
		}
	}

	void UIWorldSelect::refresh_worlds()
	{
		// Clear existing world state
		clear_selected_world();
		world_selected = false;

		// Remove world buttons
		for (auto& w : worlds)
		{
			uint16_t btn_id = BT_WORLD0 + w.wid;

			if (buttons.count(btn_id))
			{
				buttons[btn_id]->set_active(false);
				buttons.erase(btn_id);
			}
		}

		worlds.clear();
		world_textures.clear();
		worldcount = 0;

		buttons[BT_GOWORLD]->set_active(false);

		// Re-request the server list from the server
		ServerRequestPacket().dispatch();
	}

	uint16_t UIWorldSelect::get_worldbyid(uint16_t wid)
	{
		return wid;
	}

	Button::State UIWorldSelect::button_pressed(uint16_t id)
	{
		if (id == BT_ENTERWORLD || id == BT_GOWORLD)
		{
			enter_world();
			return Button::State::PRESSED;
		}
		else if (id == BT_VIEWALL)
		{
			// Show all worlds - already default behavior
			return Button::State::PRESSED;
		}
		else if (id == BT_VIEWCHOICE)
		{
			// Show selected/favorite worlds
			return Button::State::PRESSED;
		}
		else if (id == BT_SCROLLUP)
		{
			// Scroll world list up (for many worlds)
			return Button::State::NORMAL;
		}
		else if (id == BT_SCROLLDOWN)
		{
			// Scroll world list down
			return Button::State::NORMAL;
		}
		else if (id == BT_ALERT_CLOSE)
		{
			show_alert = false;
			buttons[BT_ALERT_ARROWL]->set_active(false);
			buttons[BT_ALERT_ARROWR]->set_active(false);
			buttons[BT_ALERT_CHOICE]->set_active(false);
			buttons[BT_ALERT_CLOSE]->set_active(false);
			return Button::State::NORMAL;
		}
		else if (id == BT_ALERT_ARROWL || id == BT_ALERT_ARROWR)
		{
			// Navigate alert pages
			return Button::State::NORMAL;
		}
		else if (id == BT_ALERT_CHOICE)
		{
			// Accept alert choice
			show_alert = false;
			buttons[BT_ALERT_ARROWL]->set_active(false);
			buttons[BT_ALERT_ARROWR]->set_active(false);
			buttons[BT_ALERT_CHOICE]->set_active(false);
			buttons[BT_ALERT_CLOSE]->set_active(false);
			return Button::State::NORMAL;
		}
		else if (id >= BT_WORLD0 && id < BT_CHANNEL0)
		{
			// Deselect old world
			if (buttons.count(BT_WORLD0 + worldid))
				buttons[BT_WORLD0 + worldid]->set_state(Button::State::NORMAL);

			worldid = static_cast<uint8_t>(id - BT_WORLD0);
			world_selected = true;

			// Find the world data
			for (auto& w : worlds)
			{
				if (w.wid == worldid)
				{
					change_world(w);
					break;
				}
			}

			return Button::State::PRESSED;
		}
		else if (id >= BT_CHANNEL0 && id < BT_GOWORLD)
		{
			uint8_t selectedch = static_cast<uint8_t>(id - BT_CHANNEL0);

			if (selectedch != channelid)
			{
				if (buttons.count(BT_CHANNEL0 + channelid))
					buttons[BT_CHANNEL0 + channelid]->set_state(Button::State::NORMAL);

				channelid = selectedch;

				if (buttons.count(BT_CHANNEL0 + channelid))
					buttons[BT_CHANNEL0 + channelid]->set_state(Button::State::PRESSED);

				Sound(Sound::Name::WORLDSELECT).play();
			}
			else
			{
				enter_world();
			}

			return Button::State::PRESSED;
		}

		return Button::State::NORMAL;
	}

	void UIWorldSelect::enter_world()
	{
		Configuration::get().set_worldid(worldid);
		Configuration::get().set_channelid(channelid);

		UI::get().emplace<UILoginWait>();
		auto loginwait = UI::get().get_element<UILoginWait>();

		if (loginwait && loginwait->is_active())
			CharlistRequestPacket(worldid, channelid).dispatch();
	}

	void UIWorldSelect::clear_selected_world()
	{
		channelid = 0;

		for (auto& btit : buttons)
		{
			if (btit.first >= BT_CHANNEL0 && btit.first < BT_GOWORLD)
			{
				btit.second->set_state(Button::State::NORMAL);
				btit.second->set_active(false);
			}
		}

		buttons[BT_ENTERWORLD]->set_active(false);
	}
}
