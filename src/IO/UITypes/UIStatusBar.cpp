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
#include "UIStatusBar.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "../../Configuration.h"
#include "../../Graphics/Geometry.h"
#include "UIBuddyList.h"
#include "UIChannel.h"
#include "UIEquipInventory.h"
#include "UIEvent.h"
#include "UIStatusMessenger.h"
#include "UIItemInventory.h"
#include "UIJoypad.h"
#include "UIKeyConfig.h"
#include "UIQuestLog.h"
#include "UINotice.h"
#include "UINotificationList.h"
#include "UIReport.h"

#include "../../Net/Packets/SocialPackets.h"
#include "../../Net/Packets/PlayerPackets.h"
#include "../../Data/SkillData.h"
#include "../../Data/ItemData.h"
#include "../Keyboard.h"
#include "UIOptionMenu.h"
#include "UIQuit.h"
#include "UISkillBook.h"
#include "UIStatsInfo.h"
#include "UIWorldMap.h"
#include "UIGuild.h"
#include "UIRanking.h"
#include "UIMonsterBook.h"
#include "UISystemOption.h"
#include "UIChat.h"
#include "UIFarmChat.h"
#include "UIWhisper.h"

#include "../../Net/OutPacket.h"
#include "../../Net/Session.h"
#include "../../IO/Window.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"


#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIStatusBar::UIStatusBar(const CharStats& st) : stats(st)
	{
		int16_t VWIDTH = Constants::Constants::get().get_viewwidth();
		int16_t VHEIGHT = Constants::Constants::get().get_viewheight();

		position = Point<int16_t>(512, VHEIGHT);
		dimension = Point<int16_t>(std::max<int16_t>(1366, VWIDTH), 84);

		show_menu = false;
		show_system = false;
		show_quickslot = false;
		// menu_bg and sys_bg are initialized after sub-panel buttons are created (sizes computed there)
		chat_open = false;
		chat_target_id = 0;

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];
		nl::node chat = nl::nx::ui["StatusBar2.img"]["chat"];

		has_notification = false;
		notice_pulse_tick = 0;

		// Flash-on-decrease trackers start aligned with the current values so
		// we don't spuriously flash on the first frame.
		prev_hp_pct = gethppercent();
		prev_mp_pct = getmppercent();
		prev_exp_pct = getexppercent();
		hp_flash_ticks = 0;
		mp_flash_ticks = 0;
		exp_flash_ticks = 0;

		// === Background ===
		bar_backgrnd = Texture(mainbar["backgrnd"]);
		sprites.emplace_back(mainbar["gaugeBackgrd"]);
		notice_sprite = Texture(mainbar["notice"]);
		sprites.emplace_back(mainbar["lvBacktrnd"]);
		sprites.emplace_back(mainbar["lvCover"]);

		// === Gauge cover (overlay on top of gauge area) ===
		gauge_cover = Texture(mainbar["gaugeCover"]);

		// === Class-variant gauge backgrounds ===
		gauge_backgrd_ab = Texture(mainbar["gaugeBackgrdAB"]);
		gauge_backgrd_demon = Texture(mainbar["gaugeBackgrdDemon"]);
		gauge_backgrd_kanna = Texture(mainbar["gaugeBackgrdKanna"]);
		gauge_backgrd_zero = Texture(mainbar["gaugeBackgrdZero"]);
		gauge_cover_ab = Texture(mainbar["gaugeCoverAB"]);
		lv_backtrnd_sao = Texture(mainbar["lvBacktrndSao"]);

		// === Main gauges ===
		expbar = Gauge(
			Gauge::Type::GAME,
			mainbar.resolve("gauge/exp/0"),
			mainbar.resolve("gauge/exp/1"),
			mainbar.resolve("gauge/exp/2"),
			308, 0.0f
		);
		hpbar = Gauge(
			Gauge::Type::GAME,
			mainbar.resolve("gauge/hp/0"),
			mainbar.resolve("gauge/hp/1"),
			mainbar.resolve("gauge/hp/2"),
			137, 0.0f
		);
		mpbar = Gauge(
			Gauge::Type::GAME,
			mainbar.resolve("gauge/mp/0"),
			mainbar.resolve("gauge/mp/1"),
			mainbar.resolve("gauge/mp/2"),
			137, 0.0f
		);

		// === Extra gauges ===
		nl::node gauge_node = mainbar["gauge"];

		barrier_bar = Gauge(
			Gauge::Type::GAME,
			gauge_node.resolve("barrier/0"),
			gauge_node.resolve("barrier/1"),
			gauge_node.resolve("barrier/2"),
			137, 0.0f
		);
		df_bar = Gauge(
			Gauge::Type::GAME,
			gauge_node.resolve("df/0"),
			gauge_node.resolve("df/1"),
			gauge_node.resolve("df/2"),
			137, 0.0f
		);
		relax_exp_bar = Gauge(
			Gauge::Type::GAME,
			gauge_node.resolve("relaxExp/0"),
			gauge_node.resolve("relaxExp/1"),
			gauge_node.resolve("relaxExp/2"),
			308, 0.0f
		);
		tf_bar = Gauge(
			Gauge::Type::GAME,
			gauge_node.resolve("tf/0"),
			gauge_node.resolve("tf/1"),
			gauge_node.resolve("tf/2"),
			137, 0.0f
		);

		// === Charsets ===
		statset = Charset(gauge_node["number"], Charset::Alignment::RIGHT);
		levelset = Charset(mainbar["lvNumber"], Charset::Alignment::LEFT);

		// === Labels ===
		joblabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW);
		namelabel = Text(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE);

		// === Gauge animations ===
		ani_hp_gauge = Animation(mainbar["aniHPGauge"]);
		ani_hp_gauge_ab = Animation(mainbar["aniHPGaugeAB"]);
		ani_mp_gauge = Animation(mainbar["aniMPGauge"]);

		// === Notification animations ===
		ap_notify = Animation(mainbar["ApNotify"]);
		sp_notify = Animation(mainbar["SpNotify"]);
		noncombat_notify = Animation(mainbar["noncombatNotify"]);
		cooltime_return = Texture(mainbar["coolTimeReturn"]["0"]);

		// === Main bar buttons ===
		buttons[BT_WHISPER]     = std::make_unique<MapleButton>(mainbar["BtChat"]);
		buttons[BT_CALLGM]     = std::make_unique<MapleButton>(mainbar["BtClaim"]);
		buttons[BT_CASHSHOP]   = std::make_unique<MapleButton>(mainbar["BtCashShop"]);
		buttons[BT_TRADE]      = std::make_unique<MapleButton>(mainbar["BtMTS"], Point<int16_t>(17, 0));
		buttons[BT_MENU]       = std::make_unique<MapleButton>(mainbar["BtMenu"], Point<int16_t>(53, 0));
		buttons[BT_OPTIONS]    = std::make_unique<MapleButton>(mainbar["BtSystem"], Point<int16_t>(53, 0));
		buttons[BT_CHARACTER]  = std::make_unique<MapleButton>(mainbar["BtCharacter"]);
		buttons[BT_STATS]      = std::make_unique<MapleButton>(mainbar["BtStat"]);
		buttons[BT_QUEST]      = std::make_unique<MapleButton>(mainbar["BtQuest"]);
		buttons[BT_INVENTORY]  = std::make_unique<MapleButton>(mainbar["BtInven"]);
		buttons[BT_EQUIPS]     = std::make_unique<MapleButton>(mainbar["BtEquip"]);
		buttons[BT_SKILL]      = std::make_unique<MapleButton>(mainbar["BtSkill"]);

		// === Additional main bar buttons ===
		buttons[BT_CHANNEL]    = std::make_unique<MapleButton>(mainbar["BtChannel"], Point<int16_t>(53, 0));
		buttons[BT_KEYSETTING] = std::make_unique<MapleButton>(mainbar["BtKeysetting"], Point<int16_t>(53, 0));
		buttons[BT_NOTICE]     = std::make_unique<MapleButton>(mainbar["BtNotice"]);
		buttons[BT_FARM]       = std::make_unique<MapleButton>(mainbar["BtFarm"]);
		buttons[BT_EXITDUNGEON] = std::make_unique<MapleButton>(mainbar["BtExitDungeon"]);
		buttons[BF_BT_CASHSHOP] = std::make_unique<MapleButton>(mainbar["BfBtCashShop"]);

		// Farm, ExitDungeon not used on Cosmic server
		buttons[BT_FARM]->set_active(false);
		buttons[BT_EXITDUNGEON]->set_active(false);
		buttons[BF_BT_CASHSHOP]->set_active(false);

		// BtCharacter duplicates BtStat (both open UIStatsInfo); the v83
		// toolbar only uses BtStat, so hide BtCharacter.
		buttons[BT_CHARACTER]->set_active(false);


		// === Chat buttons ===
		buttons[BT_CHATCLOSE]  = std::make_unique<MapleButton>(mainbar["chatClose"]);
		buttons[BT_CHATOPEN]   = std::make_unique<MapleButton>(mainbar["chatOpen"]);
		buttons[BT_SCROLLUP]   = std::make_unique<MapleButton>(mainbar["scrollUp"]);
		buttons[BT_SCROLLDOWN] = std::make_unique<MapleButton>(mainbar["scrollDown"]);

		// Chat buttons handled by UIChatBar, disable here
		buttons[BT_CHATOPEN]->set_active(false);
		buttons[BT_CHATCLOSE]->set_active(false);
		buttons[BT_SCROLLUP]->set_active(false);
		buttons[BT_SCROLLDOWN]->set_active(false);

		// === Chat area textures ===
		chat_cover = Texture(mainbar["chatCover"]);
		chat_enter = Texture(mainbar["chatEnter"]);
		chat_space = Texture(mainbar["chatSpace"]);
		chat_space2 = Texture(mainbar["chatSpace2"]);

		// === Chat target textures ===
		nl::node ct = mainbar["chatTarget"];
		chat_target_base = Texture(ct["base"]);
		chat_target_all = Texture(ct["all"]);
		chat_target_party = Texture(ct["party"]);
		chat_target_guild = Texture(ct["guild"]);
		chat_target_friend = Texture(ct["friend"]);
		chat_target_expedition = Texture(ct["expedition"]);
		chat_target_association = Texture(ct["association"]);
		chat_target_afctv = Texture(ct["afctv"]);

		// === Chat tab bar (from chat section) ===
		tap_bar = Texture(chat["tapBar"]);
		tap_bar_over = Texture(chat["tapBarOver"]);

		nl::node chat_scroll = chat["scroll"];
		chat_scroll_normal = Texture(chat_scroll["normal"]);
		chat_scroll_over = Texture(chat_scroll["over"]);

		// === Chat tab buttons ===
		nl::node tap = chat["Tap"];
		buttons[BT_TAP_ALL]         = std::make_unique<MapleButton>(tap["all"]);
		buttons[BT_TAP_PARTY]       = std::make_unique<MapleButton>(tap["party"]);
		buttons[BT_TAP_GUILD]       = std::make_unique<MapleButton>(tap["guild"]);
		buttons[BT_TAP_FRIEND]      = std::make_unique<MapleButton>(tap["friend"]);
		buttons[BT_TAP_EXPEDITION]  = std::make_unique<MapleButton>(tap["expedition"]);
		buttons[BT_TAP_ASSOCIATION] = std::make_unique<MapleButton>(tap["association"]);
		buttons[BT_TAP_AFREECATV]   = std::make_unique<MapleButton>(tap["afreecaTV"]);

		// Chat tabs hidden until chat is open
		buttons[BT_TAP_ALL]->set_active(false);
		buttons[BT_TAP_PARTY]->set_active(false);
		buttons[BT_TAP_GUILD]->set_active(false);
		buttons[BT_TAP_FRIEND]->set_active(false);
		buttons[BT_TAP_EXPEDITION]->set_active(false);
		buttons[BT_TAP_ASSOCIATION]->set_active(false);
		buttons[BT_TAP_AFREECATV]->set_active(false);

		// === Quick slot ===
		nl::node qs = mainbar["quickSlot"];
		quickslot_bg = Texture(qs["quickSlot"]);

		// v83 StatusBar2 bitmap is a blank 145x93 frame — render key
		// names programmatically on top of each cell.
		static const char* QS_LABEL_NAMES[8] = {
			"Shft", "Ins", "Hme", "PgU",
			"Ctl",  "Del", "End", "PgD"
		};
		for (int i = 0; i < 8; i++)
			qs_key_labels[i] = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NONE, QS_LABEL_NAMES[i]);

		buttons[BT_QS_OPEN]  = std::make_unique<MapleButton>(qs["BtOpen"]);
		buttons[BT_QS_OPEN]->set_active(true);
		buttons[BT_QS_CLOSE] = std::make_unique<MapleButton>(qs["BtClose"]);
		buttons[BT_QS_CLOSE]->set_active(false);

		// === Menu sub-panel ===
		nl::node menu_node = mainbar["Menu"];

		// Menu buttons stack top-to-bottom above the bar.
		// Bar top is at -84 relative to position. Panel sits just above that.
		// v83-visible: Stat, Skill, Quest, Item, Equip, Community, Event, Rank, EpisodBook (9 buttons)
		constexpr int16_t MENU_STEP = 26;
		constexpr int16_t MENU_VISIBLE = 9;
		constexpr int16_t MENU_PANEL_H = MENU_VISIBLE * MENU_STEP + 8;

		int16_t menu_x = 188;
		// Position the Menu panel using the same anchor style as the
		// System sub-panel so both popups line up vertically on screen.
		int16_t menu_panel_top = -(14 + MENU_PANEL_H);
		int16_t menu_y = menu_panel_top - 26;

		buttons[BT_MENU_STAT]          = std::make_unique<MapleButton>(menu_node["BtStat"],      Point<int16_t>(menu_x, menu_y));
		buttons[BT_MENU_SKILL]         = std::make_unique<MapleButton>(menu_node["BtSkill"],     Point<int16_t>(menu_x, menu_y + MENU_STEP));
		buttons[BT_MENU_QUEST]         = std::make_unique<MapleButton>(menu_node["BtQuest"],     Point<int16_t>(menu_x, menu_y + MENU_STEP * 2));
		buttons[BT_MENU_ITEM]          = std::make_unique<MapleButton>(menu_node["BtItem"],      Point<int16_t>(menu_x, menu_y + MENU_STEP * 3));
		buttons[BT_MENU_EQUIP]         = std::make_unique<MapleButton>(menu_node["BtEquip"],     Point<int16_t>(menu_x, menu_y + MENU_STEP * 4));
		buttons[BT_MENU_COMMUNITY]     = std::make_unique<MapleButton>(menu_node["BtCommunity"], Point<int16_t>(menu_x, menu_y + MENU_STEP * 5));
		buttons[BT_MENU_EVENT]         = std::make_unique<MapleButton>(menu_node["BtEvent"],     Point<int16_t>(menu_x, menu_y + MENU_STEP * 6));
		buttons[BT_MENU_RANK]          = std::make_unique<MapleButton>(menu_node["BtRank"],      Point<int16_t>(menu_x, menu_y + MENU_STEP * 7));
		buttons[BT_MENU_EPISODBOOK]    = std::make_unique<MapleButton>(menu_node["BtEpisodBook"],    Point<int16_t>(menu_x, menu_y + MENU_STEP * 8));

		// Post-BB buttons — created but disabled and never shown
		buttons[BT_MENU_MONSTERBATTLE] = std::make_unique<MapleButton>(menu_node["BtMonsterBattle"], Point<int16_t>(menu_x, menu_y + MENU_STEP * 9));
		buttons[BT_MENU_MONSTERLIFE]   = std::make_unique<MapleButton>(menu_node["BtMonsterLife"],   Point<int16_t>(menu_x, menu_y + MENU_STEP * 10));
		buttons[BT_MENU_MSN]           = std::make_unique<MapleButton>(menu_node["BtMSN"],           Point<int16_t>(menu_x, menu_y + MENU_STEP * 11));
		buttons[BT_MENU_AFREECATV]     = std::make_unique<MapleButton>(menu_node["BtAfreecaTV"],     Point<int16_t>(menu_x, menu_y + MENU_STEP * 12));

		// All menu buttons hidden until menu is toggled
		for (uint16_t i = BT_MENU_STAT; i <= BT_MENU_AFREECATV; i++)
			buttons[i]->set_active(false);

		// Disable post-BB buttons permanently
		buttons[BT_MENU_MONSTERBATTLE]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_MONSTERLIFE]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_MSN]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_AFREECATV]->set_state(Button::State::DISABLED);

		// Menu background sized to cover visible buttons

		// === System sub-panel ===
		nl::node sys_node = mainbar["System"];

		// 5 visible buttons (v83 layout):
		//   Channel → JoyPad → KeySetting → Option (System) → Quit
		// SYS_STEP = button height (25) + 0px gap between rows (tight).
		constexpr int16_t SYS_STEP = 25;
		constexpr int16_t SYS_VISIBLE = 5;
		constexpr int16_t SYS_PANEL_H = SYS_VISIBLE * SYS_STEP + 8;

		int16_t sys_x = 264;
		int16_t sys_panel_top = -(14 + SYS_PANEL_H);
		int16_t sys_y = sys_panel_top - 26;

		int16_t si = 0;
		buttons[BT_SYS_CHANNEL]    = std::make_unique<MapleButton>(sys_node["BtChannel"],    Point<int16_t>(sys_x, sys_y + SYS_STEP * si++));
		buttons[BT_SYS_JOYPAD]     = std::make_unique<MapleButton>(sys_node["BtJoyPad"],     Point<int16_t>(sys_x, sys_y + SYS_STEP * si++));
		buttons[BT_SYS_KEYSETTING] = std::make_unique<MapleButton>(sys_node["BtKeySetting"], Point<int16_t>(sys_x, sys_y + SYS_STEP * si++));
		buttons[BT_SYS_OPTION]     = std::make_unique<MapleButton>(sys_node["BtOption"],     Point<int16_t>(sys_x, sys_y + SYS_STEP * si++));
		buttons[BT_SYS_GAMEQUIT]   = std::make_unique<MapleButton>(sys_node["BtGameQuit"],   Point<int16_t>(sys_x, sys_y + SYS_STEP * si++));

		// All system buttons hidden until toggled
		buttons[BT_SYS_CHANNEL]->set_active(false);
		buttons[BT_SYS_JOYPAD]->set_active(false);
		buttons[BT_SYS_KEYSETTING]->set_active(false);
		buttons[BT_SYS_OPTION]->set_active(false);
		buttons[BT_SYS_GAMEQUIT]->set_active(false);

		// 3-piece tiled backdrop (StatusBar2.img/mainBar/System/backgrnd/0..2).
		{
			nl::node sys_bg_node = sys_node["backgrnd"];
			sys_bg_top = Texture(sys_bg_node["0"]);
			sys_bg_mid = Texture(sys_bg_node["1"]);
			sys_bg_bot = Texture(sys_bg_node["2"]);
		}

		{
			nl::node menu_bg_node = mainbar["Menu"]["backgrnd"];
			menu_bg_top = menu_bg_node["0"];
			menu_bg_mid = menu_bg_node["1"];
			menu_bg_bot = menu_bg_node["2"];
		}

		// === readyZero ===
		nl::node rz = mainbar["readyZero"];
		ready_zero_backgrnd = Texture(rz["gaugeBackgrnd"]);

		// === Buff tray background (StatusBar3.img) ===
		nl::node statusbar3 = nl::nx::ui["StatusBar3.img"];
		if (statusbar3.size() > 0)
		{
			nl::node buff = statusbar3["buff"];
			buff_backgrnd = Texture(buff["backgrnd"]);

			nl::node alarm = statusbar3["alarm"];
			alarm_backgrnd = Texture(alarm["backgrnd"]);
			alarm_anim = Animation(alarm["ani"]);

			nl::node event = statusbar3["event"];
			event_backgrnd = Texture(event["backgrnd"]);
		}
	}

	void UIStatusBar::draw(float alpha) const
	{
		int16_t vwidth = Constants::Constants::get().get_viewwidth();

		// Quickslot panel — drawn FIRST so the status bar renders on
		// top of it. The panel sits above the main bar on screen, but
		// any overlap with other status bar sprites goes to the bar.
		if (show_quickslot)
		{
			if (quickslot_bg.is_valid())
				quickslot_bg.draw(DrawArgument(position));

			int16_t cell_label_w = 30;
			for (int16_t i = 0; i < 8; ++i)
			{
				Point<int16_t> tl = quickslot_slot_pos(i);
				qs_key_labels[i].draw(DrawArgument(tl + Point<int16_t>(cell_label_w / 2, -1)));
			}

			const auto& maplekeys = UI::get().get_keyboard().get_maplekeys();
			int16_t cell_w = 30;
			int16_t cell_h = 30;
			for (int16_t i = 0; i < static_cast<int16_t>(QUICKSLOT_KEYS.size()); ++i)
			{
				int32_t keycode = QUICKSLOT_KEYS[i];
				auto it = maplekeys.find(keycode);
				if (it == maplekeys.end())
					continue;

				const Keyboard::Mapping& m = it->second;
				if (m.type == KeyType::Id::NONE || m.action == 0)
					continue;

				Texture icon = get_quickslot_icon(m.type, m.action);
				if (icon.is_valid())
				{
					Point<int16_t> tl = quickslot_slot_pos(i);
					Point<int16_t> center = tl + Point<int16_t>((cell_w - 32) / 2,
					                                             (cell_h - 32) / 2);
					icon.draw(DrawArgument(center));
				}
			}
		}

		// Draw bar background at its natural anchor (position), extending
		// further right so the sprite reaches the right edge of wider
		// viewports. `vwidth * 2` guarantees the stretch covers beyond
		// the viewport at any resolution; extra pixels past the screen
		// are clipped harmlessly.
		if (bar_backgrnd.is_valid())
		{
			int16_t bg_h = bar_backgrnd.height();
			bar_backgrnd.draw(DrawArgument(position, Point<int16_t>(vwidth * 2, bg_h)));
		}

		UIElement::draw_sprites(alpha);

		// Draw class-variant gauge backgrounds based on job
		uint16_t jobid = stats.get_job().get_id();

		// Aran/Blaster (AB) job range: 2000+
		if (jobid >= 2000 && jobid < 3000)
		{
			if (gauge_backgrd_ab.is_valid())
				gauge_backgrd_ab.draw(DrawArgument(position));
		}
		// Demon job range: 3001-3112
		else if (jobid >= 3001 && jobid <= 3112)
		{
			if (gauge_backgrd_demon.is_valid())
				gauge_backgrd_demon.draw(DrawArgument(position));
		}
		// Kanna job range: 4200+
		else if (jobid >= 4200 && jobid < 4300)
		{
			if (gauge_backgrd_kanna.is_valid())
				gauge_backgrd_kanna.draw(DrawArgument(position));
		}
		// Zero job range: 10000+
		else if (jobid >= 10000)
		{
			if (gauge_backgrd_zero.is_valid())
				gauge_backgrd_zero.draw(DrawArgument(position));

			if (ready_zero_backgrnd.is_valid())
				ready_zero_backgrnd.draw(DrawArgument(position));

			if (lv_backtrnd_sao.is_valid())
				lv_backtrnd_sao.draw(DrawArgument(position));
		}

		// Draw gauges (fills stretch inside the gauge slots).
		expbar.draw(position + Point<int16_t>(-261, -15));
		hpbar.draw(position + Point<int16_t>(-261, -31));
		mpbar.draw(position + Point<int16_t>(-90, -31));

		// Draw gauge cover ON TOP of the fills — this is the glossy
		// sheen sprite (StatusBar2/mainBar/gaugeCover, 308x28). The v83
		// "shiny bar" look comes from this overlay; drawing it before
		// the fills hides it completely.
		if (gauge_cover.is_valid())
			gauge_cover.draw(DrawArgument(position));

		// AB gauge cover variant
		if (jobid >= 2000 && jobid < 3000 && gauge_cover_ab.is_valid())
			gauge_cover_ab.draw(DrawArgument(position));

		// EXP flash-on-drop: no dedicated animation sprite, so re-draw the
		// expbar to pulse its brightness for the duration of the flash.
		if (exp_flash_ticks > 0)
		{
			float pulse = static_cast<float>(exp_flash_ticks) / FLASH_DURATION_TICKS;
			expbar.draw(DrawArgument(position + Point<int16_t>(-261, -15), pulse));
		}

		// Draw extra gauges for special classes
		// Demon Force gauge (replaces MP for Demon classes)
		if (jobid >= 3001 && jobid <= 3112)
			df_bar.draw(position + Point<int16_t>(-90, -31));

		// Time Force gauge (Zero class)
		if (jobid >= 10000)
			tf_bar.draw(position + Point<int16_t>(-90, -31));

		// Barrier gauge (Kanna)
		if (jobid >= 4200 && jobid < 4300)
			barrier_bar.draw(position + Point<int16_t>(-90, -31));

		// Relax EXP gauge (shows bonus EXP from resting)
		relax_exp_bar.draw(position + Point<int16_t>(-261, -15));

		// Gauge blink animations (StatusBar2.img/mainBar/aniHPGauge,
		// aniMPGauge, aniHPGaugeAB). The trigger threshold comes from
		// the HP/MP Warning sliders in System → Options (0..100 %). The
		// flash_ticks branch also fires the pulse briefly when the
		// gauge just dropped, regardless of the threshold.
		float hp_pct = gethppercent();
		float mp_pct = getmppercent();

		float hp_warn = Setting<HPWarning>::get().load() / 100.0f;
		float mp_warn = Setting<MPWarning>::get().load() / 100.0f;

		if (hp_pct < hp_warn || hp_flash_ticks > 0)
		{
			if (jobid >= 2000 && jobid < 3000)
				ani_hp_gauge_ab.draw(DrawArgument(position + Point<int16_t>(-261, -31)), alpha);
			else
				ani_hp_gauge.draw(DrawArgument(position + Point<int16_t>(-261, -31)), alpha);
		}

		if (mp_pct < mp_warn || mp_flash_ticks > 0)
			ani_mp_gauge.draw(DrawArgument(position + Point<int16_t>(-90, -31)), alpha);

		// Draw stat numbers
		int16_t level = stats.get_stat(MapleStat::Id::LEVEL);
		int16_t hp = stats.get_stat(MapleStat::Id::HP);
		int16_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);
		int64_t exp = stats.get_exp();

		std::string expstring = std::to_string(100 * getexppercent());
		statset.draw(
			std::to_string(exp) + "[" + expstring.substr(0, expstring.find('.') + 3) + "%]",
			position + Point<int16_t>(47, -13)
		);
		statset.draw(
			"[" + std::to_string(hp) + "/" + std::to_string(maxhp) + "]",
			position + Point<int16_t>(-124, -29)
		);
		statset.draw(
			"[" + std::to_string(mp) + "/" + std::to_string(maxmp) + "]",
			position + Point<int16_t>(47, -29)
		);
		levelset.draw(
			std::to_string(level),
			position + Point<int16_t>(-480, -24)
		);

		joblabel.draw(position + Point<int16_t>(-435, -21));
		namelabel.draw(position + Point<int16_t>(-435, -36));

		// Draw notice sprite when there are pending notifications.
		// Notice overlay moved to AFTER draw_buttons — see below.

		// Draw cooltime return indicator (NX origin positions it)
		if (cooltime_return.is_valid())
			cooltime_return.draw(DrawArgument(position));

		// Chat area rendering handled by UIChatBar

		// Draw AP notification only when player has unspent AP
		uint16_t ap = stats.get_stat(MapleStat::Id::AP);
		if (ap > 0)
			ap_notify.draw(DrawArgument(position), alpha);

		// Draw SP notification only when player has unspent SP
		uint16_t sp = stats.get_stat(MapleStat::Id::SP);
		if (sp > 0)
			sp_notify.draw(DrawArgument(position), alpha);

		// noncombat_notify blinking animation removed — the
		// UIAlarmInvite banner is the sole notification visual.

		// Quickslot panel is now drawn at the very top of draw() so it
		// sits BEHIND the rest of the status bar UI.

		// Buff tray (StatusBar3.img/buff/backgrnd) and alarm tray
		// (StatusBar3.img/alarm/backgrnd) are post-Big-Bang UI elements that
		// appear as large grid-like panels ("numpad" shaped) on screen.
		// v83 does not use these — keep the textures loaded but suppress
		// drawing them.
		// if (buff_backgrnd.is_valid())
		//     buff_backgrnd.draw(DrawArgument(position + Point<int16_t>(184, -70)));
		// if (alarm_backgrnd.is_valid())
		//     alarm_backgrnd.draw(DrawArgument(position + Point<int16_t>(-512, -70)));

		// Draw main-bar buttons first
		UIElement::draw_buttons(alpha);

		// Notice-button pulse/blink overlays removed — the UIAlarmInvite
		// banner (UIWindow.img/FadeYesNo) is the only notification now.

		// DEBUG sprite preview overlays — driven by /float <N> and
		// /notice# <N> chat commands. Renders the chosen sprite near
		// the center of the screen so it's easy to browse all entries.
		if (preview_mode != 0)
		{
			int16_t vh = Constants::Constants::get().get_viewheight();
			Point<int16_t> center(vwidth / 2, vh / 2);

			if (preview_mode == 1)
			{
				// FloatNotice/<idx> — 3-piece tiled balloon: 0 left cap,
				// 1 middle stretch, 2 right cap. Stretch middle to 160px.
				nl::node fn = nl::nx::ui["UIWindow.img"]["FloatNotice"][std::to_string(preview_idx)];
				Texture l(fn["0"]);
				Texture m(fn["1"]);
				Texture r(fn["2"]);
				if (l.is_valid())
				{
					constexpr int16_t MID_W = 160;
					int16_t lw = l.width();
					int16_t mh = m.is_valid() ? m.height() : l.height();
					Point<int16_t> tl(center.x() - (lw + MID_W + r.width()) / 2, center.y() - mh / 2);
					l.draw(DrawArgument(tl));
					if (m.is_valid())
						m.draw(DrawArgument(tl + Point<int16_t>(lw, 0), Point<int16_t>(MID_W, mh)));
					if (r.is_valid())
						r.draw(DrawArgument(tl + Point<int16_t>(lw + MID_W, 0)));
				}
			}
			else if (preview_mode == 2)
			{
				// UIWindow.img/Notice/<idx> — single-frame modal bmp.
				nl::node nt = nl::nx::ui["UIWindow.img"]["Notice"][std::to_string(preview_idx)];
				Texture t(nt);
				if (t.is_valid())
				{
					Point<int16_t> tl(center.x() - t.width() / 2, center.y() - t.height() / 2);
					t.draw(DrawArgument(tl));
				}
			}
		}

		// Sub-panel overlays — drawn LAST so they sit above every other
		// sprite/button in the status bar. Sub-panel buttons have already
		// drawn via draw_buttons() above; we re-draw them here on top of
		// the backdrop so they remain visible.
		// Padding: tight on the sides (4px), extra on top (16px) and
		// bottom (8px) so the frame extends noticeably upward.
		constexpr int16_t SUBPANEL_PAD_X    = 8;
		constexpr int16_t SUBPANEL_PAD_TOP  = 15;
		constexpr int16_t SUBPANEL_PAD_BOT  = 13;

		auto draw_subpanel = [&](const Button& top_b, const Button& bot_b,
			const Texture& t, const Texture& m, const Texture& b, float fade)
		{
			auto top_btn = top_b.bounds(position);
			auto bot_btn = bot_b.bounds(position);
			int16_t btn_w = top_btn.get_right_bottom().x() - top_btn.get_left_top().x();
			int16_t span  = bot_btn.get_right_bottom().y() - top_btn.get_left_top().y();
			int16_t panel_w = btn_w + SUBPANEL_PAD_X * 2;
			int16_t panel_h = span + SUBPANEL_PAD_TOP + SUBPANEL_PAD_BOT;
			Point<int16_t> tl(top_btn.get_left_top().x() - SUBPANEL_PAD_X,
			                  top_btn.get_left_top().y() - SUBPANEL_PAD_TOP);
			draw_tiled_panel(tl, panel_w, panel_h, t, m, b, fade);
		};

		if (menu_fade > 0.0f)
		{
			// Bottom reference is the last v83-visible entry (EpisodBook).
			// BT_MENU_AFREECATV and other post-BB rows are permanently
			// disabled — anchoring to them over-sized the backdrop.
			draw_subpanel(*buttons.at(BT_MENU_STAT),
			              *buttons.at(BT_MENU_EPISODBOOK),
			              menu_bg_top, menu_bg_mid, menu_bg_bot, menu_fade);
			// Fade the buttons alongside the backdrop (alpha overload
			// ignores the `active` flag so the visual fade is smooth).
			for (uint16_t i = BT_MENU_STAT; i <= BT_MENU_EPISODBOOK; i++)
				static_cast<MapleButton*>(buttons.at(i).get())->draw(position, menu_fade);
		}

		if (sys_fade > 0.0f)
		{
			draw_subpanel(*buttons.at(BT_SYS_CHANNEL),
			              *buttons.at(BT_SYS_GAMEQUIT),
			              sys_bg_top, sys_bg_mid, sys_bg_bot, sys_fade);
			auto draw_sys = [&](uint16_t id) {
				static_cast<MapleButton*>(buttons.at(id).get())->draw(position, sys_fade);
			};
			draw_sys(BT_SYS_CHANNEL);
			draw_sys(BT_SYS_JOYPAD);
			draw_sys(BT_SYS_KEYSETTING);
			draw_sys(BT_SYS_OPTION);
			draw_sys(BT_SYS_GAMEQUIT);
		}
	}

	void UIStatusBar::draw_tiled_panel(Point<int16_t> tl, int16_t panel_w, int16_t panel_h,
		const Texture& top, const Texture& mid, const Texture& bot,
		float fade_alpha) const
	{
		if (fade_alpha <= 0.0f) return;

		int16_t top_h = top.get_dimensions().y();
		int16_t bot_h = bot.get_dimensions().y();

		// Clamp middle stretch so top + middle + bottom >= panel_h.
		int16_t middle_h = panel_h - top_h - bot_h;
		if (middle_h < 0) middle_h = 0;

		// Stretch all three pieces to panel_w so the panel matches the
		// button row width (sprite is natively 79px).
		top.draw(DrawArgument(tl, Point<int16_t>(panel_w, top_h)) + fade_alpha);
		if (middle_h > 0)
			mid.draw(DrawArgument(tl + Point<int16_t>(0, top_h),
				Point<int16_t>(panel_w, middle_h)) + fade_alpha);
		bot.draw(DrawArgument(tl + Point<int16_t>(0, top_h + middle_h),
			Point<int16_t>(panel_w, bot_h)) + fade_alpha);
	}

	void UIStatusBar::update()
	{
		int16_t VWIDTH = Constants::Constants::get().get_viewwidth();
		int16_t VHEIGHT = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>(512, VHEIGHT);
		dimension = Point<int16_t>(std::max<int16_t>(1366, VWIDTH), 84);

		UIElement::update();

		// Fade sub-panels in/out. Target is 1.0 while shown, 0.0 while
		// hidden — a ~8-tick ramp gives a smooth fade instead of a pop.
		constexpr float FADE_STEP = 1.0f / 8.0f;
		float menu_target = show_menu   ? 1.0f : 0.0f;
		float sys_target  = show_system ? 1.0f : 0.0f;
		if (menu_fade < menu_target)      menu_fade = std::min(menu_target, menu_fade + FADE_STEP);
		else if (menu_fade > menu_target) menu_fade = std::max(menu_target, menu_fade - FADE_STEP);
		if (sys_fade < sys_target)        sys_fade  = std::min(sys_target,  sys_fade  + FADE_STEP);
		else if (sys_fade > sys_target)   sys_fade  = std::max(sys_target,  sys_fade  - FADE_STEP);

		// Sub-panel buttons are clickable only at full opacity — this
		// keeps visual fade in sync with input routing and prevents the
		// old "buttons pop in before backdrop" flash.
		bool menu_live = (menu_fade >= 1.0f);
		buttons[BT_MENU_STAT]      ->set_active(menu_live);
		buttons[BT_MENU_SKILL]     ->set_active(menu_live);
		buttons[BT_MENU_QUEST]     ->set_active(menu_live);
		buttons[BT_MENU_ITEM]      ->set_active(menu_live);
		buttons[BT_MENU_EQUIP]     ->set_active(menu_live);
		buttons[BT_MENU_COMMUNITY] ->set_active(menu_live);
		buttons[BT_MENU_EVENT]     ->set_active(menu_live);
		buttons[BT_MENU_RANK]      ->set_active(menu_live);
		buttons[BT_MENU_EPISODBOOK]->set_active(menu_live);

		bool sys_live = (sys_fade >= 1.0f);
		buttons[BT_SYS_CHANNEL]   ->set_active(sys_live);
		buttons[BT_SYS_JOYPAD]    ->set_active(sys_live);
		buttons[BT_SYS_KEYSETTING]->set_active(sys_live);
		buttons[BT_SYS_OPTION]    ->set_active(sys_live);
		buttons[BT_SYS_GAMEQUIT]  ->set_active(sys_live);

		float cur_hp = gethppercent();
		float cur_mp = getmppercent();
		float cur_exp = getexppercent();

		// Trigger flash whenever a gauge drops. Small epsilon avoids noise
		// from floating-point jitter in the smoothing inside Gauge::update.
		constexpr float eps = 0.0005f;
		if (cur_hp + eps < prev_hp_pct)
			hp_flash_ticks = FLASH_DURATION_TICKS;
		if (cur_mp + eps < prev_mp_pct)
			mp_flash_ticks = FLASH_DURATION_TICKS;
		if (cur_exp + eps < prev_exp_pct)
			exp_flash_ticks = FLASH_DURATION_TICKS;

		prev_hp_pct = cur_hp;
		prev_mp_pct = cur_mp;
		prev_exp_pct = cur_exp;

		if (hp_flash_ticks) hp_flash_ticks--;
		if (mp_flash_ticks) mp_flash_ticks--;
		if (exp_flash_ticks) exp_flash_ticks--;

		expbar.update(cur_exp);
		hpbar.update(cur_hp);
		mpbar.update(cur_mp);

		namelabel.change_text(stats.get_name());
		joblabel.change_text(stats.get_jobname());

		// Update animations
		ani_hp_gauge.update();
		ani_mp_gauge.update();
		ap_notify.update();
		sp_notify.update();
		noncombat_notify.update();
		alarm_anim.update();

		// Pulse counter for notice sprite (only advances when active)
		if (has_notification)
			notice_pulse_tick++;

		// Keep the notice button visually static — no hover swap. The
		// button is purely a notification indicator; the state transition
		// would only flash the same bitmap back and forth.
		if (buttons.count(BT_NOTICE))
			buttons[BT_NOTICE]->set_state(Button::State::NORMAL);
	}

	Button::State UIStatusBar::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case BT_WHISPER:
			UI::get().emplace<UIWhisper>();
			return Button::State::NORMAL;

		case BT_CALLGM:
			UI::get().emplace<UIReport>();
			return Button::State::NORMAL;

		case BT_FARM:
			UI::get().emplace<UIFarmChat>();
			return Button::State::NORMAL;

		case BT_STATS:
		case BT_MENU_STAT:
			UI::get().emplace<UIStatsInfo>(
				Stage::get().get_player().get_stats()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_INVENTORY:
		case BT_MENU_ITEM:
			UI::get().emplace<UIItemInventory>(
				Stage::get().get_player().get_inventory()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_EQUIPS:
		case BT_MENU_EQUIP:
			UI::get().emplace<UIEquipInventory>(
				Stage::get().get_player().get_inventory()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_SKILL:
		case BT_MENU_SKILL:
			UI::get().emplace<UISkillBook>(
				Stage::get().get_player().get_stats(),
				Stage::get().get_player().get_skills()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_QUEST:
		case BT_MENU_QUEST:
			UI::get().emplace<UIQuestLog>(
				Stage::get().get_player().get_quests()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_CASHSHOP:
		case BF_BT_CASHSHOP:
			OutPacket(OutPacket::Opcode::ENTER_CASHSHOP).dispatch();
			return Button::State::NORMAL;

		case BT_TRADE:
		{
			// Enter Maple Trading System
			OutPacket(OutPacket::Opcode::ENTER_MTS).dispatch();
			return Button::State::NORMAL;
		}

		case BT_MENU:
			toggle_menu();
			return Button::State::NORMAL;

		case BT_OPTIONS:
		{
			if (show_system)
			{
				show_system = false;
	

			}
			else
			{
				// Close menu if open
				if (show_menu)
					toggle_menu();

				show_system = true;
	
	
			}
			return Button::State::NORMAL;
		}

		case BT_CHARACTER:
			UI::get().emplace<UIStatsInfo>(
				Stage::get().get_player().get_stats()
			);
			return Button::State::NORMAL;

		case BT_KEYSETTING:
		case BT_SYS_KEYSETTING:
			UI::get().emplace<UIKeyConfig>(
				Stage::get().get_player().get_inventory(),
				Stage::get().get_player().get_skills()
			);
			remove_menus();
			return Button::State::NORMAL;

		case BT_NOTICE:
		{
			// Open the notification drawer anchored above this button.
			// The popup positions itself so its bottom-right corner is
			// the button's top-left.
			Point<int16_t> btn_pos = position + buttons[BT_NOTICE]->bounds(Point<int16_t>(0, 0)).get_left_top();
			UI::get().emplace<UINotificationList>(btn_pos);
			return Button::State::NORMAL;
		}

		case BT_CHANNEL:
		case BT_SYS_CHANNEL:
			UI::get().emplace<UIChannel>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_GAMEQUIT:
			UI::get().emplace<UIQuit>(stats);
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_OPTION:
			UI::get().emplace<UIOptionMenu>();
			remove_menus();
			return Button::State::NORMAL;


		case BT_SYS_JOYPAD:
			UI::get().emplace<UIJoypad>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_CHATOPEN:
			chat_open = true;
			buttons[BT_CHATOPEN]->set_active(false);
			buttons[BT_CHATCLOSE]->set_active(true);
			buttons[BT_SCROLLUP]->set_active(true);
			buttons[BT_SCROLLDOWN]->set_active(true);
			buttons[BT_TAP_ALL]->set_active(true);
			buttons[BT_TAP_PARTY]->set_active(true);
			buttons[BT_TAP_GUILD]->set_active(true);
			buttons[BT_TAP_FRIEND]->set_active(true);
			buttons[BT_TAP_EXPEDITION]->set_active(true);
			buttons[BT_TAP_ASSOCIATION]->set_active(true);
			buttons[BT_TAP_AFREECATV]->set_active(true);
			return Button::State::NORMAL;

		case BT_CHATCLOSE:
			chat_open = false;
			buttons[BT_CHATCLOSE]->set_active(false);
			buttons[BT_CHATOPEN]->set_active(true);
			buttons[BT_SCROLLUP]->set_active(false);
			buttons[BT_SCROLLDOWN]->set_active(false);
			buttons[BT_TAP_ALL]->set_active(false);
			buttons[BT_TAP_PARTY]->set_active(false);
			buttons[BT_TAP_GUILD]->set_active(false);
			buttons[BT_TAP_FRIEND]->set_active(false);
			buttons[BT_TAP_EXPEDITION]->set_active(false);
			buttons[BT_TAP_ASSOCIATION]->set_active(false);
			buttons[BT_TAP_AFREECATV]->set_active(false);
			return Button::State::NORMAL;

		case BT_TAP_ALL:
			chat_target_id = 0;
			return Button::State::PRESSED;
		case BT_TAP_PARTY:
			chat_target_id = 1;
			return Button::State::PRESSED;
		case BT_TAP_GUILD:
			chat_target_id = 2;
			return Button::State::PRESSED;
		case BT_TAP_FRIEND:
			chat_target_id = 3;
			return Button::State::PRESSED;
		case BT_TAP_EXPEDITION:
			chat_target_id = 4;
			return Button::State::PRESSED;
		case BT_TAP_ASSOCIATION:
			chat_target_id = 5;
			return Button::State::PRESSED;
		case BT_TAP_AFREECATV:
			chat_target_id = 6;
			return Button::State::PRESSED;

		case BT_QS_OPEN:
			toggle_qs();
			return Button::State::NORMAL;

		case BT_QS_CLOSE:
			toggle_qs();
			return Button::State::NORMAL;

		case BT_MENU_COMMUNITY:
			UI::get().emplace<UIBuddyList>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_EVENT:
			UI::get().emplace<UIEvent>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_RANK:
			UI::get().emplace<UIRanking>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_EPISODBOOK:
			UI::get().emplace<UIMonsterBook>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_MONSTERBATTLE:
		case BT_MENU_MONSTERLIFE:
		case BT_MENU_MSN:
		case BT_MENU_AFREECATV:
			remove_menus();
			return Button::State::NORMAL;

		default:
			return Button::State::NORMAL;
		}
	}

	bool UIStatusBar::is_in_range(Point<int16_t> cursorpos) const
	{
		int16_t vwidth = Constants::Constants::get().get_viewwidth();

		// Extend upward when menu, system sub-panel, or quick slot is open
		int16_t extra_height = (show_menu || show_system || show_quickslot) ? 300 : 0;

		Rectangle<int16_t> bounds(
			Point<int16_t>(0, position.y() - 84 - extra_height),
			Point<int16_t>(vwidth, position.y())
		);

		return bounds.contains(cursorpos);
	}

	UIElement::Type UIStatusBar::get_type() const
	{
		return TYPE;
	}

	void UIStatusBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			remove_menus();
	}

	void UIStatusBar::toggle_qs()
	{
		show_quickslot = !show_quickslot;

		if (show_quickslot)
		{
			buttons[BT_QS_OPEN]->set_active(false);
			buttons[BT_QS_CLOSE]->set_active(true);
		}
		else
		{
			buttons[BT_QS_OPEN]->set_active(true);
			buttons[BT_QS_CLOSE]->set_active(false);
		}
	}

	void UIStatusBar::toggle_menu()
	{
		if (show_menu)
		{
			show_menu = false;
		}
		else
		{
			// Close system panel if open
			if (show_system)
				show_system = false;

			show_menu = true;
		}
	}

	void UIStatusBar::remove_menus()
	{
		if (show_menu)
			toggle_menu();

		if (show_system)
			show_system = false;
	}

	bool UIStatusBar::is_menu_active()
	{
		return show_menu || show_system;
	}

	float UIStatusBar::getexppercent() const
	{
		int16_t level = stats.get_stat(MapleStat::Id::LEVEL);

		if (level >= ExpTable::LEVELCAP)
			return 0.0f;

		int64_t exp = stats.get_exp();

		return static_cast<float>(
			static_cast<double>(exp) / ExpTable::values[level]
		);
	}

	float UIStatusBar::gethppercent() const
	{
		int16_t hp = stats.get_stat(MapleStat::Id::HP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);

		return static_cast<float>(hp) / maxhp;
	}

	float UIStatusBar::getmppercent() const
	{
		int16_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);

		return static_cast<float>(mp) / maxmp;
	}

	void UIStatusBar::notify()
	{
		has_notification = true;

		if (buttons.count(BT_NOTICE))
			buttons[BT_NOTICE]->set_active(true);
	}

	void UIStatusBar::clear_notification()
	{
		has_notification = false;

		if (buttons.count(BT_NOTICE))
			buttons[BT_NOTICE]->set_active(false);
	}

	void UIStatusBar::set_preview_float(int idx)  { preview_mode = 1; preview_idx = idx; }
	void UIStatusBar::set_preview_notice(int idx) { preview_mode = 2; preview_idx = idx; }
	void UIStatusBar::clear_preview()             { preview_mode = 0; preview_idx = 0; }

	bool UIStatusBar::preview_step(int delta)
	{
		if (preview_mode == 0) return false;

		// Collect valid indexes (child names under the active root) and
		// advance by delta, wrapping around.
		nl::node root;
		if (preview_mode == 1)
			root = nl::nx::ui["UIWindow.img"]["FloatNotice"];
		else
			root = nl::nx::ui["UIWindow.img"]["Notice"];

		std::vector<int> indexes;
		for (auto c : root)
		{
			std::string n = c.name();
			if (!n.empty() && std::all_of(n.begin(), n.end(), ::isdigit))
				indexes.push_back(std::atoi(n.c_str()));
		}
		if (indexes.empty()) return true;
		std::sort(indexes.begin(), indexes.end());

		auto it = std::find(indexes.begin(), indexes.end(), preview_idx);
		int pos;
		if (it == indexes.end())
			pos = 0;
		else
			pos = static_cast<int>(it - indexes.begin());

		int n = static_cast<int>(indexes.size());
		pos = ((pos + delta) % n + n) % n;
		preview_idx = indexes[pos];
		return true;
	}

	// === Quickslot drop / render ===

	// Layout constants for the v83 quickslot panel.
	// Per NX dump: quickSlot bitmap is 145x93 with origin (-143, 143),
	// so drawing at `position=(512, VHEIGHT)` places the panel top-left at
	// `(position.x + 143, position.y - 143)`.
	static constexpr int16_t QS_PANEL_OFFSET_X = 143;
	static constexpr int16_t QS_PANEL_OFFSET_Y = -143;
	// First slot top-left offset inside the panel, step between slots.
	// Tweak these to align with the visible cube squares.
	static constexpr int16_t QS_CELL_OFFSET_X = 8;
	static constexpr int16_t QS_CELL_OFFSET_Y = 15;
	static constexpr int16_t QS_CELL_W   = 30;
	static constexpr int16_t QS_CELL_H   = 30;
	static constexpr int16_t QS_COL_STEP = 34;
	static constexpr int16_t QS_ROW_STEP = 34;

	Point<int16_t> UIStatusBar::quickslot_panel_pos() const
	{
		return position + Point<int16_t>(QS_PANEL_OFFSET_X, QS_PANEL_OFFSET_Y);
	}

	Point<int16_t> UIStatusBar::quickslot_slot_pos(int16_t slot) const
	{
		int16_t col = slot % 4;
		int16_t row = slot / 4;
		return quickslot_panel_pos() + Point<int16_t>(QS_CELL_OFFSET_X + col * QS_COL_STEP,
		                                               QS_CELL_OFFSET_Y + row * QS_ROW_STEP);
	}

	int16_t UIStatusBar::quickslot_slot_at(Point<int16_t> cursorpos) const
	{
		if (!show_quickslot)
			return -1;

		for (int16_t i = 0; i < 8; i++)
		{
			Point<int16_t> tl = quickslot_slot_pos(i);
			Rectangle<int16_t> rect(tl, tl + Point<int16_t>(QS_CELL_W, QS_CELL_H));
			if (rect.contains(cursorpos))
				return i;
		}
		return -1;
	}

	Texture UIStatusBar::get_quickslot_icon(KeyType::Id type, int32_t action) const
	{
		if (action == 0)
			return Texture();

		auto iter = qs_icon_cache.find(action * 8 + type);
		if (iter != qs_icon_cache.end())
			return iter->second;

		// Match the keyboard UI (UIKeyConfig::get_skill_texture / get_item_texture):
		// skills use NORMAL icon, items use the non-raw icon variant.
		Texture tx;
		if (type == KeyType::Id::SKILL)
			tx = SkillData::get(action).get_icon(SkillData::Icon::NORMAL);
		else if (type == KeyType::Id::ITEM)
			tx = ItemData::get(action).get_icon(false);
		else if (type == KeyType::Id::MACRO)
			tx = nl::nx::ui["UIWindow.img"]["SkillMacro"]["Macroicon"][std::to_string(action)]["icon"];

		// v83 skill/item icons have origin (0, 32) so a raw draw at `pos`
		// renders 32 px ABOVE `pos` (matching StatefulIcon's compensation).
		// Shift so the texture's origin becomes (0, 0) — top-left at `pos`.
		tx.shift(Point<int16_t>(0, 32));

		qs_icon_cache[action * 8 + type] = tx;
		return tx;
	}

	void UIStatusBar::assign_quickslot(int16_t slot, KeyType::Id type, int32_t action)
	{
		if (slot < 0 || slot >= 8)
			return;

		uint8_t key = QUICKSLOT_KEYS[slot];

		// Persist locally so subsequent keypresses work immediately.
		UI::get().get_keyboard().assign(key, static_cast<uint8_t>(type), action);

		// Notify the server so the mapping persists across sessions.
		std::vector<std::tuple<KeyConfig::Key, KeyType::Id, int32_t>> updates;
		updates.emplace_back(static_cast<KeyConfig::Key>(key), type, action);
		ChangeKeyMapPacket(updates).dispatch();
	}

	bool UIStatusBar::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		int16_t slot = quickslot_slot_at(cursorpos);
		if (slot < 0)
			return false; // drop outside slots - let caller fall through

		Icon::IconType itype = const_cast<Icon&>(icon).get_type();
		int32_t action = icon.get_action_id();

		if (itype == Icon::IconType::SKILL)
			assign_quickslot(slot, KeyType::Id::SKILL, action);
		else if (itype == Icon::IconType::ITEM)
			assign_quickslot(slot, KeyType::Id::ITEM, action);
		else if (itype == Icon::IconType::MACRO)
			assign_quickslot(slot, KeyType::Id::MACRO, action);

		return true;
	}

	Cursor::State UIStatusBar::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// If cursor is over a quickslot cube that has a binding, allow the
		// user to pick it up and drag it (either to another cube, or off the
		// quickslot to clear the binding).
		if (show_quickslot)
		{
			int16_t slot = quickslot_slot_at(cursorpos);
			if (slot >= 0)
			{
				uint8_t key = QUICKSLOT_KEYS[slot];
				const auto& maplekeys = UI::get().get_keyboard().get_maplekeys();
				auto it = maplekeys.find(static_cast<int32_t>(key));
				if (it != maplekeys.end())
				{
					const Keyboard::Mapping& m = it->second;
					if ((m.type == KeyType::Id::SKILL || m.type == KeyType::Id::ITEM)
						&& m.action != 0)
					{
						if (clicked)
						{
							// Build a fresh Icon owning its own raw texture.
							// The Icon ctor shifts (0,32), so pass the raw
							// (origin 0,32) texture — not the cache's already-
							// shifted one.
							Texture raw;
							if (m.type == KeyType::Id::SKILL)
								raw = SkillData::get(m.action).get_icon(SkillData::Icon::NORMAL);
							else
								raw = ItemData::get(m.action).get_icon(false);

							auto icon = std::make_unique<Icon>(
								std::make_unique<QuickslotDragType>(key, m.type, m.action),
								raw,
								-1);

							// Clear the binding immediately; if the drop
							// lands on a cube, send_icon reassigns it.
							clear_quickslot(key);

							// Compute cursor offset relative to the icon's
							// top-left so dragdraw follows the grip point.
							Point<int16_t> tl = quickslot_slot_pos(slot);
							int16_t cell_w = 30, cell_h = 30;
							Point<int16_t> center = tl + Point<int16_t>(
								(cell_w - 32) / 2, (cell_h - 32) / 2);

							icon->start_drag(cursorpos - center);

							Icon* raw_ptr = icon.get();
							qs_drag_icons[key] = std::move(icon);
							UI::get().drag_icon(raw_ptr);

							return Cursor::State::GRABBING;
						}
						else
						{
							return Cursor::State::CANGRAB;
						}
					}
				}
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIStatusBar::clear_quickslot(uint8_t key)
	{
		UI::get().get_keyboard().remove(key);

		std::vector<std::tuple<KeyConfig::Key, KeyType::Id, int32_t>> updates;
		updates.emplace_back(static_cast<KeyConfig::Key>(key), KeyType::Id::NONE, 0);
		ChangeKeyMapPacket(updates).dispatch();
	}

	UIStatusBar::QuickslotDragType::QuickslotDragType(uint8_t k, KeyType::Id t, int32_t a)
		: key(k), type(t), action(a) {}

	void UIStatusBar::QuickslotDragType::drop_on_stage() const
	{
		// Binding was already cleared at drag-start. Nothing to do.
	}

	Icon::IconType UIStatusBar::QuickslotDragType::get_type()
	{
		if (type == KeyType::Id::SKILL)
			return Icon::IconType::SKILL;
		if (type == KeyType::Id::ITEM)
			return Icon::IconType::ITEM;
		return Icon::IconType::NONE;
	}

}
