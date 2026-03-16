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

		nl::node back = nl::nx::map["Back"]["login.img"]["back"];
		nl::node worldsrc = nl::nx::ui["Login.img"]["WorldSelect"]["BtWorld"]["release"];
		nl::node channelsrc = nl::nx::ui["Login.img"]["WorldSelect"]["BtChannel"];
		nl::node frame = nl::nx::ui["Login.img"]["Common"]["frame"];


		sprites.emplace_back(back["11"], Point<int16_t>(370, 300));
		sprites.emplace_back(worldsrc["layer:bg"], Point<int16_t>(650, 45));
		sprites.emplace_back(frame, Point<int16_t>(400, 290));

		buttons[BT_ENTERWORLD] = std::make_unique<MapleButton>(
			channelsrc["button:GoWorld"],
			Point<int16_t>(200, 170)
		);
		buttons[BT_ENTERWORLD]->set_active(false);

		channels_background = channelsrc["layer:bg"];

		// Pre-create channel buttons (up to 20 channels)
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
	}

	void UIWorldSelect::draw(float alpha) const
	{
		// Draw sprites (backgrounds) first
		UIElement::draw_sprites(alpha);

		// Draw channel panel AFTER sprites but BEFORE buttons
		if (world_selected)
		{
			channels_background.draw(position + Point<int16_t>(200, 170));

			if (worldid < world_textures.size())
				world_textures[worldid].draw(position + Point<int16_t>(200, 170));
		}

		// Draw buttons last (on top of everything)
		UIElement::draw_buttons(alpha);
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;

		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
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
				// Do nothing on escape for now
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

		// Re-request the server list from the server
		ServerRequestPacket().dispatch();
	}

	uint16_t UIWorldSelect::get_worldbyid(uint16_t wid)
	{
		return wid;
	}

	Button::State UIWorldSelect::button_pressed(uint16_t id)
	{
		if (id == BT_ENTERWORLD)
		{
			enter_world();
			return Button::State::PRESSED;
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
		else if (id >= BT_CHANNEL0)
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
			if (btit.first >= BT_CHANNEL0)
			{
				btit.second->set_state(Button::State::NORMAL);
				btit.second->set_active(false);
			}
		}

		buttons[BT_ENTERWORLD]->set_active(false);
	}
}
