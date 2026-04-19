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

#include "../UIScale.h"

#include "../../Audio/Audio.h"
#include "../../Constants.h"
#include "../../Net/Packets/LoginPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIWorldSelect::UIWorldSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600), ScaleMode::CENTER_OFFSET)
	{
		worldid = Setting<DefaultWorld>::get().load();
		channelid = Setting<DefaultChannel>::get().load();
		region = Setting<DefaultRegion>::get().load();
		worldcount = 0;
		world_selected = false;
		draw_chatballoon = true;

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		// v83 positions from shroom95 decompilation
		// Channel panel bg has origin (224,116), so channel_pos is center point
		// Panel top-left = channel_pos - (224,116)
		// Channel buttons centered inside scroll
		// scroll at (143,100), 513 wide → center x = 399
		// ch_panel_tl = channel_pos - (224,116)
		// We want ch_panel_tl.x + 23 + 2*66 ≈ 399 (center 3rd channel)
		// ch_panel_tl.x = 399 - 23 - 132 = 244 → channel_pos.x = 244 + 224 = 468
		channel_pos = Point<int16_t>(468, 320);
		// World buttons panel — just below the rope on signboard
		world_pos = Point<int16_t>(130, 162);

		nl::node obj = nl::nx::map["Obj"]["login.img"];
		nl::node login = nl::nx::ui["Login.img"];
		world_select = login["WorldSelect"];
		// v83: world buttons are directly under BtWorld/0, BtWorld/1, etc.
		world_src = world_select["BtWorld"];
		// v83: channel sprites are under WorldSelect/channel/0, channel/1, etc.
		channel_src = world_select["channel"];
		nl::node common = login["Common"];
		nl::node back = nl::nx::map["Back"]["login.img"]["back"];
		nl::node ws_obj = obj["WorldSelect"];

		// v83: NeoCity scene fills the entire screen as the background.
		// neoCity/0 (625x479, origin 312,239) is scaled to fill 800x600.
		nl::node map_login = nl::nx::ui["MapLogin.img"];

		// Base background (fills any gaps from rounding)
		sprites.emplace_back(back["11"], UIScale::bg_args());

		// neoCity/0 — main background scene, scaled to fill the full screen
		nl::node neoCity0 = nl::nx::map["Obj"]["login.img"]["WorldSelect"]["neoCity"]["0"];
		if (neoCity0)
		{
			// Scale to fill view, same as bg_args() but adjusted for 625x479 → 800x600
			float sx = UIScale::scale_x() * 800.0f / 625.0f;
			float sy = UIScale::scale_y() * 600.0f / 479.0f;
			sprites.emplace_back(neoCity0, DrawArgument(Point<int16_t>(400, 300), sx, sy));
		}

		// NeoCity scene overlay animations — part of background scene
		const Point<int16_t> cam_offset = Point<int16_t>(366, 948);
		nl::node map_login2 = nl::nx::ui["MapLogin.img"];

		if (map_login2)
		{
			for (int page = 0; page < 10; page++)
			{
				nl::node ml_page = map_login2[std::to_string(page)];
				if (!ml_page) continue;

				for (auto entry : ml_page["obj"])
				{
					std::string l0 = entry["l0"];
					if (l0 != "WorldSelect") continue;

					std::string l1 = entry["l1"];
					std::string l2 = entry["l2"];

					if (l1 == "neoCity" && l2 == "0") continue;
					if (l1 == "signboard") continue;

					std::string oS = entry["oS"];
					int16_t x = entry["x"];
					int16_t y = entry["y"];

					nl::node sprite_src = nl::nx::map["Obj"][oS + ".img"][l0][l1][l2];
					if (!sprite_src) continue;

					sprites.emplace_back(sprite_src, Point<int16_t>(x, y) + cam_offset);
				}
			}
		}

		// UI frame overlay (on top of background and animations)
		sprites.emplace_back(common["frame"], UIScale::bg_args());

		// Signboard — wooden board behind world buttons
		if (ws_obj["signboard"]["0"])
			sprites.emplace_back(ws_obj["signboard"]["0"], Point<int16_t>(400, 80));

		// Step indicator
		if (common["step"]["1"])
			sprites.emplace_back(common["step"]["1"], Point<int16_t>(40, 0));

		// View All / View Choice buttons
		buttons[BT_VIEWALL] = std::make_unique<MapleButton>(world_select["BtViewAll"], Point<int16_t>(0, 53));
		buttons[BT_VIEWCHOICE] = std::make_unique<MapleButton>(world_select["BtViewChoice"], Point<int16_t>(0, 53));
		buttons[BT_VIEWALL]->set_active(false);
		buttons[BT_VIEWCHOICE]->set_active(true);

		// Quit game button
		if (common["BtExit"])
			buttons[BT_QUITGAME] = std::make_unique<MapleButton>(common["BtExit"], Point<int16_t>(0, 515));

		// v83: Channel buttons — 5 per row (from shroom95 decompilation)
		// Positions relative to channel panel top-left: (col*66+23, row*29+93)
		// Panel top-left = channel_pos - origin = (203-224, 164-116) = (-21, 48)
		ch_panel_tl = channel_pos - Point<int16_t>(224, 116);

		for (uint8_t i = 0; i < 20; ++i)
		{
			std::string ch = std::to_string(i);
			nl::node chnode = channel_src[ch];

			if (chnode)
			{
				int16_t col = i % 5;
				int16_t row = i / 5;
				Point<int16_t> ch_btn_pos = ch_panel_tl + Point<int16_t>(col * 100 + 23, row * 38 + 93);

				buttons[BT_CHANNEL0 + i] = std::make_unique<TwoSpriteButton>(
					chnode["normal"],
					chnode["disabled"],
					ch_btn_pos
				);
				buttons[BT_CHANNEL0 + i]->set_active(false);
			}
		}

		// v83: Channel panel background
		channels_background = world_select["chBackgrn"];

		// Scroll/springboard — always visible
		// scroll/1/3 = closed (513x152), scroll/0/1 = open (513x416)
		if (world_select["scroll"]["1"]["3"])
			scroll_closed = Texture(world_select["scroll"]["1"]["3"]);
		if (world_select["scroll"]["0"]["1"])
			scroll_open = Texture(world_select["scroll"]["0"]["1"]);

		// Scroll position — just below the signboard
		scroll_pos = Point<int16_t>(143, 100);

		// Channel selection highlight — use last frame (full size 59x33) as static texture
		if (channel_src["chSelect"]["3"])
			channel_selected = Animation(channel_src["chSelect"]);

		// Channel population gauge
		if (channel_src["chgauge"])
			channel_gauge = Texture(channel_src["chgauge"]);


		// v83: Enter world button (BtGoworld) — below channel grid
		// Panel bottom ≈ ch_panel_tl.y + 233 = 48 + 233 = 281
		buttons[BT_ENTERWORLD] = std::make_unique<MapleButton>(
			world_select["BtGoworld"], ch_panel_tl + Point<int16_t>(170, 210)
		);
		buttons[BT_ENTERWORLD]->set_active(false);

		// Set up region (loads world panel bg + world buttons)
		set_region(region);

	}

	void UIWorldSelect::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		auto drawpos = get_draw_position();

		// Scroll/springboard — always visible
		if (world_selected)
		{
			// Open springboard
			if (scroll_open.is_valid())
				scroll_open.draw(drawpos + scroll_pos);

			// World name/decoration at top-left of scroll
			if (worldid < world_textures.size())
				world_textures[worldid].draw(drawpos + scroll_pos + Point<int16_t>(50, 130));
		}
		else
		{
			// Closed springboard
			if (scroll_closed.is_valid())
				scroll_closed.draw(drawpos + scroll_pos);
		}

		UIElement::draw_buttons(alpha);

		// Draw gauge and selection highlight ON TOP of channel buttons
		if (world_selected)
		{
			uint8_t channel_total = 0;
			for (auto& w : worlds)
			{
				if (w.wid == worldid)
				{
					channel_total = w.channelcount;
					break;
				}
			}

			Point<int16_t> ch_origin = drawpos + ch_panel_tl;

			for (uint8_t i = 0; i < channel_total && i < 20; ++i)
			{
				int16_t col = i % 5;
				int16_t row = i / 5;
				Point<int16_t> ch_pos = ch_origin + Point<int16_t>(col * 100 + 23, row * 38 + 93);

				if (channel_gauge.is_valid())
					channel_gauge.draw(ch_pos + Point<int16_t>(46, 23));

				if (i == channelid)
					channel_selected.draw(DrawArgument(ch_pos + Point<int16_t>(16, 10)), alpha);
			}
		}

		version.draw(drawpos + Point<int16_t>(707, 1));
	}

	void UIWorldSelect::update()
	{
		UIElement::update();

		channel_selected.update();
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;
		auto drawpos = get_draw_position();

		// Click outside channel panel to deselect world
		if (world_selected)
		{
			Point<int16_t> panel_tl = drawpos + ch_panel_tl;
			Rectangle<int16_t> ch_bounds(
				panel_tl,
				panel_tl + Point<int16_t>(449, 233)
			);

			if (!ch_bounds.contains(cursorpos))
			{
				if (clicked)
				{
					world_selected = false;
					clear_selected_world();
				}
			}
		}

		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(drawpos).contains(cursorpos))
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
		if (!pressed)
			return;

		if (world_selected)
		{
			uint8_t channel_total = 0;
			for (auto& w : worlds)
			{
				if (w.wid == worldid)
				{
					channel_total = w.channelcount;
					break;
				}
			}

			constexpr uint8_t COLUMNS = 5;

			if (keycode == KeyAction::Id::UP)
			{
				int next = channelid - COLUMNS;
				if (next >= 0 && next < channel_total)
					button_pressed(BT_CHANNEL0 + next);
			}
			else if (keycode == KeyAction::Id::DOWN)
			{
				int next = channelid + COLUMNS;
				if (next < channel_total)
					button_pressed(BT_CHANNEL0 + next);
			}
			else if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::TAB)
			{
				if (channelid > 0)
					button_pressed(BT_CHANNEL0 + channelid - 1);
				else if (channel_total > 0)
					button_pressed(BT_CHANNEL0 + channel_total - 1);
			}
			else if (keycode == KeyAction::Id::RIGHT)
			{
				if (channelid < channel_total - 1)
					button_pressed(BT_CHANNEL0 + channelid + 1);
				else
					button_pressed(BT_CHANNEL0);
			}
			else if (escape)
			{
				world_selected = false;
				clear_selected_world();
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				enter_world();
			}
		}
		else
		{
			if (keycode == KeyAction::Id::RETURN)
			{
				// Press the currently highlighted world
				for (auto& w : worlds)
				{
					auto it = buttons.find(BT_WORLD0 + w.wid);
					if (it != buttons.end() && it->second->get_state() == Button::State::PRESSED)
					{
						button_pressed(BT_WORLD0 + w.wid);
						return;
					}
				}

				// No world pressed yet — select first
				if (!worlds.empty())
				{
					buttons[BT_WORLD0 + worlds[0].wid]->set_state(Button::State::PRESSED);
				}
			}
			else if (escape)
			{
				// Could open quit confirm here
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

		int16_t world_idx = 0;

		for (auto& world : worlds)
		{
			if (world.channelcount < 1)
				continue;

			std::string wid_str = std::to_string(world.wid);
			nl::node worldbtn = world_src[wid_str];

			if (worldbtn)
			{
				// v83 shroom95: 6 per row, 96px col stride, 26px row stride
				Point<int16_t> btn_pos = world_pos + Point<int16_t>((world_idx % 6) * 96, (world_idx / 6) * 26);

				buttons[BT_WORLD0 + world.wid] = std::make_unique<MapleButton>(
					worldbtn,
					btn_pos
				);
				buttons[BT_WORLD0 + world.wid]->set_active(true);
				world_idx++;
			}

			if (channelid >= world.channelcount)
				channelid = 0;
		}
	}

	void UIWorldSelect::add_world(World world)
	{
		// v83: per-world decoration from WorldSelect/world/0, world/1, etc.
		std::string wid_str = std::to_string(world.wid);
		nl::node world_node = world_select["world"][wid_str];
		world_textures.emplace_back(world_node);

		worlds.emplace_back(std::move(world));
		worldcount++;
	}

	void UIWorldSelect::add_recommended_world(RecommendedWorld)
	{
		// Not used in v83
	}

	void UIWorldSelect::change_world(World selectedWorld)
	{
		buttons[BT_WORLD0 + selectedWorld.wid]->set_state(Button::State::PRESSED);

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

		Sound(Sound::Name::SCROLLUP).play();

		world_selected = false;
		clear_selected_world();
		draw_chatballoon = false;
	}

	void UIWorldSelect::set_region(uint8_t regionid)
	{
		region = regionid;
		Setting<DefaultRegion>::get().save(region);

		// v83: world panel bg — use WorldSelect/chBackgrn (same sprite)
		worlds_background = world_select["chBackgrn"];
	}

	void UIWorldSelect::refresh_worlds()
	{
		clear_selected_world();
		world_selected = false;

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
			return Button::State::NORMAL;
		}
		else if (id == BT_QUITGAME)
		{
			// Could open quit confirm UI
			return Button::State::NORMAL;
		}
		else if (id == BT_VIEWALL)
		{
			return Button::State::NORMAL;
		}
		else if (id == BT_VIEWCHOICE)
		{
			return Button::State::NORMAL;
		}
		else if (id >= BT_WORLD0 && id < BT_CHANNEL0)
		{
			// Deselect old world
			if (buttons.count(BT_WORLD0 + worldid))
				buttons[BT_WORLD0 + worldid]->set_state(Button::State::NORMAL);

			worldid = static_cast<uint8_t>(id - BT_WORLD0);
			world_selected = true;

			clear_selected_world();

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
		else if (id >= BT_CHANNEL0 && id < BT_ENTERWORLD)
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

		for (size_t i = 0; i < 20; ++i)
		{
			if (buttons.count(BT_CHANNEL0 + i))
			{
				buttons[BT_CHANNEL0 + i]->set_state(Button::State::NORMAL);
				buttons[BT_CHANNEL0 + i]->set_active(false);
			}
		}

		if (buttons.count(BT_CHANNEL0))
			buttons[BT_CHANNEL0]->set_state(Button::State::PRESSED);

		if (buttons.count(BT_ENTERWORLD))
			buttons[BT_ENTERWORLD]->set_active(false);
	}
}
