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
#include "UICharSelect.h"

#include "UILoginNotice.h"
#include "UIRaceSelect.h"
#include "UISoftKey.h"
#include "UIWorldSelect.h"

#include "../UI.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../UIScale.h"

#include "../../Configuration.h"
#include "../../Constants.h"

#include "../../Audio/Audio.h"
#include "../../Character/Job.h"

#include "../../Net/Packets/SelectCharPackets.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UICharSelect::UICharSelect(std::vector<CharEntry> characters, int8_t characters_count, int32_t slots, int8_t require_pic) : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)), characters(characters), characters_count(characters_count), slots(slots), require_pic(require_pic)
	{
		burning_character = false;

		// Uniform content scale, anchored to the LEFT edge (the scene reads
		// left-to-right: treehouse -> bridge -> signpost; extra width on wide
		// screens stays open sky on the right)
		ui_scale = std::min(UIScale::scale_x(), UIScale::scale_y());
		box = Point<int16_t>(
			0,
			static_cast<int16_t>((UIScale::view_height() - 600.0f * ui_scale) / 2.0f));

		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		pagepos = Point<int16_t>(247, 462);
		// selected-world plate, top-left under the CHARACTER SELECT tab
		worldpos = Point<int16_t>(8, 90);
		// top-left of the info scroll that unrolls above the characters
		charinfopos = Point<int16_t>(150, 125);

		// The three buttons hang on the right signpost's planks
		// centered on the signboard's three plank bands (sign top-left 450,138;
		// bands at design y 150-187, 190-230, 239-283)
		Point<int16_t> character_sel_pos = Point<int16_t>(521, 154);
		Point<int16_t> character_new_pos = Point<int16_t>(521, 192);
		Point<int16_t> character_del_pos = Point<int16_t>(521, 240);

		selected_character = Setting<DefaultCharacter>::get().load();
		selected_page = selected_character / PAGESIZE;
		page_count = std::ceil((double)slots / (double)PAGESIZE);

		tab = nl::nx::ui["Basic.img"]["Cursor"]["18"]["0"];

		tab_index = 0;
		tab_active = false;
		tab_move = false;

		Point<int16_t> tab_adj = Point<int16_t>(86, 5);

		tab_pos[0] = character_sel_pos + tab_adj + Point<int16_t>(47, 3);
		tab_pos[1] = character_new_pos + tab_adj;
		tab_pos[2] = character_del_pos + tab_adj;

		tab_move_pos = 0;

		tab_map[0] = Buttons::CHARACTER_SELECT;
		tab_map[1] = Buttons::CHARACTER_NEW;
		tab_map[2] = Buttons::CHARACTER_DELETE;

		nl::node Login = nl::nx::ui["Login.img"];
		nl::node Common = Login["Common"];
		nl::node CharSelect = Login["CharSelect"];
		nl::node selectWorld = Common["selectWorld"];
		nl::node selectedWorld = CharSelect["selectedWorld"];
		nl::node pageNew = CharSelect["pageNew"];

		world_dimensions = Texture(selectWorld).get_dimensions();

		uint16_t world;
		uint8_t world_id = Configuration::get().get_worldid();
		uint8_t channel_id = Configuration::get().get_channelid();

		if (auto worldselect = UI::get().get_element<UIWorldSelect>())
			world = worldselect->get_worldbyid(world_id);

		world_sprites.emplace_back(selectWorld, DrawArgument(lay(worldpos), ui_scale, ui_scale));
		world_sprites.emplace_back(selectedWorld["icon"][world], DrawArgument(lay(worldpos - Point<int16_t>(12, -1)), ui_scale, ui_scale));
		world_sprites.emplace_back(selectedWorld["name"][world], DrawArgument(lay(worldpos - Point<int16_t>(8, 1)), ui_scale, ui_scale));
		world_sprites.emplace_back(selectedWorld["ch"][channel_id], DrawArgument(lay(worldpos - Point<int16_t>(0, 1)), ui_scale, ui_scale));

		// Authentic v83 character select scene, positions from the v83
		// MapLogin data (stage camera at 349,1568): sky gradient, the giant
		// tree with the mushroom house on the left, clouds, the log
		// rope-bridge the characters stand on, and the wooden signpost the
		// char info parchment pins to.
		nl::node backs = nl::nx::map["Back"]["login.img"]["back"];

		// sky gradient strip stretched over the whole view
		backdrop = Texture(backs["1"]);

		if (backdrop.is_valid())
		{
			Point<int16_t> borigin = backdrop.get_origin();
			backdrop_args = DrawArgument(
				borigin, borigin,
				Point<int16_t>(
					static_cast<int16_t>(UIScale::view_width()),
					static_cast<int16_t>(UIScale::view_height())),
				1.0f, 1.0f, 1.0f, 0.0f);
		}

		auto scenepc = [&](nl::node src, int16_t x, int16_t y)
		{
			if (src)
				sprites.emplace_back(src, DrawArgument(lay(x, y), ui_scale, ui_scale));
		};

		// All pieces native size, aligned to the bridge chunk (top-left
		// -100,313): its baked trunk/door continue the tree above and the
		// greenery below (seam runs match at these offsets exactly). The whole
		// welded group sits 100px off-screen left so the walkway starts near
		// the window edge.
		scenepc(backs["10"], 77, 32);     // mushroom house
		scenepc(backs["15"], 143, 81);    // tree top
		// 2px up so the bridge (drawn later) overlaps the seam — separate
		// rounding of the two sprites otherwise shows a sky line at y 473
		scenepc(backs["13"], 141, 695);   // tree base / greenery

		// Drifting clouds (drawn in draw() so they scroll): one behind the
		// scene, one low in front of the bridge.
		cloud_back = Texture(backs["3"]);
		cloud_front = Texture(backs["5"]);
		cloud_back_pos = Point<int16_t>(238, 71);
		cloud_front_pos = Point<int16_t>(301, 407);
		cloud_drift = 0.0f;

		// signpost planted on the bridge (behind it, so the deck hides the
		// pole base); its three planks carry the buttons
		scenepc(nl::nx::map["Obj"]["login.img"]["CharSelect"]["signboard"]["0"], 571, 257);

		// bridge drawn last so the deck rail covers the signpost pole and the
		// characters read as standing on the walkway (deck top y 374)
		scenepc(backs["14"], 315, 393);


		if (Common["step"]["2"])
			sprites.emplace_back(Common["step"]["2"], DrawArgument(lay(40, -6), ui_scale, ui_scale));

		burning_notice = Common["Burning"]["BurningNotice"];
		burning_count = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::CREAM, "1");

		charslot = CharSelect["charSlot"]["0"];
		pagebase = pageNew["base"]["0"];
		pagenumber = Charset(pageNew["number"], Charset::Alignment::LEFT);
		pagenumberpos = pageNew["numberpos"];

		// v83 info scroll: unroll animation + held-open frame + stat sheet,
		// short for explorers, tall for KoC/Aran
		info_unroll[0] = Animation(CharSelect["scroll"]["0"]);
		info_unroll[1] = Animation(CharSelect["scroll"]["2"]);
		info_unroll[0].set_hold_last(true);
		info_unroll[1].set_hold_last(true);
		info_open_tex[0] = Texture(CharSelect["scroll"]["0"]["3"]);
		info_open_tex[1] = Texture(CharSelect["scroll"]["2"]["3"]);
		info_sheet[0] = Texture(CharSelect["charInfo"]);
		info_sheet[1] = Texture(CharSelect["charInfo1"]);
		info_state = 0;
		info_tall = false;

		level_label = OutlinedText(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, Color::Name::TOBACCOBROWN);
		fame_label = OutlinedText(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, Color::Name::TOBACCOBROWN);
		rank_label = OutlinedText(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, Color::Name::TOBACCOBROWN);

		nametag = CharSelect["nameTag"];

		buttons[Buttons::CHARACTER_SELECT] = std::make_unique<MapleButton>(CharSelect["BtSelect"], lay(character_sel_pos));
		buttons[Buttons::CHARACTER_NEW] = std::make_unique<MapleButton>(CharSelect["BtNew"], lay(character_new_pos));
		buttons[Buttons::CHARACTER_DELETE] = std::make_unique<MapleButton>(CharSelect["BtDelete"], lay(character_del_pos));
		// v83 wooden arrow signs standing on the bridge walkway, flanking the
		// character slots (state 0 = normal, 1 = lit)
		// (arrow art is center-anchored via origin ~(43,37); base rests on the
		// deck line at y=373)
		buttons[Buttons::PAGELEFT] = std::make_unique<TwoSpriteButton>(
			CharSelect["pageL"]["0"]["0"], CharSelect["pageL"]["1"]["0"], lay(188, 337));
		buttons[Buttons::PAGERIGHT] = std::make_unique<TwoSpriteButton>(
			CharSelect["pageR"]["0"]["0"], CharSelect["pageR"]["1"]["0"], lay(499, 337));

		if (Common["BtChangePIC"])
		{
			buttons[Buttons::CHANGEPIC] = std::make_unique<MapleButton>(Common["BtChangePIC"], lay(0, 80));
			buttons[Buttons::CHANGEPIC]->set_active(false);
		}

		if (Login["WorldSelect"]["BtResetPIC"])
		{
			buttons[Buttons::RESETPIC] = std::make_unique<MapleButton>(Login["WorldSelect"]["BtResetPIC"], lay(0, 115));
			buttons[Buttons::RESETPIC]->set_active(false);
		}

		if (CharSelect["EditCharList"]["BtCharacter"])
		{
			buttons[Buttons::EDITCHARLIST] = std::make_unique<MapleButton>(CharSelect["EditCharList"]["BtCharacter"], lay(-1, 47));
			buttons[Buttons::EDITCHARLIST]->set_active(false);
		}

		if (Common["BtStart"])
		{
			// "Back to world select" — always visible, bottom-left (matches the
			// creation screens). It was being created inactive, so it never drew.
			buttons[Buttons::BACK] = std::make_unique<MapleButton>(Common["BtStart"], lay(-6, 515));
		}

		for (size_t i = 0; i < PAGESIZE; i++)
			buttons[Buttons::CHARACTER_SLOT0 + i] = std::make_unique<AreaButton>(
				lay(static_cast<int16_t>(253 + (72 * i)), 298),
				Point<int16_t>(
					static_cast<int16_t>(50 * ui_scale),
					static_cast<int16_t>(80 * ui_scale))
			);

		for (auto& btit : buttons)
			btit.second->set_scale(ui_scale);

		levelset = Charset(CharSelect["lv"], Charset::Alignment::CENTER);
		namelabel = OutlinedText(Text::Font::A14B, Text::Alignment::CENTER, Color::Name::WHITE, Color::Name::IRISHCOFFEE);

		for (size_t i = 0; i < InfoLabel::NUM_LABELS; i++)
			infolabels[i] = OutlinedText(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, Color::Name::TOBACCOBROWN);

		for (CharEntry& entry : characters)
		{
			charlooks.emplace_back(entry.look);
			nametags.emplace_back(nametag, Text::Font::A12M, entry.stats.name);
		}

		// Empty slots are the pulsing glow silhouettes standing on the deck
		emptyslot_effect = CharSelect["character"]["1"];

		selectedslot_effect[0] = CharSelect["effect"][0];
		selectedslot_effect[1] = CharSelect["effect"][1];

		charslotlabel = OutlinedText(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, Color::Name::JAMBALAYA);
		charslotlabel.change_text(get_slot_text());

		update_buttons();

		if (characters_count > 0)
		{
			if (selected_character < characters_count)
				update_selected_character();
			else
				select_last_slot();
		}

		if (Configuration::get().get_auto_login())
		{
			SelectCharPicPacket(
				Configuration::get().get_auto_pic(),
				Configuration::get().get_auto_cid()
			).dispatch();
		}

	}

	Point<int16_t> UICharSelect::lay(int16_t x, int16_t y) const
	{
		return box + Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	void UICharSelect::draw_cloud(const Texture& c, Point<int16_t> base, float speed) const
	{
		if (!c.is_valid())
			return;

		// Drift rightward and wrap: the cloud slides from off-screen-left
		// (-width) across to the right edge (800) over a span of width+800,
		// then repeats — a continuous scroll.
		int cw = c.width();
		int span = 800 + cw;
		int phase = base.x() + cw + static_cast<int>(cloud_drift * speed);
		int px = ((phase % span) + span) % span - cw;

		c.draw(DrawArgument(lay(static_cast<int16_t>(px), base.y()), ui_scale, ui_scale));
	}

	Point<int16_t> UICharSelect::lay(Point<int16_t> p) const
	{
		return lay(p.x(), p.y());
	}

	Point<int16_t> UICharSelect::scl(int16_t x, int16_t y) const
	{
		return Point<int16_t>(
			static_cast<int16_t>(x * ui_scale),
			static_cast<int16_t>(y * ui_scale));
	}

	Point<int16_t> UICharSelect::scl(Point<int16_t> p) const
	{
		return scl(p.x(), p.y());
	}

	void UICharSelect::draw(float inter) const
	{
		if (backdrop.is_valid())
		{
			// back/1 is a 20px-wide sky strip: tile it sideways at scene scale.
			// Stretching it across the window smears the columns into wide
			// vertical bands.
			Point<int16_t> o = backdrop.get_origin();
			int16_t vw = static_cast<int16_t>(UIScale::view_width());
			int16_t vh = static_cast<int16_t>(UIScale::view_height());
			int16_t tw = static_cast<int16_t>(backdrop.get_dimensions().x() * ui_scale);
			if (tw < 1)
				tw = 1;
			for (int16_t x = 0; x < vw; x = static_cast<int16_t>(x + tw))
			{
				Point<int16_t> p = o + Point<int16_t>(x, 0);
				backdrop.draw(DrawArgument(p, p, Point<int16_t>(tw, vh), 1.0f, 1.0f, 1.0f, 0.0f));
			}
		}

		// far cloud drifts behind the tree/bridge
		draw_cloud(cloud_back, cloud_back_pos, 1.0f);

		UIElement::draw_sprites(inter);

		// near cloud drifts (a touch faster) in front of the bridge
		draw_cloud(cloud_front, cloud_front_pos, 1.6f);

		Point<int16_t> drawpos(0, 0);

		version.draw(lay(707, 4));

		if (charslot.is_valid())
		{
			charslot.draw(DrawArgument(lay(589, 106 - charslot_y), ui_scale, ui_scale));
			charslotlabel.draw(lay(702, 111 - charslot_y));
		}

		for (Sprite sprite : world_sprites)
			sprite.draw(drawpos, inter);

		std::string total = pad_number_with_leading_zero(page_count);
		std::string current = pad_number_with_leading_zero(selected_page + 1);

		std::list<uint8_t> fliplist = { 3, 4, 5 };

		for (uint8_t i = 0; i < PAGESIZE; i++)
		{
			uint8_t index = i + selected_page * PAGESIZE;
			bool flip_character = std::find(fliplist.begin(), fliplist.end(), i) != fliplist.end();
			bool selectedslot = index == selected_character;

			Point<int16_t> feet = lay(get_character_slot_pos(i, 278, 374));

			if (index < characters_count)
			{
				float xs = flip_character ? -ui_scale : ui_scale;
				DrawArgument chararg(feet, feet, Point<int16_t>(0, 0), xs, ui_scale, 1.0f, 0.0f);

				nametags[index].draw(feet + scl(2, 6));

				const StatsEntry& character_stats = characters[index].stats;

				if (selectedslot)
				{
					// light streak from the sky down onto the character
					// (authored origins hang both pieces from this anchor)
					selectedslot_effect[1].draw(DrawArgument(
						feet + scl(0, -385), ui_scale, ui_scale), inter);

					// info scroll centered above the selected character:
					// unroll animation, then the stat sheet on the parchment
					Point<int16_t> scroll_at(
						static_cast<int16_t>(get_character_slot_pos(i, 278, 374).x() - 108),
						charinfopos.y());
					Point<int16_t> spos = lay(scroll_at);
					size_t t = info_tall ? 1 : 0;

					if (info_state == 1)
					{
						info_unroll[t].draw(DrawArgument(spos, ui_scale, ui_scale), inter);
					}
					else
					{
						info_open_tex[t].draw(DrawArgument(spos, ui_scale, ui_scale));

						Point<int16_t> ipos = lay(scroll_at + Point<int16_t>(17, 36));
						// the sheet art carries origin (45,57); compensate so
						// its top-left lands at ipos
						info_sheet[t].draw(DrawArgument(ipos + scl(45, 57), ui_scale, ui_scale));

						infolabels[InfoLabel::JOB].draw(ipos + scl(176, -1));
						level_label.draw(ipos + scl(88, 18));
						fame_label.draw(ipos + scl(176, 18));
						infolabels[InfoLabel::STR].draw(ipos + scl(88, 37));
						infolabels[InfoLabel::INT].draw(ipos + scl(176, 37));
						infolabels[InfoLabel::DEX].draw(ipos + scl(88, 56));
						infolabels[InfoLabel::LUK].draw(ipos + scl(176, 56));
						rank_label.draw(ipos + scl(92, 93));
					}
				}

				charlooks[index].draw(chararg, inter);

				// sunbeam glow over the character (hangs from the same sky
				// anchor as the streak via its authored origin)
				if (selectedslot)
					selectedslot_effect[0].draw(DrawArgument(
						feet + scl(0, -385), ui_scale, ui_scale), inter);
			}
			else if (i < slots)
			{
				// glow silhouette standing on the deck (feet-anchored art)
				emptyslot_effect.draw(DrawArgument(feet, ui_scale, ui_scale), inter);
			}
		}

		UIElement::draw_buttons(inter);

		if (tab_active)
			tab.draw(DrawArgument(
				lay(tab_pos[tab_index]) + Point<int16_t>(0, static_cast<int16_t>(tab_move_pos * ui_scale)),
				ui_scale, ui_scale));

		if (burning_character)
		{
			burning_notice.draw(DrawArgument(lay(190, 502), ui_scale, ui_scale), inter);
			burning_count.draw(lay(149, 464));
		}

		pagebase.draw(DrawArgument(lay(pagepos), ui_scale, ui_scale));
		pagenumber.draw(current.substr(0, 1), lay(pagepos + Point<int16_t>(pagenumberpos[0])));
		pagenumber.draw(current.substr(1, 1), lay(pagepos + Point<int16_t>(pagenumberpos[1])));
		pagenumber.draw(total.substr(0, 1), lay(pagepos + Point<int16_t>(pagenumberpos[2])));
		pagenumber.draw(total.substr(1, 1), lay(pagepos + Point<int16_t>(pagenumberpos[3])));
	}

	void UICharSelect::update()
	{
		UIElement::update();

		if (show_timestamp)
		{
			if (timestamp > 0)
			{
				timestamp -= Constants::TIMESTEP;

				if (timestamp <= 176)
					charslot_y += 1;
			}
		}
		else
		{
			if (timestamp <= 176)
			{
				timestamp += Constants::TIMESTEP;

				if (charslot_y >= 0)
					charslot_y -= 1;
			}
		}

		if (tab_move && tab_move_pos < 4)
			tab_move_pos += 1;

		if (tab_move && tab_move_pos == 4)
			tab_move = false;

		if (!tab_move && tab_move_pos > 0)
			tab_move_pos -= 1;

		for (CharLook& charlook : charlooks)
			charlook.update(Constants::TIMESTEP);

		for (Animation& effect : selectedslot_effect)
			effect.update();

		// info scroll unrolling -> settle open
		if (info_state == 1 && info_unroll[info_tall ? 1 : 0].update())
			info_state = 2;

		emptyslot_effect.update();

		cloud_drift += 0.35f; // slow rightward cloud scroll

		if (burning_character)
			burning_notice.update();
	}

	void UICharSelect::doubleclick(Point<int16_t> cursorpos)
	{
		uint16_t button_index = selected_character + Buttons::CHARACTER_SLOT0;
		auto& btit = buttons[button_index];

		if (btit->is_active() && btit->bounds(get_draw_position()).contains(cursorpos) && btit->get_state() == Button::State::NORMAL && button_index >= Buttons::CHARACTER_SLOT0)
			button_pressed(Buttons::CHARACTER_SELECT);
	}

	Cursor::State UICharSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		auto drawpos = get_draw_position();

		Rectangle<int16_t> charslot_bounds = Rectangle<int16_t>(
			lay(worldpos),
			lay(worldpos) + scl(world_dimensions)
			);

		if (charslot_bounds.contains(cursorpos))
		{
			if (clicked)
			{
				show_timestamp = !show_timestamp;

				return Cursor::State::CLICKING;
			}
		}

		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;

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

						if (tab_active && btit.first == tab_map[tab_index])
							btit.second->set_state(Button::State::MOUSEOVER);

						ret = Cursor::State::IDLE;
					}
					else
					{
						if (!tab_active || btit.first != tab_map[tab_index])
							ret = Cursor::State::CANCLICK;
					}
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				if (!tab_active || btit.first != tab_map[tab_index])
					btit.second->set_state(Button::State::NORMAL);
			}
		}

		return ret;
	}

	void UICharSelect::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				button_pressed(Buttons::BACK);
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				if (tab_active)
				{
					uint16_t btn_index = tab_map[tab_index];

					auto& btn = buttons[btn_index];
					Button::State state = btn->get_state();

					if (state != Button::State::DISABLED)
						button_pressed(btn_index);
				}
				else
				{
					button_pressed(Buttons::CHARACTER_SELECT);
				}
			}
			else
			{
				if (keycode == KeyAction::Id::TAB)
				{
					uint8_t prev_tab = tab_index;

					if (!tab_active)
					{
						tab_active = true;

						if (!buttons[Buttons::CHARACTER_SELECT]->is_active())
							tab_index++;
					}
					else
					{
						tab_index++;

						if (tab_index > 2)
						{
							tab_active = false;
							tab_index = 0;
						}
					}

					tab_move = true;
					tab_move_pos = 0;

					auto& prev_btn = buttons[tab_map[prev_tab]];
					Button::State prev_state = prev_btn->get_state();

					if (prev_state != Button::State::DISABLED)
						prev_btn->set_state(Button::State::NORMAL);

					if (tab_active)
					{
						auto& btn = buttons[tab_map[tab_index]];
						Button::State state = btn->get_state();

						if (state != Button::State::DISABLED)
							btn->set_state(Button::State::MOUSEOVER);
					}
				}
				else
				{
					uint8_t selected_index = selected_character;
					uint8_t index_total = std::min(characters_count, static_cast<int8_t>((selected_page + 1) * PAGESIZE));

					uint8_t COLUMNS = PAGESIZE;
					uint8_t columns = std::min(index_total, COLUMNS);

					uint8_t rows = std::floor((index_total - 1) / COLUMNS) + 1;

					int32_t current_col = 0;

					if (columns > 0)
					{
						div_t div = std::div(selected_index, columns);
						current_col = div.rem;
					}

					if (keycode == KeyAction::Id::UP)
					{
						uint8_t next_index = (selected_index - COLUMNS < 0 ? (selected_index - COLUMNS) + rows * COLUMNS : selected_index - COLUMNS);

						if (next_index == selected_character)
							return;

						if (next_index >= index_total)
							button_pressed(next_index - COLUMNS + Buttons::CHARACTER_SLOT0);
						else
							button_pressed(next_index + Buttons::CHARACTER_SLOT0);
					}
					else if (keycode == KeyAction::Id::DOWN)
					{
						uint8_t next_index = (selected_index + COLUMNS >= index_total ? current_col : selected_index + COLUMNS);

						if (next_index == selected_character)
							return;

						if (next_index > index_total)
							button_pressed(next_index + COLUMNS + Buttons::CHARACTER_SLOT0);
						else
							button_pressed(next_index + Buttons::CHARACTER_SLOT0);
					}
					else if (keycode == KeyAction::Id::LEFT)
					{
						if (selected_index != 0)
						{
							selected_index--;

							if (selected_index >= (selected_page + 1) * PAGESIZE - PAGESIZE)
								button_pressed(selected_index + Buttons::CHARACTER_SLOT0);
							else
								button_pressed(Buttons::PAGELEFT);
						}
					}
					else if (keycode == KeyAction::Id::RIGHT)
					{
						if (selected_index != characters_count - 1)
						{
							selected_index++;

							if (selected_index < index_total)
								button_pressed(selected_index + Buttons::CHARACTER_SLOT0);
							else
								button_pressed(Buttons::PAGERIGHT);
						}
					}
				}
			}
		}
	}

	UIElement::Type UICharSelect::get_type() const
	{
		return TYPE;
	}

	void UICharSelect::add_character(CharEntry&& character)
	{
		charlooks.emplace_back(character.look);
		nametags.emplace_back(nametag, Text::Font::A13M, character.stats.name);
		characters.emplace_back(std::forward<CharEntry>(character));

		characters_count++;
	}

	void UICharSelect::post_add_character()
	{
		bool page_matches = (characters_count - 1) / PAGESIZE == selected_page;

		if (!page_matches)
			button_pressed(Buttons::PAGERIGHT);

		update_buttons();

		if (characters_count > 1)
			select_last_slot();
		else
			update_selected_character();

		makeactive();

		charslotlabel.change_text(get_slot_text());
	}

	void UICharSelect::remove_character(int32_t id)
	{
		for (size_t i = 0; i < characters.size(); i++)
		{
			if (characters[i].id == id)
			{
				charlooks.erase(charlooks.begin() + i);
				nametags.erase(nametags.begin() + i);
				characters.erase(characters.begin() + i);

				characters_count--;

				if (selected_page > 0)
				{
					bool page_matches = (characters_count - 1) / PAGESIZE == selected_page;

					if (!page_matches)
						button_pressed(Buttons::PAGELEFT);
				}

				update_buttons();

				if (selected_character < characters_count)
					update_selected_character();
				else
					select_last_slot();

				return;
			}
		}
	}

	const CharEntry& UICharSelect::get_character(int32_t id)
	{
		for (CharEntry& character : characters)
			if (character.id == id)
				return character;

		static const CharEntry null_character = { {}, {}, 0 };

		return null_character;
	}

	Button::State UICharSelect::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::CHARACTER_SELECT:
			{
				if (characters.size() > 0)
				{
					Setting<DefaultCharacter>::get().save(selected_character);
					int32_t id = characters[selected_character].id;

					switch (require_pic)
					{
						case 0:
						{
							std::function<void()> onok = [&]()
							{
								request_pic();
							};

							UI::get().emplace<UILoginNotice>(UILoginNotice::Message::PIC_REQ, onok);
							break;
						}
						case 1:
						{
							std::function<void(const std::string&)> onok = [id](const std::string& pic)
							{
								SelectCharPicPacket(pic, id).dispatch();
							};

							UI::get().emplace<UISoftKey>(onok);
							break;
						}
						case 2:
						{
							SelectCharPacket(id).dispatch();
							break;
						}
					}
				}

				break;
			}
			case Buttons::CHARACTER_NEW:
			{
				Sound(Sound::Name::SCROLLUP).play();

				deactivate();

				tab_index = 0;
				tab_active = false;
				tab_move = false;
				tab_move_pos = 0;

				UI::get().emplace<UIRaceSelect>();
				break;
			}
			case Buttons::CHARACTER_DELETE:
			{
				int32_t id = characters[selected_character].id;

				switch (require_pic)
				{
					case 0:
					{
						std::function<void()> onok = [&]()
						{
							charslotlabel.change_text(get_slot_text());
						};

						UI::get().emplace<UILoginNotice>(UILoginNotice::Message::CHAR_DEL_FAIL_NO_PIC, onok);
						break;
					}
					case 1:
					{
						std::function<void()> oncancel = [&]()
						{
							charslotlabel.change_text(get_slot_text());
						};

						std::function<void()> onok = [&, id, oncancel]()
						{
							std::function<void(const std::string&)> onok = [&, id](const std::string& pic)
							{
								DeleteCharPicPacket(pic, id).dispatch();
								charslotlabel.change_text(get_slot_text());
							};

							UI::get().emplace<UISoftKey>(onok, oncancel);
						};

						const StatsEntry& character_stats = characters[selected_character].stats;
						uint16_t cjob = character_stats.stats[MapleStat::Id::JOB];

						if (cjob < 1000)
							UI::get().emplace<UILoginNotice>(UILoginNotice::Message::DELETE_CONFIRMATION, onok, oncancel);
						else
							UI::get().emplace<UILoginNotice>(UILoginNotice::Message::CASH_ITEMS_CONFIRM_DELETION, onok);

						break;
					}
					case 2:
					{
						DeleteCharPacket(id).dispatch();
						charslotlabel.change_text(get_slot_text());
						break;
					}
				}

				break;
			}
			case Buttons::PAGELEFT:
			{
				uint8_t previous_page = selected_page;

				if (selected_page > 0)
					selected_page--;
				else if (characters_count > PAGESIZE)
					selected_page = page_count - 1;

				if (previous_page != selected_page)
					update_buttons();

				select_last_slot();
				break;
			}
			case Buttons::PAGERIGHT:
			{
				uint8_t previous_page = selected_page;

				if (selected_page < page_count - 1)
					selected_page++;
				else
					selected_page = 0;

				if (previous_page != selected_page)
				{
					update_buttons();

					button_pressed(Buttons::CHARACTER_SLOT0);
				}

				break;
			}
			case Buttons::CHANGEPIC:
			{
				break;
			}
			case Buttons::RESETPIC:
			{
				std::string url = Configuration::get().get_resetpic();

#ifdef _WIN32
				ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
				break;
			}
			case Buttons::EDITCHARLIST:
			{
				break;
			}
			case Buttons::BACK:
			{
				deactivate();

				Sound(Sound::Name::SCROLLUP).play();

				if (auto worldselect = UI::get().get_element<UIWorldSelect>())
					worldselect->makeactive();

				break;
			}
			default:
			{
				if (buttonid >= Buttons::CHARACTER_SLOT0)
				{
					uint8_t previous_character = selected_character;
					selected_character = buttonid - Buttons::CHARACTER_SLOT0 + selected_page * PAGESIZE;

					if (previous_character != selected_character)
					{
						if (previous_character < characters_count)
						{
							charlooks[previous_character].set_stance(Stance::Id::STAND1);
							nametags[previous_character].set_selected(false);
						}

						if (selected_character < characters_count)
							update_selected_character();
					}
				}

				break;
			}
		}

		return Button::State::NORMAL;
	}

	void UICharSelect::update_buttons()
	{
		for (uint8_t i = 0; i < PAGESIZE; i++)
		{
			uint8_t index = i + selected_page * PAGESIZE;

			if (index < characters_count)
				buttons[Buttons::CHARACTER_SLOT0 + i]->set_state(Button::State::NORMAL);
			else
				buttons[Buttons::CHARACTER_SLOT0 + i]->set_state(Button::State::DISABLED);
		}

		if (characters_count >= slots)
			buttons[Buttons::CHARACTER_NEW]->set_state(Button::State::DISABLED);
		else
			buttons[Buttons::CHARACTER_NEW]->set_state(Button::State::NORMAL);

		bool character_found = false;

		for (int8_t i = PAGESIZE - 1; i >= 0; i--)
		{
			uint8_t index = i + selected_page * PAGESIZE;

			if (index < characters_count)
			{
				character_found = true;

				break;
			}
		}

		buttons[Buttons::CHARACTER_SELECT]->set_active(character_found);
		buttons[Buttons::CHARACTER_DELETE]->set_state(character_found ? Button::State::NORMAL : Button::State::DISABLED);
	}

	void UICharSelect::update_selected_character()
	{
		Sound(Sound::Name::CHARSELECT).play();

		charlooks[selected_character].set_stance(Stance::Id::WALK1);
		nametags[selected_character].set_selected(true);

		const StatsEntry& character_stats = characters[selected_character].stats;

		namelabel.change_text(character_stats.name);

		for (size_t i = 0; i < InfoLabel::NUM_LABELS; i++)
			infolabels[i].change_text(get_infolabel(i, character_stats));

		level_label.change_text(std::to_string(character_stats.stats[MapleStat::Id::LEVEL]));
		fame_label.change_text(std::to_string(character_stats.stats[MapleStat::Id::FAME]));

		int32_t rank = character_stats.rank.first;
		rank_label.change_text(rank > 0 ? std::to_string(rank) : "--");

		// replay the info scroll unroll, tall parchment for KoC/Aran
		uint16_t job = character_stats.stats[MapleStat::Id::JOB];
		info_tall = job >= 1000;
		info_state = 1;
		info_unroll[info_tall ? 1 : 0].reset();
	}

	void UICharSelect::select_last_slot()
	{
		for (int8_t i = PAGESIZE - 1; i >= 0; i--)
		{
			uint8_t index = i + selected_page * PAGESIZE;

			if (index < characters_count)
			{
				button_pressed(i + Buttons::CHARACTER_SLOT0);

				return;
			}
		}
	}

	std::string UICharSelect::get_slot_text()
	{
		show_timestamp = true;
		timestamp = 7 * 1000;
		charslot_y = 0;

		return pad_number_with_leading_zero(characters_count) + "/" + pad_number_with_leading_zero(slots);
	}

	std::string UICharSelect::pad_number_with_leading_zero(uint8_t value) const
	{
		std::string return_val = std::to_string(value);
		return_val.insert(return_val.begin(), 2 - return_val.length(), '0');

		return return_val;
	}

	Point<int16_t> UICharSelect::get_character_slot_pos(size_t index, uint16_t x_adj, uint16_t y_adj) const
	{
		// Characters stand in one row on the bridge walkway between the arrows
		int16_t x = static_cast<int16_t>(72 * index);

		return Point<int16_t>(x + x_adj, y_adj);
	}

	Point<int16_t> UICharSelect::get_infolabel_pos(size_t index) const
	{
		switch (index)
		{
			case InfoLabel::JOB:
				return Point<int16_t>(72, -48);
			case InfoLabel::STR:
				return Point<int16_t>(-5, 26);
			case InfoLabel::DEX:
				return Point<int16_t>(-5, 48);
			case InfoLabel::INT:
				return Point<int16_t>(72, 26);
			case InfoLabel::LUK:
				return Point<int16_t>(72, 48);
			case InfoLabel::NUM_LABELS:
				break;
			default:
				break;
		}

		return Point<int16_t>();
	}

	std::string UICharSelect::get_infolabel(size_t index, StatsEntry character_stats) const
	{
		switch (index)
		{
			case InfoLabel::JOB:
				return Job(character_stats.stats[MapleStat::Id::JOB]).get_name();
			case InfoLabel::STR:
				return std::to_string(character_stats.stats[MapleStat::Id::STR]);
			case InfoLabel::DEX:
				return std::to_string(character_stats.stats[MapleStat::Id::DEX]);
			case InfoLabel::INT:
				return std::to_string(character_stats.stats[MapleStat::Id::INT]);
			case InfoLabel::LUK:
				return std::to_string(character_stats.stats[MapleStat::Id::LUK]);
			case InfoLabel::NUM_LABELS:
				break;
			default:
				break;
		}

		return "";
	}

	void UICharSelect::request_pic()
	{
		std::function<void(const std::string&)> enterpic = [&](const std::string& entered_pic)
		{
			std::function<void(const std::string&)> verifypic = [&, entered_pic](const std::string& verify_pic)
			{
				if (entered_pic == verify_pic)
				{
					RegisterPicPacket(
						characters[selected_character].id,
						entered_pic
					).dispatch();
				}
				else
				{
					std::function<void()> onreturn = [&]()
					{
						request_pic();
					};

					UI::get().emplace<UILoginNotice>(UILoginNotice::Message::PASSWORD_IS_INCORRECT, onreturn);
				}
			};

			UI::get().emplace<UISoftKey>(verifypic, []() {}, "Please re-enter your new PIC.", Point<int16_t>(24, 0));
		};

		UI::get().emplace<UISoftKey>(enterpic, []() {}, "Your new PIC must at least be 6 characters long.", Point<int16_t>(24, 0));
	}
}