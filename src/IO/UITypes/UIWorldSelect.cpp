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

#include <algorithm>
#include <cctype>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/bitmap.hpp>
#endif

namespace ms
{
	// Authentic v83 world select (design coords): the world buttons ARE the
	// hanging wooden planks (BtWorld/N, 26x95, animated on hover), hung from
	// the rod's hooks in front of the rolled scroll. Picking a world plays
	// scroll/0 (the parchment unrolls to 513x401); the world title, channel
	// buttons and Enter World draw on the open parchment. Picking the same
	// world again plays scroll/1 (rolls closed).
	namespace
	{
		// Scene anchors
		constexpr Point<int16_t> HANGER_POS(400, 80);
		constexpr Point<int16_t> STEP_POS(40, 0);

		// Scroll frames draw from this fixed top-left (origin 0,0 art)
		constexpr Point<int16_t> SCROLL_POS(143, 100);

		// World planks hang at the rod's rope ends: measured from the rod
		// art, the rope tails sit at design x = 141 + 27k (ending y ~165);
		// the plank's top knot is 13px in from its left edge. The first
		// plank hangs from the third rope.
		constexpr Point<int16_t> WORLD_POS(182, 158);
		constexpr int16_t WORLD_STRIDE = 27;

		// Content on the open parchment, relative to SCROLL_POS: the
		// chBackgrn sheet (which carries the divider line), the world title
		// above the divider, the channel grid below it, Enter Channel at the
		// parchment's bottom right
		constexpr Point<int16_t> CHBACK_OFF(32, 118);     // sheet top-left (art origin 224,116)
		constexpr Point<int16_t> TITLE_OFF(52, 140);      // world/N decoration
		constexpr Point<int16_t> EVENT_TITLE_OFF(185, 146);
		constexpr int16_t CH_COLS = 4;
		constexpr Point<int16_t> CH_GRID_OFF(55, 205);    // channel button grid
		constexpr Point<int16_t> CH_STRIDE(106, 35);
		constexpr Point<int16_t> CHSELECT_OFF(46, 15);    // highlight anchor in a button
		constexpr Point<int16_t> GOWORLD_OFF(357, 325);   // Enter Channel button

		// EVENT balloon above an event-flagged world's plank
		constexpr Point<int16_t> EVENT_PLANK_OFF(-1, -16);

		// Population gauge over a channel button's bar area (the gauge art
		// has origin (36,4); the stretch width encodes the load fraction).
		// The fill is relative to the busiest channel, with a floor so a
		// nearly-empty server still shows sensible slivers.
		constexpr Point<int16_t> GAUGE_OFF(46, 19);
		constexpr int16_t GAUGE_W = 73;
		constexpr int32_t GAUGE_MIN_CAP = 20;

		// EVENT balloon bouncing over an event-running channel button. A
		// channel counts as event-running when its world has flag == 1, or
		// when the server sets bit 30 on that channel's load in the
		// serverlist packet (per-channel switch; the bit is stripped before
		// the load fills the gauge).
		constexpr Point<int16_t> CH_EVENT_OFF(60, -14);
		constexpr int32_t CH_EVENT_BIT = 0x40000000;

		// Scroll open/close state machine
		enum ScrollState : int8_t
		{
			SCROLL_CLOSED,
			SCROLL_OPENING,
			SCROLL_OPEN,
			SCROLL_CLOSING
		};
	}

	UIWorldSelect::UIWorldSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600))
	{
		worldid = Setting<DefaultWorld>::get().load();
		channelid = Setting<DefaultChannel>::get().load();
		region = Setting<DefaultRegion>::get().load();
		worldcount = 0;
		world_selected = false;

		// The whole 800x600 design renders scaled uniformly, centered in the
		// view, so the composition survives large/4K resolutions intact.
		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		box = Point<int16_t>(
			static_cast<int16_t>((UIScale::view_width() - 800.0f * ui_scale) / 2.0f),
			static_cast<int16_t>((UIScale::view_height() - 600.0f * ui_scale) / 2.0f));

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		world_pos = WORLD_POS;

		nl::node login = nl::nx::ui["Login.img"];
		world_select = login["WorldSelect"];
		world_src = world_select["BtWorld"];
		channel_src = world_select["channel"];
		nl::node common = login["Common"];
		nl::node ws_obj = nl::nx::map["Obj"]["login.img"]["WorldSelect"];
		nl::node neo = ws_obj["neoCity"];

		// City background: ONE copy of the scene, cover-scaled so it fills
		// the whole view with no seams or duplicated features. The vertical
		// overflow is cropped 75% off the top (sky) / 25% off the bottom so
		// the skyline stays in view.
		backdrop = Texture(neo["0"]["0"]);

		if (backdrop.is_valid())
		{
			float cover = std::max(UIScale::scale_x(), UIScale::scale_y());
			float bsx = cover * 800.0f / backdrop.width();
			float bsy = cover * 600.0f / backdrop.height();
			float bh = backdrop.height() * bsy;
			float btop = -0.75f * (bh - UIScale::view_height());
			Point<int16_t> borigin = backdrop.get_origin();

			backdrop_args = DrawArgument(
				Point<int16_t>(
					static_cast<int16_t>(UIScale::view_width() / 2),
					static_cast<int16_t>(btop + bsy * borigin.y())),
				bsx, bsy);
		}

		// Scene set pieces: the winged mascot on the rocks (static back layer
		// + animated overlay, offset measured from the art), blinking eyes in
		// the dark city, twinkling sparkles, and the NeoCity logo.
		auto scene = [&](nl::node src, int16_t x, int16_t y)
		{
			if (src)
				sprites.emplace_back(src, DrawArgument(lay(x, y), ui_scale, ui_scale));
		};

		scene(neo["2"], 655, 470);
		scene(neo["1"], 655 + 48, 470 + 22);
		scene(neo["3"], 90, 555);
		scene(neo["4"], 200, 480);
		scene(neo["5"], 310, 505);

		// UI frame stretched over the full view (window dressing). Placed at
		// the view center explicitly — bg_args() assumes a CENTER_OFFSET
		// element, but this element draws from (0,0).
		sprites.emplace_back(common["frame"], DrawArgument(
			Point<int16_t>(UIScale::view_width() / 2, UIScale::view_height() / 2),
			UIScale::scale_x(), UIScale::scale_y()));

		// The rope hanger and step indicator draw over the scroll in draw()
		hanger = Texture(ws_obj["signboard"]["0"]["0"]);
		step_tex = Texture(common["step"]["1"]);

		// The v83 scroll: closed frame, open/close animations, held-open frame
		scroll_state = SCROLL_CLOSED;
		scroll_closed_tex = Texture(world_select["scroll"]["0"]["0"]);
		scroll_open_tex = Texture(world_select["scroll"]["0"]["3"]);
		scroll_opening = Animation(world_select["scroll"]["0"]);
		scroll_closing = Animation(world_select["scroll"]["1"]);

		// World title decoration shown on the open parchment
		world_title_src = world_select["world"];

		// Channel selection highlight: a burst that plays once and settles
		// on its full glow; it replays when the picked channel changes
		if (channel_src["chSelect"]["3"])
		{
			channel_selected = Animation(channel_src["chSelect"]);
			channel_selected.set_hold_last(true);
		}

		channel_gauge = Texture(channel_src["chgauge"]);
		focused_world = -1;

		// Bouncing EVENT balloon, shown for worlds the server flags with an
		// event (serverlist flag == 1)
		if (channel_src["chEvent"])
			event_badge = Animation(channel_src["chEvent"]);

		// View All / View Choice buttons
		buttons[BT_VIEWALL] = std::make_unique<MapleButton>(world_select["BtViewAll"], lay(8, 53));
		buttons[BT_VIEWCHOICE] = std::make_unique<MapleButton>(world_select["BtViewChoice"], lay(8, 53));
		buttons[BT_VIEWALL]->set_active(false);
		buttons[BT_VIEWCHOICE]->set_active(true);

		// Quit game button
		if (common["BtExit"])
			buttons[BT_QUITGAME] = std::make_unique<MapleButton>(common["BtExit"], lay(8, 515));

		// The divider sheet drawn on the open parchment
		chback = Texture(world_select["chBackgrn"]);

		// Channel buttons on the open parchment
		for (uint8_t i = 0; i < 20; ++i)
		{
			nl::node chnode = channel_src[std::to_string(i)];

			if (chnode)
			{
				Point<int16_t> pos = SCROLL_POS + CH_GRID_OFF + Point<int16_t>(
					(i % CH_COLS) * CH_STRIDE.x(), (i / CH_COLS) * CH_STRIDE.y());

				buttons[BT_CHANNEL0 + i] = std::make_unique<TwoSpriteButton>(
					chnode["normal"], chnode["disabled"], lay(pos));
				buttons[BT_CHANNEL0 + i]->set_active(false);
			}
		}

		// Enter world button on the parchment bottom
		buttons[BT_ENTERWORLD] = std::make_unique<MapleButton>(
			world_select["BtGoworld"],
			lay(SCROLL_POS + GOWORLD_OFF)
		);
		buttons[BT_ENTERWORLD]->set_active(false);

		// All login-flow buttons render at the content scale
		for (auto& btit : buttons)
			btit.second->set_scale(ui_scale);

		set_region(region);
	}

	Point<int16_t> UIWorldSelect::lay(int16_t x, int16_t y) const
	{
		return box + Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	Point<int16_t> UIWorldSelect::lay(Point<int16_t> p) const
	{
		return lay(p.x(), p.y());
	}

	void UIWorldSelect::draw(float alpha) const
	{
		// Dimmed full-view backdrop behind everything
		if (backdrop.is_valid())
			backdrop.draw(backdrop_args);

		UIElement::draw_sprites(alpha);

		// The v83 scroll: closed frame / opening animation / held-open frame
		// with the world title, channel grid and highlight / closing animation
		DrawArgument scroll_args(lay(SCROLL_POS), ui_scale, ui_scale);

		switch (scroll_state)
		{
		case SCROLL_OPENING:
			scroll_opening.draw(scroll_args, alpha);
			break;
		case SCROLL_CLOSING:
			scroll_closing.draw(scroll_args, alpha);
			break;
		case SCROLL_OPEN:
		{
			scroll_open_tex.draw(scroll_args);

			// divider sheet, world title above the line, EVENT balloon for
			// event-flagged worlds
			chback.draw(DrawArgument(
				lay(SCROLL_POS + CHBACK_OFF + chback.get_origin()), ui_scale, ui_scale));
			world_title.draw(DrawArgument(lay(SCROLL_POS + TITLE_OFF), ui_scale, ui_scale));

			for (auto& w : worlds)
				if (w.wid == worldid && w.flag == 1)
					event_badge.draw(DrawArgument(
						lay(SCROLL_POS + EVENT_TITLE_OFF), ui_scale, ui_scale), alpha);

			break;
		}
		case SCROLL_CLOSED:
		default:
			scroll_closed_tex.draw(scroll_args);
			break;
		}

		hanger.draw(DrawArgument(lay(HANGER_POS), ui_scale, ui_scale));
		step_tex.draw(DrawArgument(lay(STEP_POS), ui_scale, ui_scale));

		// EVENT balloon above an event-flagged world's hanging plank
		{
			int16_t idx = 0;

			for (auto& w : worlds)
			{
				if (w.channelcount < 1)
					continue;

				if (w.flag == 1)
					event_badge.draw(DrawArgument(
						lay(world_pos + Point<int16_t>(idx * WORLD_STRIDE, 0) + EVENT_PLANK_OFF),
						ui_scale, ui_scale), alpha);

				idx++;
			}
		}

		UIElement::draw_buttons(alpha);

		if (scroll_state == SCROLL_OPEN)
		{
			// Population gauge over each channel's bar, filled by the channel
			// load the server reported (relative to the busiest channel)
			for (auto& w : worlds)
			{
				if (w.wid != worldid)
					continue;

				bool world_event = w.flag == 1;
				int32_t cap = GAUGE_MIN_CAP;

				for (int32_t load : w.chloads)
					cap = std::max(cap, load & ~CH_EVENT_BIT);

				for (uint8_t c = 0; c < w.channelcount && c < 20; ++c)
				{
					int32_t rawload = c < w.chloads.size() ? w.chloads[c] : 0;
					int32_t load = rawload & ~CH_EVENT_BIT;
					Point<int16_t> chpos = SCROLL_POS + CH_GRID_OFF + Point<int16_t>(
						(c % CH_COLS) * CH_STRIDE.x(), (c / CH_COLS) * CH_STRIDE.y());

					float frac = std::min(load / float(cap), 1.0f);
					int16_t gw = static_cast<int16_t>(GAUGE_W * frac);

					if (gw >= 2)
					{
						Point<int16_t> gpos = lay(chpos + GAUGE_OFF);
						channel_gauge.draw(DrawArgument(
							gpos, gpos, Point<int16_t>(gw, 0),
							ui_scale, ui_scale, 1.0f, 0.0f));
					}

					// EVENT balloon on channels running an event
					if (world_event || (rawload & CH_EVENT_BIT) != 0)
						event_badge.draw(DrawArgument(
							lay(chpos + CH_EVENT_OFF), ui_scale, ui_scale), alpha);
				}

				break;
			}

			// Selection highlight over the picked channel button
			Point<int16_t> chpos = SCROLL_POS + CH_GRID_OFF + Point<int16_t>(
				(channelid % CH_COLS) * CH_STRIDE.x(), (channelid / CH_COLS) * CH_STRIDE.y());
			channel_selected.draw(DrawArgument(
				lay(chpos + CHSELECT_OFF), ui_scale, ui_scale), alpha);
		}

		version.draw(lay(707, 1));
	}

	void UIWorldSelect::update()
	{
		UIElement::update();

		channel_selected.update();
		event_badge.update();

		// Advance the scroll animation; when a full pass ends, settle into
		// the target state (channels appear once fully open)
		if (scroll_state == SCROLL_OPENING)
		{
			if (scroll_opening.update())
			{
				scroll_state = SCROLL_OPEN;
				show_channels();
			}
		}
		else if (scroll_state == SCROLL_CLOSING)
		{
			if (scroll_closing.update())
				scroll_state = SCROLL_CLOSED;
		}
	}

	void UIWorldSelect::show_channels()
	{
		channel_selected.reset();

		for (auto& w : worlds)
		{
			if (w.wid == worldid)
			{
				for (size_t i = 0; i < w.channelcount; ++i)
				{
					if (buttons.count(BT_CHANNEL0 + i))
					{
						buttons[BT_CHANNEL0 + i]->set_active(true);

						if (i == channelid)
							buttons[BT_CHANNEL0 + i]->set_state(Button::State::PRESSED);
					}
				}

				buttons[BT_ENTERWORLD]->set_active(true);
				break;
			}
		}
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;
		auto drawpos = get_draw_position();

		// Click outside the open scroll to roll it closed
		if (world_selected && scroll_state == SCROLL_OPEN)
		{
			Rectangle<int16_t> scroll_bounds(
				lay(SCROLL_POS - Point<int16_t>(0, 45)),
				lay(SCROLL_POS + Point<int16_t>(513, 401)));

			if (!scroll_bounds.contains(cursorpos) && clicked)
			{
				world_selected = false;
				clear_selected_world();
				scroll_state = SCROLL_CLOSING;
				scroll_closing.reset();
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
				else if (btit.second->get_state() == Button::State::PRESSED ||
					btit.second->get_state() == Button::State::KEYFOCUSED)
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

			constexpr uint8_t COLUMNS = CH_COLS;

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
				scroll_state = SCROLL_CLOSING;
				scroll_closing.reset();
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				enter_world();
			}
		}
		else
		{
			// Keyboard navigation over the hanging world planks: LEFT/RIGHT/
			// TAB move the key focus (the plank's keyFocused art), RETURN
			// picks the focused world.
			if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::RIGHT ||
				keycode == KeyAction::Id::TAB)
			{
				if (worlds.empty())
					return;

				int16_t count = static_cast<int16_t>(worlds.size());
				int16_t dir = keycode == KeyAction::Id::LEFT ? -1 : 1;

				if (focused_world >= 0 && focused_world < count)
				{
					auto it = buttons.find(BT_WORLD0 + worlds[focused_world].wid);

					if (it != buttons.end())
						it->second->set_state(Button::State::NORMAL);
				}

				focused_world = focused_world < 0
					? (dir > 0 ? 0 : count - 1)
					: static_cast<int16_t>((focused_world + dir + count) % count);

				auto it = buttons.find(BT_WORLD0 + worlds[focused_world].wid);

				if (it != buttons.end())
					it->second->set_state(Button::State::KEYFOCUSED);
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				if (focused_world >= 0 && focused_world < static_cast<int16_t>(worlds.size()))
					button_pressed(BT_WORLD0 + worlds[focused_world].wid);
				else if (!worlds.empty())
					button_pressed(BT_WORLD0 + worlds[0].wid);
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
				// One hanging plank per world along the rod's hooks
				Point<int16_t> btn_pos = world_pos + Point<int16_t>(world_idx * WORLD_STRIDE, 0);

				buttons[BT_WORLD0 + world.wid] = std::make_unique<MapleButton>(
					worldbtn,
					lay(btn_pos)
				);
				buttons[BT_WORLD0 + world.wid]->set_scale(ui_scale);
				buttons[BT_WORLD0 + world.wid]->set_active(true);
				world_idx++;
			}

			if (channelid >= world.channelcount)
				channelid = 0;
		}
	}

	void UIWorldSelect::add_world(World world)
	{
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

		// The world title on the open parchment
		world_title = Texture(world_title_src[std::to_string(selectedWorld.wid)]);

		// Channels activate in show_channels() once the scroll finishes
		// opening; when it is already open (switching worlds), immediately.
		if (scroll_state == SCROLL_OPEN)
			show_channels();
		else if (scroll_state != SCROLL_OPENING)
		{
			scroll_state = SCROLL_OPENING;
			scroll_opening.reset();
		}
	}

	void UIWorldSelect::remove_selected()
	{
		deactivate();

		Sound(Sound::Name::SCROLLUP).play();

		world_selected = false;
		clear_selected_world();
		scroll_state = SCROLL_CLOSED;
	}

	void UIWorldSelect::set_region(uint8_t regionid)
	{
		region = regionid;
		Setting<DefaultRegion>::get().save(region);
	}

	void UIWorldSelect::refresh_worlds()
	{
		world_selected = false;
		clear_selected_world();
		scroll_state = SCROLL_CLOSED;

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
			UI::get().quit();
			return Button::State::NORMAL;
		}
		else if (id == BT_VIEWALL)
		{
			buttons[BT_VIEWALL]->set_active(false);
			buttons[BT_VIEWCHOICE]->set_active(true);
			return Button::State::NORMAL;
		}
		else if (id == BT_VIEWCHOICE)
		{
			buttons[BT_VIEWCHOICE]->set_active(false);
			buttons[BT_VIEWALL]->set_active(true);
			return Button::State::NORMAL;
		}
		else if (id >= BT_WORLD0 && id < BT_CHANNEL0)
		{
			// Pressing the already-selected world again rolls the scroll
			// back up
			if (world_selected && id == BT_WORLD0 + worldid)
			{
				world_selected = false;
				clear_selected_world();
				scroll_state = SCROLL_CLOSING;
				scroll_closing.reset();

				return Button::State::NORMAL;
			}

			// Deselect old world
			if (buttons.count(BT_WORLD0 + worldid))
				buttons[BT_WORLD0 + worldid]->set_state(Button::State::NORMAL);

			worldid = static_cast<uint8_t>(id - BT_WORLD0);
			world_selected = true;
			focused_world = -1;

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

				channel_selected.reset();
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
