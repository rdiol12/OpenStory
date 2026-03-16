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
#include "UIOptionMenu.h"
#include "UIQuit.h"
#include "UISkillBook.h"
#include "UIStatsInfo.h"
#include "UIWorldMap.h"

#include "../../Net/OutPacket.h"
#include "../../Net/Session.h"
#include "../../IO/Window.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"

#include <iostream>
#include <fstream>

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
		dimension = Point<int16_t>(1366, 80);

		show_menu = false;
		show_system = false;
		show_quickslot = false;
		chat_open = false;
		chat_target_id = 0;

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];
		nl::node chat = nl::nx::ui["StatusBar2.img"]["chat"];

		// === Background sprites ===
		sprites.emplace_back(mainbar["backgrnd"]);
		sprites.emplace_back(mainbar["gaugeBackgrd"]);
		sprites.emplace_back(mainbar["notice"]);
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
		buttons[BT_QS_OPEN]  = std::make_unique<MapleButton>(qs["BtOpen"]);
		buttons[BT_QS_CLOSE] = std::make_unique<MapleButton>(qs["BtClose"]);
		buttons[BT_QS_CLOSE]->set_active(false);

		// === Menu sub-panel ===
		// NX menu_backgrnd is invalid in this version, use ColorBox instead
		nl::node menu_node = mainbar["Menu"];

		// Position buttons above the bar in a vertical column
		// Menu button is around x=699 on screen, bar top is y=684
		// Each button ~63x25, stack upward with 2px gap
		int16_t menu_x = 188;  // 700 - 512 (parent x)
		int16_t menu_y_start = -111; // first button top: 768 - 111 = 657
		int16_t menu_step = -27;     // each button 27px apart going up

		buttons[BT_MENU_STAT]          = std::make_unique<MapleButton>(menu_node["BtStat"],      Point<int16_t>(menu_x, menu_y_start));
		buttons[BT_MENU_SKILL]         = std::make_unique<MapleButton>(menu_node["BtSkill"],     Point<int16_t>(menu_x, menu_y_start + menu_step));
		buttons[BT_MENU_QUEST]         = std::make_unique<MapleButton>(menu_node["BtQuest"],     Point<int16_t>(menu_x, menu_y_start + menu_step * 2));
		buttons[BT_MENU_ITEM]          = std::make_unique<MapleButton>(menu_node["BtItem"],      Point<int16_t>(menu_x, menu_y_start + menu_step * 3));
		buttons[BT_MENU_EQUIP]         = std::make_unique<MapleButton>(menu_node["BtEquip"],     Point<int16_t>(menu_x, menu_y_start + menu_step * 4));
		buttons[BT_MENU_COMMUNITY]     = std::make_unique<MapleButton>(menu_node["BtCommunity"], Point<int16_t>(menu_x, menu_y_start + menu_step * 5));
		buttons[BT_MENU_EVENT]         = std::make_unique<MapleButton>(menu_node["BtEvent"],     Point<int16_t>(menu_x, menu_y_start + menu_step * 6));
		buttons[BT_MENU_RANK]          = std::make_unique<MapleButton>(menu_node["BtRank"],      Point<int16_t>(menu_x, menu_y_start + menu_step * 7));
		buttons[BT_MENU_EPISODBOOK]    = std::make_unique<MapleButton>(menu_node["BtEpisodBook"],    Point<int16_t>(menu_x, menu_y_start + menu_step * 8));
		buttons[BT_MENU_MONSTERBATTLE] = std::make_unique<MapleButton>(menu_node["BtMonsterBattle"], Point<int16_t>(menu_x, menu_y_start + menu_step * 9));
		buttons[BT_MENU_MONSTERLIFE]   = std::make_unique<MapleButton>(menu_node["BtMonsterLife"],   Point<int16_t>(menu_x, menu_y_start + menu_step * 10));
		buttons[BT_MENU_MSN]           = std::make_unique<MapleButton>(menu_node["BtMSN"],           Point<int16_t>(menu_x, menu_y_start + menu_step * 11));
		buttons[BT_MENU_AFREECATV]     = std::make_unique<MapleButton>(menu_node["BtAfreecaTV"],     Point<int16_t>(menu_x, menu_y_start + menu_step * 12));

		// Menu buttons hidden until menu is toggled
		buttons[BT_MENU_STAT]->set_active(false);
		buttons[BT_MENU_SKILL]->set_active(false);
		buttons[BT_MENU_QUEST]->set_active(false);
		buttons[BT_MENU_ITEM]->set_active(false);
		buttons[BT_MENU_EQUIP]->set_active(false);
		buttons[BT_MENU_COMMUNITY]->set_active(false);
		buttons[BT_MENU_EVENT]->set_active(false);
		buttons[BT_MENU_RANK]->set_active(false);
		buttons[BT_MENU_EPISODBOOK]->set_active(false);
		buttons[BT_MENU_MONSTERBATTLE]->set_active(false);
		buttons[BT_MENU_MONSTERLIFE]->set_active(false);
		buttons[BT_MENU_MSN]->set_active(false);
		buttons[BT_MENU_AFREECATV]->set_active(false);

		// === System sub-panel ===
		nl::node sys_node = mainbar["System"];

		// Position buttons above the bar, right of Menu panel
		// System button is around x=776 on screen
		int16_t sys_x = 264;  // 776 - 512 (parent x)
		int16_t sys_y_start = -111;
		int16_t sys_step = -27;

		buttons[BT_SYS_CHANNEL]      = std::make_unique<MapleButton>(sys_node["BtChannel"],      Point<int16_t>(sys_x, sys_y_start));
		buttons[BT_SYS_GAMEOPTION]   = std::make_unique<MapleButton>(sys_node["BtGameOption"],   Point<int16_t>(sys_x, sys_y_start + sys_step));
		buttons[BT_SYS_GAMEQUIT]     = std::make_unique<MapleButton>(sys_node["BtGameQuit"],     Point<int16_t>(sys_x, sys_y_start + sys_step * 2));
		buttons[BT_SYS_JOYPAD]       = std::make_unique<MapleButton>(sys_node["BtJoyPad"],       Point<int16_t>(sys_x, sys_y_start + sys_step * 3));
		buttons[BT_SYS_KEYSETTING]   = std::make_unique<MapleButton>(sys_node["BtKeySetting"],   Point<int16_t>(sys_x, sys_y_start + sys_step * 4));
		buttons[BT_SYS_MONSTERLIFE]  = std::make_unique<MapleButton>(sys_node["BtMonsterLife"],  Point<int16_t>(sys_x, sys_y_start + sys_step * 5));
		buttons[BT_SYS_OPTION]       = std::make_unique<MapleButton>(sys_node["BtOption"],       Point<int16_t>(sys_x, sys_y_start + sys_step * 6));
		buttons[BT_SYS_ROOMCHANGE]   = std::make_unique<MapleButton>(sys_node["BtRoomChange"],   Point<int16_t>(sys_x, sys_y_start + sys_step * 7));
		buttons[BT_SYS_SYSTEMOPTION] = std::make_unique<MapleButton>(sys_node["BtSystemOption"], Point<int16_t>(sys_x, sys_y_start + sys_step * 8));

		// System buttons hidden until system menu is toggled
		buttons[BT_SYS_CHANNEL]->set_active(false);
		buttons[BT_SYS_GAMEOPTION]->set_active(false);
		buttons[BT_SYS_GAMEQUIT]->set_active(false);
		buttons[BT_SYS_JOYPAD]->set_active(false);
		buttons[BT_SYS_KEYSETTING]->set_active(false);
		buttons[BT_SYS_MONSTERLIFE]->set_active(false);
		buttons[BT_SYS_OPTION]->set_active(false);
		buttons[BT_SYS_ROOMCHANGE]->set_active(false);
		buttons[BT_SYS_SYSTEMOPTION]->set_active(false);

		// Disable v83-incompatible buttons (post-BB features)
		buttons[BT_MENU_MONSTERBATTLE]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_MONSTERLIFE]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_MSN]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_AFREECATV]->set_state(Button::State::DISABLED);
		buttons[BT_MENU_EPISODBOOK]->set_state(Button::State::DISABLED);
		buttons[BT_SYS_MONSTERLIFE]->set_state(Button::State::DISABLED);

		// === readyZero ===
		nl::node rz = mainbar["readyZero"];
		ready_zero_backgrnd = Texture(rz["gaugeBackgrnd"]);
	}

	void UIStatusBar::draw(float alpha) const
	{
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

		// Draw gauge cover on top of gauge background
		if (gauge_cover.is_valid())
			gauge_cover.draw(DrawArgument(position));

		// AB gauge cover variant
		if (jobid >= 2000 && jobid < 3000 && gauge_cover_ab.is_valid())
			gauge_cover_ab.draw(DrawArgument(position));

		// Draw gauges
		expbar.draw(position + Point<int16_t>(-261, -15));
		hpbar.draw(position + Point<int16_t>(-261, -31));
		mpbar.draw(position + Point<int16_t>(-90, -31));

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

		// Draw gauge animations when HP/MP is low
		float hp_pct = gethppercent();
		float mp_pct = getmppercent();

		if (hp_pct < 0.3f)
		{
			// Use AB variant for AB jobs
			if (jobid >= 2000 && jobid < 3000)
				ani_hp_gauge_ab.draw(DrawArgument(position + Point<int16_t>(-261, -31)), alpha);
			else
				ani_hp_gauge.draw(DrawArgument(position + Point<int16_t>(-261, -31)), alpha);
		}

		if (mp_pct < 0.3f)
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

		// Draw menu sub-panel background
		if (show_menu)
		{
			// 8 visible buttons * 27px = 216px tall, 67px wide, starting at screen (698, 441)
			ColorBox menu_bg(69, 220, Color::Name::BLACK, 0.75f);
			menu_bg.draw(DrawArgument(Point<int16_t>(698, 439)));
		}

		// Draw system sub-panel background
		if (show_system)
		{
			// 8 active buttons * 27px = 216px, starting at screen (774, 441)
			ColorBox sys_bg(69, 220, Color::Name::BLACK, 0.75f);
			sys_bg.draw(DrawArgument(Point<int16_t>(774, 439)));
		}

		// Draw quick slot panel
		if (show_quickslot)
		{
			if (quickslot_bg.is_valid())
				quickslot_bg.draw(DrawArgument(position));
		}

		// Draw buttons on top
		UIElement::draw_buttons(alpha);
	}

	void UIStatusBar::update()
	{
		int16_t VHEIGHT = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>(512, VHEIGHT);

		UIElement::update();

		expbar.update(getexppercent());
		hpbar.update(gethppercent());
		mpbar.update(getmppercent());

		namelabel.change_text(stats.get_name());
		joblabel.change_text(stats.get_jobname());

		// Update animations
		ani_hp_gauge.update();
		ani_mp_gauge.update();
		ap_notify.update();
		sp_notify.update();
		noncombat_notify.update();
	}

	Button::State UIStatusBar::button_pressed(uint16_t id)
	{
		switch (id)
		{
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
		{
			std::ofstream dbg("menu_debug.txt", std::ios::app);
			dbg << "BT_MENU clicked, show_menu=" << show_menu << std::endl;
			toggle_menu();
			dbg << "After toggle, show_menu=" << show_menu << std::endl;
			dbg << "menu_backgrnd valid=" << menu_backgrnd.is_valid() << std::endl;
			if (show_menu)
			{
				// Dump sub-panel button positions
				auto stat_bounds = buttons[BT_MENU_STAT]->bounds(position);
				dbg << "BT_MENU_STAT active=" << buttons[BT_MENU_STAT]->is_active()
					<< " bounds=(" << stat_bounds.get_left_top().x() << "," << stat_bounds.get_left_top().y()
					<< ")-(" << stat_bounds.get_right_bottom().x() << "," << stat_bounds.get_right_bottom().y() << ")" << std::endl;
				auto skill_bounds = buttons[BT_MENU_SKILL]->bounds(position);
				dbg << "BT_MENU_SKILL active=" << buttons[BT_MENU_SKILL]->is_active()
					<< " bounds=(" << skill_bounds.get_left_top().x() << "," << skill_bounds.get_left_top().y()
					<< ")-(" << skill_bounds.get_right_bottom().x() << "," << skill_bounds.get_right_bottom().y() << ")" << std::endl;
			}
			dbg.close();
			return Button::State::NORMAL;
		}

		case BT_OPTIONS:
		{
			std::ofstream dbg("menu_debug.txt", std::ios::app);
			dbg << "BT_OPTIONS clicked, show_system=" << show_system << std::endl;
			if (show_system)
			{
				show_system = false;
				// Hide system sub-panel buttons
				buttons[BT_SYS_CHANNEL]->set_active(false);
				buttons[BT_SYS_GAMEOPTION]->set_active(false);
				buttons[BT_SYS_GAMEQUIT]->set_active(false);
				buttons[BT_SYS_JOYPAD]->set_active(false);
				buttons[BT_SYS_KEYSETTING]->set_active(false);
				buttons[BT_SYS_MONSTERLIFE]->set_active(false);
				buttons[BT_SYS_OPTION]->set_active(false);
				buttons[BT_SYS_ROOMCHANGE]->set_active(false);
				buttons[BT_SYS_SYSTEMOPTION]->set_active(false);
			}
			else
			{
				// Close menu if open
				if (show_menu)
					toggle_menu();

				show_system = true;
				buttons[BT_SYS_CHANNEL]->set_active(true);
				buttons[BT_SYS_GAMEOPTION]->set_active(true);
				buttons[BT_SYS_GAMEQUIT]->set_active(true);
				buttons[BT_SYS_JOYPAD]->set_active(true);
				buttons[BT_SYS_KEYSETTING]->set_active(true);
				// BT_SYS_MONSTERLIFE stays disabled (post-BB)
				buttons[BT_SYS_OPTION]->set_active(true);
				buttons[BT_SYS_ROOMCHANGE]->set_active(true);
				buttons[BT_SYS_SYSTEMOPTION]->set_active(true);
			}
			dbg << "After toggle, show_system=" << show_system << std::endl;
			dbg.close();
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

		case BT_CHANNEL:
		case BT_SYS_CHANNEL:
			UI::get().emplace<UIChannel>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_NOTICE:
			return Button::State::NORMAL;

		case BT_SYS_GAMEQUIT:
			UI::get().emplace<UIQuit>(stats);
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_GAMEOPTION:
		case BT_SYS_OPTION:
		case BT_SYS_SYSTEMOPTION:
			UI::get().emplace<UIOptionMenu>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_ROOMCHANGE:
			UI::get().emplace<UIChannel>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_JOYPAD:
			UI::get().emplace<UIJoypad>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_SYS_MONSTERLIFE:
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
		case BT_MENU_EPISODBOOK:
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
		// Extend upward when menu or system sub-panel is open
		int16_t extra_height = (show_menu || show_system) ? 300 : 0;

		// Top edge: 84px above bar position + extra for sub-panels
		// Bottom edge: position.y() (screen bottom)
		Rectangle<int16_t> bounds(
			Point<int16_t>(position.x() - 512, position.y() - 84 - extra_height),
			Point<int16_t>(position.x() - 512 + dimension.x(), position.y())
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

			buttons[BT_MENU_STAT]->set_active(false);
			buttons[BT_MENU_SKILL]->set_active(false);
			buttons[BT_MENU_QUEST]->set_active(false);
			buttons[BT_MENU_ITEM]->set_active(false);
			buttons[BT_MENU_EQUIP]->set_active(false);
			buttons[BT_MENU_COMMUNITY]->set_active(false);
			buttons[BT_MENU_EVENT]->set_active(false);
			buttons[BT_MENU_RANK]->set_active(false);
			buttons[BT_MENU_EPISODBOOK]->set_active(false);
			buttons[BT_MENU_MONSTERBATTLE]->set_active(false);
			buttons[BT_MENU_MONSTERLIFE]->set_active(false);
			buttons[BT_MENU_MSN]->set_active(false);
			buttons[BT_MENU_AFREECATV]->set_active(false);
		}
		else
		{
			// Close system panel if open
			if (show_system)
			{
				show_system = false;
				buttons[BT_SYS_CHANNEL]->set_active(false);
				buttons[BT_SYS_GAMEOPTION]->set_active(false);
				buttons[BT_SYS_GAMEQUIT]->set_active(false);
				buttons[BT_SYS_JOYPAD]->set_active(false);
				buttons[BT_SYS_KEYSETTING]->set_active(false);
				buttons[BT_SYS_MONSTERLIFE]->set_active(false);
				buttons[BT_SYS_OPTION]->set_active(false);
				buttons[BT_SYS_ROOMCHANGE]->set_active(false);
				buttons[BT_SYS_SYSTEMOPTION]->set_active(false);
			}

			show_menu = true;

			buttons[BT_MENU_STAT]->set_active(true);
			buttons[BT_MENU_SKILL]->set_active(true);
			buttons[BT_MENU_QUEST]->set_active(true);
			buttons[BT_MENU_ITEM]->set_active(true);
			buttons[BT_MENU_EQUIP]->set_active(true);
			buttons[BT_MENU_COMMUNITY]->set_active(true);
			buttons[BT_MENU_EVENT]->set_active(true);
			buttons[BT_MENU_RANK]->set_active(true);
			// Post-BB buttons stay disabled
			// BT_MENU_EPISODBOOK, BT_MENU_MONSTERBATTLE, BT_MENU_MONSTERLIFE, BT_MENU_MSN, BT_MENU_AFREECATV
		}
	}

	void UIStatusBar::remove_menus()
	{
		if (show_menu)
			toggle_menu();

		if (show_system)
		{
			show_system = false;
			buttons[BT_SYS_CHANNEL]->set_active(false);
			buttons[BT_SYS_GAMEOPTION]->set_active(false);
			buttons[BT_SYS_GAMEQUIT]->set_active(false);
			buttons[BT_SYS_JOYPAD]->set_active(false);
			buttons[BT_SYS_KEYSETTING]->set_active(false);
			buttons[BT_SYS_MONSTERLIFE]->set_active(false);
			buttons[BT_SYS_OPTION]->set_active(false);
			buttons[BT_SYS_ROOMCHANGE]->set_active(false);
			buttons[BT_SYS_SYSTEMOPTION]->set_active(false);
		}
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
}
