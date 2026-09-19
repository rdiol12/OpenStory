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
#include <cstring>
#include <vector>

#include "../../Configuration.h"
#include "../../Graphics/Geometry.h"
#include "../NotificationCenter.h"
#include "UIBuddyList.h"
#include "UIChannel.h"
#include "UIEquipInventory.h"
#include "UIEvent.h"
#include "UIMessenger.h"
#include "UIMonsterBattle.h"
#include "UIMonsterLife.h"
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
#include "UIChat.h"
#include "UIFarmChat.h"
#include "UIWhisper.h"

#include "../../Net/OutPacket.h"
#include "../../Net/Session.h"
#include "../../IO/Window.h"

#include "../UI.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"


#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// === Quickslot layout constants (used by draw() and the slot helpers) ===
	// Panel: quickSlot bitmap is 145x93 with origin (-143, 143), so drawing at
	// `position` places the panel top-left at (position.x + 143, position.y - 143).
	static constexpr int16_t QS_PANEL_OFFSET_X = 143;
	static constexpr int16_t QS_PANEL_OFFSET_Y = -143;
	// Extra lift applied to the whole opened quickslot (panel + icons + labels).
	// 0 = original position (the panel itself is fine where it is).
	static constexpr int16_t QS_LIFT = 0;
	// Upward shift for the quickslot arrow button in the OPEN state (BtClose).
	static constexpr int16_t QS_OPEN_BTN_LIFT = 15;
	// Vertical nudge (negative = up) for the cube icon and the key-name label.
	static constexpr int16_t QS_ICON_NUDGE_Y  = -8;
	static constexpr int16_t QS_LABEL_NUDGE_Y = -14;
	// Cube grid, measured from the StatusBar2 panel: cubes start at x=8,41,74,107
	// (step 33, ~28 wide) and y=16,49 (step 33, ~28 tall).
	static constexpr int16_t QS_CELL_OFFSET_X = 8;
	static constexpr int16_t QS_CELL_OFFSET_Y = 16;
	static constexpr int16_t QS_CELL_W   = 28;
	static constexpr int16_t QS_CELL_H   = 28;
	static constexpr int16_t QS_COL_STEP = 33;
	static constexpr int16_t QS_ROW_STEP = 33;

	// === Stock-v83 (StatusBar.img) geometry ===============================
	// All offsets are "bar-local": measured from the top-left corner of the
	// 800x71 `base/backgrnd` strip, which is anchored to the bottom-left of
	// the view. The art splits into a white chat row (y 3..29) above a
	// blue-grey instrument row (y 34..70), so the two button sizes v83 ships
	// (28x20 icons and 54x34 plates) each have a row that fits them.
	static constexpr int16_t V83_BAR_H      = 71;
	static constexpr int16_t V83_CHATROW_Y  = 5;   // top of the 19/20px chat-row controls
	static constexpr int16_t V83_BAND_Y     = 34;  // top of the instrument row
	static constexpr int16_t V83_EDGE_PAD   = 8;

	// `gauge/bar` is one 340x31 sheet: rows 0..14 hold the HP / MP / EXP
	// captions and rows 15..30 the three coloured fills, side by side. The
	// x/width pairs below were measured off the sheet's alpha runs.
	static constexpr int16_t V83_GAUGE_W    = 340;
	static constexpr int16_t V83_GAUGE_H    = 31;
	static constexpr int16_t V83_FILL_Y     = 15;
	static constexpr int16_t V83_FILL_H     = 16;
	static constexpr int16_t V83_HP_X       = 2,   V83_HP_W  = 105;
	static constexpr int16_t V83_MP_X       = 110, V83_MP_W  = 105;
	static constexpr int16_t V83_EXP_X      = 223, V83_EXP_W = 115;

	// Where the gauge sheet sits inside the bar (left of it: level, name, job).
	static constexpr int16_t V83_GAUGE_BX   = 156;
	static constexpr int16_t V83_GAUGE_BY   = 37;
	static constexpr int16_t V83_TEXT_X     = 10;
	static constexpr int16_t V83_NAME_Y     = 34;
	static constexpr int16_t V83_JOB_Y      = 50;

	static constexpr int16_t V83_BIG_W      = 54, V83_BIG_H   = 34, V83_BIG_STEP   = 56;
	static constexpr int16_t V83_SMALL_W    = 28, V83_SMALL_H = 20, V83_SMALL_STEP = 30;
	static constexpr int16_t V83_SMALL_N    = 6;  // Stat, Inven, Equip, Skill, KeySet, QuickSlot

	// Quickslot cube grid, measured off `base/quickSlot` (151x80): the frame
	// rules sit at x 6/40/75/110/144 and y 7/40/72, leaving 32x30 interiors.
	static constexpr int16_t QS83_CELL_OFFSET_X = 7;
	static constexpr int16_t QS83_CELL_OFFSET_Y = 8;
	static constexpr int16_t QS83_CELL_W   = 32;
	static constexpr int16_t QS83_CELL_H   = 30;
	static constexpr int16_t QS83_COL_STEP = 35;
	static constexpr int16_t QS83_ROW_STEP = 34;
	static constexpr int16_t QS83_PANEL_W  = 151;
	static constexpr int16_t QS83_PANEL_H  = 80;

	// Text-row pop-up metrics (stock v83 ships no Menu/System panel art).
	static constexpr int16_t V83_ROW_H    = 18;
	static constexpr int16_t V83_LIST_W   = 118;
	static constexpr int16_t V83_LIST_PAD = 4;

	// `StatusBar.img/number` names its punctuation glyphs "Lbracket",
	// "Rbracket", "slash" and "percent"; Charset keys every glyph by the
	// FIRST character of its node name, so it can only look them up as
	// 'L', 'R', 's' and 'p'. Rewriting the display string is enough - the
	// digits keep their own names and nothing collides.
	static std::string v83_numstr(const std::string& text)
	{
		std::string out;
		out.reserve(text.size());

		for (char c : text)
		{
			switch (c)
			{
			case '[': out.push_back('L'); break;
			case ']': out.push_back('R'); break;
			case '/': out.push_back('s'); break;
			case '%': out.push_back('p'); break;
			default:  out.push_back(c);   break;
			}
		}

		return out;
	}

	// Copies a sub-rectangle out of an NX bitmap into a standalone Texture.
	// Needed because v83 packs the three gauge fills into one sheet, while
	// the shared Gauge component wants one texture per gauge. `key` names the
	// atlas slot, so repeated construction of the status bar reuses it.
	static Texture v83_crop(nl::node src, const std::string& key,
		int16_t x, int16_t y, int16_t w, int16_t h)
	{
		if (src.data_type() != nl::node::type::bitmap)
			return Texture();

		nl::bitmap bmp = src.get_bitmap();

		if (!bmp)
			return Texture();

		auto sw = static_cast<int32_t>(bmp.width());
		auto sh = static_cast<int32_t>(bmp.height());

		if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > sw || y + h > sh)
			return Texture();

		// data() decompresses on the fly and invalidates any pointer handed
		// out earlier, so the copy has to finish before the next call.
		const auto* pixels = static_cast<const uint8_t*>(bmp.data());

		if (!pixels)
			return Texture();

		std::vector<uint8_t> out(static_cast<size_t>(w) * h * 4);

		for (int16_t row = 0; row < h; row++)
			std::memcpy(
				out.data() + static_cast<size_t>(row) * w * 4,
				pixels + (static_cast<size_t>(y + row) * sw + x) * 4,
				static_cast<size_t>(w) * 4);

		return Texture::from_pixels(key, w, h, std::move(out), Point<int16_t>(0, 0));
	}

	// Short display name for a maple/DIK quickslot keycode (drawn on each cube).
	static std::string qs_keyname(uint8_t code)
	{
		switch (code)
		{
		// letters
		case 16: return "Q"; case 17: return "W"; case 18: return "E"; case 19: return "R";
		case 20: return "T"; case 21: return "Y"; case 22: return "U"; case 23: return "I";
		case 24: return "O"; case 25: return "P";
		case 30: return "A"; case 31: return "S"; case 32: return "D"; case 33: return "F";
		case 34: return "G"; case 35: return "H"; case 36: return "J"; case 37: return "K";
		case 38: return "L";
		case 44: return "Z"; case 45: return "X"; case 46: return "C"; case 47: return "V";
		case 48: return "B"; case 49: return "N"; case 50: return "M";
		// number row
		case 2: return "1"; case 3: return "2"; case 4: return "3"; case 5: return "4";
		case 6: return "5"; case 7: return "6"; case 8: return "7"; case 9: return "8";
		case 10: return "9"; case 11: return "0";
		// function keys
		case 59: return "F1"; case 60: return "F2"; case 61: return "F3"; case 62: return "F4";
		case 63: return "F5"; case 64: return "F6"; case 65: return "F7"; case 66: return "F8";
		case 67: return "F9"; case 68: return "F10"; case 87: return "F11"; case 88: return "F12";
		// modifiers / navigation (the default quickslot set)
		case 42: case 54: return "Sh";
		case 29: case 157: return "Ct";
		case 56: return "Al";
		case 82: return "Ins"; case 71: return "Hm"; case 73: return "PU";
		case 83: return "Del"; case 79: return "End"; case 81: return "PD";
		case 57: return "Sp"; case 15: return "Tab"; case 14: return "BS";
		default: return std::to_string(static_cast<int>(code));
		}
	}

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

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];
		nl::node chat = nl::nx::ui["StatusBar2.img"]["chat"];

		// Node probe, not a version check: stock v83 UI.wz has no
		// StatusBar2.img at all, and NoLifeNx answers a missing path with a
		// null node rather than an error, so every lookup below would quietly
		// produce an empty Texture. Falling back to StatusBar.img keeps the
		// bar drawn on v83 assets while later-version art still wins here.
		v83_layout = !mainbar || mainbar.size() == 0;

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

		// === Labels ===
		// Shared by both layouts: the fonts are compiled into the binary, so
		// these keep working whatever the NX build ships.
		joblabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW);
		namelabel = Text(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE);
		// Bold white key label drawn in each quickslot cube's corner.
		qs_key_label = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::WHITE);
		v83_menu_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);

		// The quickslot key caps live in StatusBar.img in every build.
		nl::node keynames = nl::nx::ui["StatusBar.img"]["key"];
		for (int i = 0; i < 8; i++)
			qs_key_sprites[i] = keynames[std::to_string(i)];

		if (v83_layout)
		{
			build_v83();
			return;
		}

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

		nl::node energy_node = nl::nx::ui["UIWindow.img"]["EnergyBar"];

		energy_bar_c = energy_node["c"];
		energy_bar_e = energy_node["e"];
		energy_fill = energy_node["Gage"]["1"]["0"];

		if (energy_node["effect"])
			energy_full_effect = Animation(energy_node["effect"]);

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

		// Chat targets are handled by the chat bar's own To: button
		// (UIChatBar cycles All/Buddy/Guild/Alliance/Party and sends the
		// matching MultiChat packet) — the old half-built tab row here
		// was never positioned and never wired, so it's gone.
		nl::node chat_scroll = chat["scroll"];
		chat_scroll_normal = Texture(chat_scroll["normal"]);
		chat_scroll_over = Texture(chat_scroll["over"]);

		// === Quick slot ===
		nl::node qs = mainbar["quickSlot"];
		// Prefer the v83 `StatusBar.img/base/quickSlot` bitmap — that
		// one has the Shft/Ctl/Ins/Del/Hme/End/PgU/PgD key labels
		// baked into the panel frame. Only when it isn't present in
		// this NX build do we fall back to the StatusBar2 variant
		// (`mainBar/quickSlot/quickSlot`), which is a blank frame and
		// would need programmatic labels — left to a separate path.
		nl::node statusbar_v83 = nl::nx::ui["StatusBar.img"];
		nl::node v83_labeled = statusbar_v83["base"]["quickSlot"];
		if (v83_labeled)
		{
			quickslot_bg = Texture(v83_labeled);
			quickslot_bg_v83 = true;
		}
		else
		{
			quickslot_bg = Texture(qs["quickSlot"]);
			quickslot_bg_v83 = false;
		}

		buttons[BT_QS_OPEN]  = std::make_unique<MapleButton>(qs["BtOpen"]);
		buttons[BT_QS_OPEN]->set_active(true);
		// Open state: nudge the "close quickslot" arrow up from its sprite spot.
		buttons[BT_QS_CLOSE] = std::make_unique<MapleButton>(qs["BtClose"], Point<int16_t>(0, -QS_OPEN_BTN_LIFT));
		buttons[BT_QS_CLOSE]->set_active(false);

		// === Menu sub-panel ===
		nl::node menu_node = mainbar["Menu"];

		// Menu buttons stack top-to-bottom above the bar.
		// Bar top is at -84 relative to position. Panel sits just above that.
		// v83-visible: Stat, Skill, Quest, Item, Equip, Community, Event, Rank, EpisodBook, MSN
		constexpr int16_t MENU_STEP = 26;
		constexpr int16_t MENU_VISIBLE = 12;
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
		buttons[BT_MENU_MSN]           = std::make_unique<MapleButton>(menu_node["BtMSN"],       Point<int16_t>(menu_x, menu_y + MENU_STEP * 9));
		buttons[BT_MENU_MONSTERBATTLE] = std::make_unique<MapleButton>(menu_node["BtMonsterBattle"], Point<int16_t>(menu_x, menu_y + MENU_STEP * 10));
		buttons[BT_MENU_MONSTERLIFE]   = std::make_unique<MapleButton>(menu_node["BtMonsterLife"], Point<int16_t>(menu_x, menu_y + MENU_STEP * 11));

		// Monster Battle / Monster Life / AfreecaTV have no Cosmic counterpart.

		// All menu buttons hidden until menu is toggled
		for (uint16_t i = BT_MENU_STAT; i <= BT_MENU_MONSTERLIFE; i++)
			buttons[i]->set_active(false);

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

	// =====================================================================
	// Stock-v83 layout (UI.wz/StatusBar.img)
	// =====================================================================

	namespace
	{
		// Pop-up row captions, in draw order. The ids they pair with are the
		// existing BT_MENU_* / BT_SYS_* buttons, so button_pressed() already
		// knows what each row opens; only the artwork changes.
		const char* const V83_MENU_LABELS[] = {
			"Stats", "Skills", "Quests", "Inventory", "Equipment",
			"Buddy List", "Event", "Ranking", "Monster Book", "Messenger",
			"Monster Battle", "Monster Life"
		};

		const char* const V83_SYS_LABELS[] = {
			"Change Channel", "Joypad", "Keyboard Setting", "Options", "Quit Game"
		};
	}

	Point<int16_t> UIStatusBar::v83_at(int16_t lx, int16_t ly) const
	{
		// `position` is (512, VHEIGHT); the v83 bar's top-left corner is the
		// bottom-left corner of the view, V83_BAR_H above the baseline.
		return Point<int16_t>(static_cast<int16_t>(lx - 512),
		                      static_cast<int16_t>(ly - V83_BAR_H));
	}

	void UIStatusBar::build_v83()
	{
		nl::node sb = nl::nx::ui["StatusBar.img"];
		nl::node base = sb["base"];
		nl::node gauge = sb["gauge"];

		v83_backgrnd     = Texture(base["backgrnd"]);
		v83_gauge_track  = Texture(gauge["graduation"]);
		v83_notice_box   = Texture(base["box"]);
		v83_icon_memo    = Texture(base["iconMemo"]);
		v83_icon_red     = Texture(base["iconRed"]);

		// Split the single 340x31 `gauge/bar` sheet into the caption strip
		// plus one fill texture per gauge, then hand each fill to the shared
		// Gauge component (single-texture ctor stretches it to the percentage).
		nl::node barnode = gauge["bar"];

		v83_gauge_labels = v83_crop(barnode, "sb83/labels", 0, 0, V83_GAUGE_W, V83_FILL_Y);

		hpbar = Gauge(Gauge::Type::GAME,
			v83_crop(barnode, "sb83/hp", V83_HP_X, V83_FILL_Y, V83_HP_W, V83_FILL_H),
			V83_HP_W, 0.0f);
		mpbar = Gauge(Gauge::Type::GAME,
			v83_crop(barnode, "sb83/mp", V83_MP_X, V83_FILL_Y, V83_MP_W, V83_FILL_H),
			V83_MP_W, 0.0f);
		expbar = Gauge(Gauge::Type::GAME,
			v83_crop(barnode, "sb83/exp", V83_EXP_X, V83_FILL_Y, V83_EXP_W, V83_FILL_H),
			V83_EXP_W, 0.0f);

		v83_hp_flash = Animation(gauge["hpFlash"]);
		v83_mp_flash = Animation(gauge["mpFlash"]);

		// RIGHT alignment so the readouts end flush with each gauge's tip.
		statset = Charset(sb["number"], Charset::Alignment::RIGHT);

		// The quickslot frame with the key captions baked in.
		quickslot_bg = Texture(base["quickSlot"]);
		quickslot_bg_v83 = true;

		// --- chat-row icon buttons -------------------------------------
		buttons[BT_STATS]      = std::make_unique<MapleButton>(sb["StatKey"]);
		buttons[BT_INVENTORY]  = std::make_unique<MapleButton>(sb["InvenKey"]);
		buttons[BT_EQUIPS]     = std::make_unique<MapleButton>(sb["EquipKey"]);
		buttons[BT_SKILL]      = std::make_unique<MapleButton>(sb["SkillKey"]);
		buttons[BT_KEYSETTING] = std::make_unique<MapleButton>(sb["KeySet"]);
		buttons[BT_QS_OPEN]    = std::make_unique<MapleButton>(sb["QuickSlot"]);
		buttons[BT_QS_CLOSE]   = std::make_unique<MapleButton>(sb["QuickSlotD"]);
		buttons[BT_QS_CLOSE]->set_active(false);

		buttons[BT_CALLGM]  = std::make_unique<MapleButton>(sb["BtClaim"]);
		buttons[BT_WHISPER] = std::make_unique<MapleButton>(sb["BtWhisper"]);

		// AP / SP blink overlays; v83 ships them as the buttons' `ani` state.
		v83_stat_ani  = Animation(sb["StatKey"]["ani"]);
		v83_skill_ani = Animation(sb["SkillKey"]["ani"]);

		// --- instrument-row plates -------------------------------------
		// MENU and SHORTCUT open the two text pop-ups; SHOP and TRADE map
		// straight onto the cash shop and MTS, matching their captions.
		buttons[BT_MENU]     = std::make_unique<MapleButton>(sb["BtMenu"]);
		buttons[BT_CASHSHOP] = std::make_unique<MapleButton>(sb["BtShop"]);
		buttons[BT_OPTIONS]  = std::make_unique<MapleButton>(sb["BtShort"]);
		buttons[BT_TRADE]    = std::make_unique<MapleButton>(sb["BtNPT"]);

		// --- notice indicator ------------------------------------------
		// `base/box` is a 42x19 two-cell frame; the left cell doubles as the
		// notification drawer's button.
		buttons[BT_NOTICE] = std::make_unique<AreaButton>(
			Point<int16_t>(0, 0), Point<int16_t>(21, 19));

		// --- pop-up rows -----------------------------------------------
		// Stock v83 has no Menu/System panel artwork, so the rows are
		// hit-boxes drawn as text by draw_v83_list().
		for (uint16_t id = BT_MENU_STAT; id <= BT_MENU_MONSTERLIFE; id++)
		{
			buttons[id] = std::make_unique<AreaButton>(
				Point<int16_t>(0, 0), Point<int16_t>(V83_LIST_W, V83_ROW_H));
			buttons[id]->set_active(false);
		}

		for (uint16_t id : { BT_SYS_CHANNEL, BT_SYS_JOYPAD, BT_SYS_KEYSETTING,
			BT_SYS_OPTION, BT_SYS_GAMEQUIT })
		{
			buttons[id] = std::make_unique<AreaButton>(
				Point<int16_t>(0, 0), Point<int16_t>(V83_LIST_W, V83_ROW_H));
			buttons[id]->set_active(false);
		}

		layout_v83();
	}

	void UIStatusBar::layout_v83()
	{
		int16_t vwidth = Constants::Constants::get().get_viewwidth();
		v83_laid_out_width = vwidth;

		// The four 54x34 plates hug the right edge of the instrument row.
		int16_t big_x = static_cast<int16_t>(
			vwidth - V83_EDGE_PAD - (4 * V83_BIG_W + 3 * (V83_BIG_STEP - V83_BIG_W)));
		int16_t big_y = V83_BAND_Y + 1;

		buttons[BT_MENU]    ->set_position(v83_at(big_x,                    big_y));
		buttons[BT_CASHSHOP]->set_position(v83_at(big_x + V83_BIG_STEP,     big_y));
		buttons[BT_OPTIONS] ->set_position(v83_at(big_x + V83_BIG_STEP * 2, big_y));
		buttons[BT_TRADE]   ->set_position(v83_at(big_x + V83_BIG_STEP * 3, big_y));

		// The six 28x20 icons sit on the chat row, right-aligned above them.
		int16_t small_x = static_cast<int16_t>(
			vwidth - V83_EDGE_PAD
			- (V83_SMALL_N * V83_SMALL_W + (V83_SMALL_N - 1) * (V83_SMALL_STEP - V83_SMALL_W)));

		const uint16_t icon_order[V83_SMALL_N] = {
			BT_STATS, BT_INVENTORY, BT_EQUIPS, BT_SKILL, BT_KEYSETTING, BT_QS_OPEN
		};

		for (int16_t i = 0; i < V83_SMALL_N; i++)
			buttons[icon_order[i]]->set_position(
				v83_at(static_cast<int16_t>(small_x + i * V83_SMALL_STEP), V83_CHATROW_Y));

		// The close arrow replaces the open arrow in the same cell.
		buttons[BT_QS_CLOSE]->set_position(
			v83_at(static_cast<int16_t>(small_x + (V83_SMALL_N - 1) * V83_SMALL_STEP), V83_CHATROW_Y));

		v83_statkey_pos  = v83_at(small_x, V83_CHATROW_Y);
		v83_skillkey_pos = v83_at(static_cast<int16_t>(small_x + 3 * V83_SMALL_STEP), V83_CHATROW_Y);

		int16_t claim_x   = static_cast<int16_t>(small_x - 8 - 20);
		int16_t whisper_x = static_cast<int16_t>(claim_x - 3 - 12);
		int16_t box_x     = static_cast<int16_t>(whisper_x - 6 - 42);

		buttons[BT_CALLGM] ->set_position(v83_at(claim_x,   V83_CHATROW_Y));
		buttons[BT_WHISPER]->set_position(v83_at(whisper_x, V83_CHATROW_Y));
		buttons[BT_NOTICE] ->set_position(v83_at(box_x,     V83_CHATROW_Y));
		v83_notice_pos = v83_at(box_x, V83_CHATROW_Y);

		v83_gauge_pos = v83_at(V83_GAUGE_BX, V83_GAUGE_BY);

		// Quickslot panel floats just above the bar, under its toggle button.
		v83_quickslot_pos = v83_at(
			static_cast<int16_t>(vwidth - V83_EDGE_PAD - QS83_PANEL_W),
			static_cast<int16_t>(-QS83_PANEL_H - 2));

		// Pop-up columns grow upward from the top edge of the bar.
		constexpr size_t MENU_ROWS = BT_MENU_MONSTERLIFE - BT_MENU_STAT + 1;
		constexpr size_t SYS_ROWS = 5;

		int16_t menu_top = static_cast<int16_t>(
			-(static_cast<int16_t>(MENU_ROWS) * V83_ROW_H) - V83_LIST_PAD * 2 - 2);
		int16_t sys_top = static_cast<int16_t>(
			-(static_cast<int16_t>(SYS_ROWS) * V83_ROW_H) - V83_LIST_PAD * 2 - 2);

		// Each column hangs under the plate that opens it, pulled back inside
		// the right edge when that would overflow the view.
		constexpr int16_t LIST_TOTAL_W = V83_LIST_W + V83_LIST_PAD * 2;
		auto clamp_list_x = [&](int16_t x)
		{
			return std::min<int16_t>(x, static_cast<int16_t>(vwidth - V83_EDGE_PAD - LIST_TOTAL_W));
		};

		v83_menu_list_pos = v83_at(clamp_list_x(big_x), menu_top);
		v83_sys_list_pos = v83_at(
			clamp_list_x(static_cast<int16_t>(big_x + V83_BIG_STEP * 2)), sys_top);

		for (size_t i = 0; i < MENU_ROWS; i++)
			buttons[static_cast<uint16_t>(BT_MENU_STAT + i)]->set_position(
				v83_menu_list_pos
				+ Point<int16_t>(V83_LIST_PAD,
					static_cast<int16_t>(V83_LIST_PAD + i * V83_ROW_H)));

		const uint16_t sys_order[SYS_ROWS] = {
			BT_SYS_CHANNEL, BT_SYS_JOYPAD, BT_SYS_KEYSETTING, BT_SYS_OPTION, BT_SYS_GAMEQUIT
		};

		for (size_t i = 0; i < SYS_ROWS; i++)
			buttons[sys_order[i]]->set_position(
				v83_sys_list_pos
				+ Point<int16_t>(V83_LIST_PAD,
					static_cast<int16_t>(V83_LIST_PAD + i * V83_ROW_H)));
	}

	void UIStatusBar::draw_v83_list(const uint16_t* ids, size_t count,
		const char* const* labels, Point<int16_t> topleft, float fade) const
	{
		if (fade <= 0.0f || count == 0)
			return;

		Point<int16_t> tl = position + topleft;
		auto panel_h = static_cast<int16_t>(count * V83_ROW_H + V83_LIST_PAD * 2);

		ColorBox backdrop(static_cast<int16_t>(V83_LIST_W + V83_LIST_PAD * 2), panel_h,
			Color::Name::BLACK, 0.82f * fade);
		backdrop.draw(DrawArgument(tl));

		for (size_t i = 0; i < count; i++)
		{
			auto row_tl = tl + Point<int16_t>(V83_LIST_PAD,
				static_cast<int16_t>(V83_LIST_PAD + i * V83_ROW_H));

			auto iter = buttons.find(ids[i]);

			if (iter != buttons.end() && iter->second
				&& iter->second->get_state() == Button::State::MOUSEOVER)
			{
				ColorBox hover(V83_LIST_W, V83_ROW_H, Color::Name::WHITE, 0.22f * fade);
				hover.draw(DrawArgument(row_tl));
			}

			v83_menu_label.change_text(labels[i]);
			v83_menu_label.draw(DrawArgument(row_tl + Point<int16_t>(5, 1), fade));
		}
	}

	void UIStatusBar::draw_v83(float alpha) const
	{
		int16_t vwidth = Constants::Constants::get().get_viewwidth();

		// Quickslot panel first so the bar composites over anything that
		// bleeds into it.
		if (show_quickslot && quickslot_bg.is_valid())
			quickslot_bg.draw(DrawArgument(position + v83_quickslot_pos));

		if (show_quickslot)
			draw_quickslot_cells();

		// The 800x71 strip is a plain horizontal gradient, so stretching it
		// across a wider view is seamless.
		if (v83_backgrnd.is_valid())
			v83_backgrnd.draw(DrawArgument(
				position + v83_at(0, 0), Point<int16_t>(vwidth, V83_BAR_H)));

		Point<int16_t> gauge_tl = position + v83_gauge_pos;

		if (v83_gauge_track.is_valid())
			v83_gauge_track.draw(DrawArgument(gauge_tl));

		if (v83_gauge_labels.is_valid())
			v83_gauge_labels.draw(DrawArgument(gauge_tl));

		Point<int16_t> hp_tl = gauge_tl + Point<int16_t>(V83_HP_X, V83_FILL_Y);
		Point<int16_t> mp_tl = gauge_tl + Point<int16_t>(V83_MP_X, V83_FILL_Y);
		Point<int16_t> exp_tl = gauge_tl + Point<int16_t>(V83_EXP_X, V83_FILL_Y);

		hpbar.draw(hp_tl);
		mpbar.draw(mp_tl);
		expbar.draw(exp_tl);

		// Low-HP / low-MP blink, thresholds from System -> Options.
		float hp_warn = Setting<HPWarning>::get().load() / 100.0f;
		float mp_warn = Setting<MPWarning>::get().load() / 100.0f;

		if (gethppercent() < hp_warn || hp_flash_ticks > 0)
			v83_hp_flash.draw(DrawArgument(hp_tl - Point<int16_t>(2, 1)), alpha);

		if (getmppercent() < mp_warn || mp_flash_ticks > 0)
			v83_mp_flash.draw(DrawArgument(mp_tl - Point<int16_t>(2, 1)), alpha);

		// Readouts, right-aligned inside each gauge.
		int32_t hp = stats.get_stat(MapleStat::Id::HP);
		int32_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);
		int64_t exp = stats.get_exp();

		constexpr int16_t NUM_INSET = 3;
		constexpr int16_t NUM_Y = 5;

		statset.draw(
			v83_numstr("[" + std::to_string(hp) + "/" + std::to_string(maxhp) + "]"),
			hp_tl + Point<int16_t>(V83_HP_W - NUM_INSET, NUM_Y));
		statset.draw(
			v83_numstr("[" + std::to_string(mp) + "/" + std::to_string(maxmp) + "]"),
			mp_tl + Point<int16_t>(V83_MP_W - NUM_INSET, NUM_Y));

		std::string expstring = std::to_string(100 * getexppercent());
		statset.draw(
			v83_numstr(std::to_string(exp) + "["
				+ expstring.substr(0, expstring.find('.') + 3) + "%]"),
			exp_tl + Point<int16_t>(V83_EXP_W - NUM_INSET, NUM_Y));

		// Name and "Lv.N Job" fill the empty space left of the gauges.
		namelabel.draw(position + v83_at(V83_TEXT_X, V83_NAME_Y));
		joblabel.draw(position + v83_at(V83_TEXT_X, V83_JOB_Y));

		UIElement::draw_buttons(alpha);

		// The notice frame sits behind its (art-less) AreaButton.
		if (v83_notice_box.is_valid())
			v83_notice_box.draw(DrawArgument(position + v83_notice_pos));

		const Texture& notice_icon = has_notification ? v83_icon_red : v83_icon_memo;

		if (notice_icon.is_valid())
			notice_icon.draw(DrawArgument(position + v83_notice_pos
				+ Point<int16_t>(4, static_cast<int16_t>((19 - notice_icon.height()) / 2))));

		// v83 blinks the Stat / Skill shortcut icons while points are unspent.
		if (stats.get_stat(MapleStat::Id::AP) > 0)
			v83_stat_ani.draw(DrawArgument(position + v83_statkey_pos), alpha);

		if (stats.get_stat(MapleStat::Id::SP) > 0)
			v83_skill_ani.draw(DrawArgument(position + v83_skillkey_pos), alpha);

		// Pop-ups last so they sit over everything else in the bar.
		constexpr size_t MENU_ROWS = BT_MENU_MONSTERLIFE - BT_MENU_STAT + 1;
		uint16_t menu_ids[MENU_ROWS];

		for (size_t i = 0; i < MENU_ROWS; i++)
			menu_ids[i] = static_cast<uint16_t>(BT_MENU_STAT + i);

		draw_v83_list(menu_ids, MENU_ROWS, V83_MENU_LABELS,
			v83_menu_list_pos, menu_fade);

		static const uint16_t sys_ids[] = {
			BT_SYS_CHANNEL, BT_SYS_JOYPAD, BT_SYS_KEYSETTING, BT_SYS_OPTION, BT_SYS_GAMEQUIT
		};

		draw_v83_list(sys_ids, 5, V83_SYS_LABELS, v83_sys_list_pos, sys_fade);
	}

	void UIStatusBar::update_screen(int16_t, int16_t)
	{
		if (v83_layout)
			layout_v83();
	}

	void UIStatusBar::update_boss_hp(const std::string& name, int8_t percent)
	{
		// 0% (or below) means the boss died / the tag cleared — hide the gauge.
		if (percent <= 0)
		{
			boss_hp_ticks = 0;
			boss_gage.clear();
			return;
		}

		boss_gage.set_mob(0, name, 0);
		boss_hp_percent = std::clamp(static_cast<float>(percent) / 100.0f, 0.0f, 1.0f);
		boss_hp_ticks = 250; // ~4s of fixed-step updates; refreshed on every report
	}

	void UIStatusBar::draw(float alpha) const
	{
		int16_t vwidth = Constants::Constants::get().get_viewwidth();

		// Boss HP gauge, centred near the top of the screen while a boss is
		// reporting its HP (auto-hides via boss_hp_ticks in update()).
		if (boss_hp_ticks > 0 && boss_gage.is_active())
		{
			Point<int16_t> boss_pos(
				static_cast<int16_t>((vwidth - boss_gage.width()) / 2), 18);
			boss_gage.draw(boss_pos, boss_hp_percent);
		}

		// Stock v83 assets need their own layout; everything below this point
		// reads StatusBar2/StatusBar3 nodes that such a build does not ship.
		if (v83_layout)
		{
			draw_v83(alpha);
			return;
		}

		if (Stage::get().is_energy_active() && energy_bar_c.is_valid())
		{
			constexpr int16_t ENERGY_MAX = 10000;
			constexpr int16_t ENERGY_WIDTH = 100;

			int32_t amount = Stage::get().get_energy();
			int16_t filled = static_cast<int16_t>(
				static_cast<int64_t>(amount) * ENERGY_WIDTH / ENERGY_MAX);

			Point<int16_t> ep((vwidth - ENERGY_WIDTH) / 2, position.y() - 62);

			for (int16_t x = 0; x < ENERGY_WIDTH; x++)
				energy_bar_c.draw(DrawArgument(ep + Point<int16_t>(x, 0)));

			energy_bar_e.draw(DrawArgument(ep + Point<int16_t>(-energy_bar_e.width(), 0)));
			energy_bar_e.draw(DrawArgument(ep + Point<int16_t>(ENERGY_WIDTH, 0)));

			if (energy_fill.is_valid())
				for (int16_t x = 0; x < filled; x++)
					energy_fill.draw(DrawArgument(ep + Point<int16_t>(x, 2)));

			if (amount >= ENERGY_MAX)
				energy_full_effect.draw(DrawArgument(ep + Point<int16_t>(ENERGY_WIDTH / 2, 0)), alpha);
		}

		// Quickslot panel — drawn FIRST so the status bar renders on
		// top of it. The panel sits above the main bar on screen, but
		// any overlap with other status bar sprites goes to the bar.
		if (show_quickslot)
		{
			if (quickslot_bg.is_valid())
			{
				// The v83 base sprite has no origin, so it must be drawn at
				// the panel position (above the bar, where the slots are).
				// The StatusBar2 fallback self-positions from `position`.
				// Drawing at `position` was hiding the base panel behind the
				// status bar — only the slot icons showed.
				// v83 panel draws at the (already lifted) panel pos; the StatusBar2
				// fallback self-positions from `position` via its origin, so lift it
				// the same amount here to keep it aligned with the slot cells.
				Point<int16_t> bgpos = quickslot_bg_v83
					? quickslot_panel_pos()
					: position + Point<int16_t>(0, -QS_LIFT);
				quickslot_bg.draw(DrawArgument(bgpos));
			}

			draw_quickslot_cells();
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
		int32_t hp = stats.get_stat(MapleStat::Id::HP); // uint16_t -> int32; avoid int16 overflow > 32767
		int32_t mp = stats.get_stat(MapleStat::Id::MP);
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

		// Pending-notification badge above the bell button. Set by
		// `notify()` whenever a quest completes, an invite arrives, an
		// item-effect msg fires, etc., and cleared once the player
		// opens the notification drawer or otherwise resolves it.
		// notice_sprite carries its own NX origin so a plain draw at
		// `position` lands it on top of BtNotice.
		if (has_notification && notice_sprite.is_valid())
			notice_sprite.draw(DrawArgument(position));

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
			draw_subpanel(*buttons.at(BT_MENU_STAT),
			              *buttons.at(BT_MENU_MONSTERLIFE),
			              menu_bg_top, menu_bg_mid, menu_bg_bot, menu_fade);
			// Fade the buttons alongside the backdrop (alpha overload
			// ignores the `active` flag so the visual fade is smooth).
			for (uint16_t i = BT_MENU_STAT; i <= BT_MENU_MONSTERLIFE; i++)
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
		dimension = Point<int16_t>(std::max<int16_t>(1366, VWIDTH),
			v83_layout ? V83_BAR_H : 84);

		// Right-anchored v83 elements are positioned against the logical view
		// width, so re-derive them whenever that changes. An in-game
		// resolution change (Settings-driven on map entry, or the options
		// menu) otherwise leaves every right-edge button where the previous
		// viewport put it.
		if (v83_layout && v83_laid_out_width != VWIDTH)
			layout_v83();

		UIElement::update();

		// Age out the boss HP gauge if the boss stopped reporting (dead / left).
		if (boss_hp_ticks > 0)
		{
			if (--boss_hp_ticks <= 0)
				boss_gage.clear();
		}

		// Age notification entries; auto-decline anything older than
		// NotificationCenter::TTL_TICKS (~2 minutes). When the queue
		// drains we drop the badge but keep the bell button visible.
		NotificationCenter::get().tick();
		if (NotificationCenter::get().empty())
			has_notification = false;

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
		buttons[BT_MENU_MSN]       ->set_active(menu_live);
		buttons[BT_MENU_MONSTERBATTLE]->set_active(menu_live);
		buttons[BT_MENU_MONSTERLIFE]->set_active(menu_live);

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

		if (v83_layout)
			// The v83 bar has no level plate, so the level rides along with
			// the job on the second text row.
			joblabel.change_text("Lv." + std::to_string(stats.get_stat(MapleStat::Id::LEVEL))
				+ "  " + stats.get_jobname());
		else
			joblabel.change_text(stats.get_jobname());

		// Update animations
		ani_hp_gauge.update();
		ani_mp_gauge.update();
		ap_notify.update();
		sp_notify.update();
		noncombat_notify.update();
		alarm_anim.update();
		v83_hp_flash.update();
		v83_mp_flash.update();
		v83_stat_ani.update();
		v83_skill_ani.update();

		// Pulse counter for notice sprite (only advances when active)
		if (has_notification)
			notice_pulse_tick++;

		// Forcing NORMAL here re-armed the NORMAL->MOUSEOVER transition every frame,
		// which replayed the hover sound continuously.
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
			if (auto alarm = UI::get().get_element<UIAlarmInvite>())
				if (alarm->is_active())
					alarm->stash();

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
			return Button::State::NORMAL;

		case BT_CHATCLOSE:
			chat_open = false;
			buttons[BT_CHATCLOSE]->set_active(false);
			buttons[BT_CHATOPEN]->set_active(true);
			buttons[BT_SCROLLUP]->set_active(false);
			buttons[BT_SCROLLDOWN]->set_active(false);
			return Button::State::NORMAL;

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

		case BT_MENU_MSN:
			UI::get().emplace<UIMessenger>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_MONSTERBATTLE:
			UI::get().emplace<UIMonsterBattle>();
			remove_menus();
			return Button::State::NORMAL;

		case BT_MENU_MONSTERLIFE:
			UI::get().emplace<UIMonsterLife>();
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
		int16_t bar_height = v83_layout ? V83_BAR_H : 84;

		Rectangle<int16_t> bounds(
			Point<int16_t>(0, position.y() - bar_height - extra_height),
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
		// get_stat returns uint16_t; store in int32_t so HP > 32767 doesn't
		// overflow to a negative int16_t (which made the bar go invisible).
		int32_t hp = stats.get_stat(MapleStat::Id::HP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);

		if (maxhp <= 0)
			return 0.0f;
		return std::clamp(static_cast<float>(hp) / static_cast<float>(maxhp), 0.0f, 1.0f);
	}

	float UIStatusBar::getmppercent() const
	{
		int32_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);

		if (maxmp <= 0)
			return 0.0f;
		return std::clamp(static_cast<float>(mp) / static_cast<float>(maxmp), 0.0f, 1.0f);
	}

	void UIStatusBar::notify()
	{
		// The bell button itself is always active (clickable so the
		// user can open an empty drawer at any time). Only the notice
		// badge sprite above the button toggles with has_notification.
		has_notification = true;
	}

	void UIStatusBar::clear_notification()
	{
		has_notification = false;
	}

	Point<int16_t> UIStatusBar::get_notice_anchor() const
	{
		auto it = buttons.find(BT_NOTICE);
		if (it == buttons.end() || !it->second)
		{
			// Fallback: roughly where the bell sits at 800x600.
			int16_t vw = Constants::Constants::get().get_viewwidth();
			int16_t vh = Constants::Constants::get().get_viewheight();
			return Point<int16_t>(vw - 8, vh - 36);
		}
		return position + it->second->bounds(Point<int16_t>(0, 0)).get_left_top();
	}

	// === Quickslot drop / render ===

	void UIStatusBar::draw_quickslot_cells() const
	{
		const auto& maplekeys = UI::get().get_keyboard().get_maplekeys();
		const auto& quickslot_keys = UI::get().get_keyboard().get_quickslot_keys();

		// Cube metrics and the nudges that centre art inside them differ per
		// panel: the StatusBar2 frame has 28px cubes, the v83 one 32x30.
		const int16_t cell_w = v83_layout ? QS83_CELL_W : QS_CELL_W;
		const int16_t cell_h = v83_layout ? QS83_CELL_H : QS_CELL_H;
		const int16_t icon_nudge = v83_layout ? 0 : QS_ICON_NUDGE_Y;
		const int16_t label_nudge = v83_layout ? 0 : QS_LABEL_NUDGE_Y;

		for (int16_t i = 0; i < static_cast<int16_t>(quickslot_keys.size()); ++i)
		{
			int32_t keycode = quickslot_keys[i];
			Point<int16_t> tl = quickslot_slot_pos(i);

			auto it = maplekeys.find(keycode);
			bool bound = it != maplekeys.end()
				&& it->second.type != KeyType::Id::NONE && it->second.action != 0;

			if (bound)
			{
				Texture icon = get_quickslot_icon(it->second.type, it->second.action);
				if (icon.is_valid())
				{
					// Fit the (32px) icon into the cube and centre it — its
					// origin is already normalised to top-left by get_quickslot_icon.
					Point<int16_t> dims = icon.get_dimensions();
					float scale = 1.0f;
					int16_t maxdim = std::max(dims.x(), dims.y());
					int16_t fit = std::min(cell_w, cell_h);
					if (maxdim > fit)
						scale = static_cast<float>(fit) / static_cast<float>(maxdim);

					int16_t dw = static_cast<int16_t>(dims.x() * scale);
					int16_t dh = static_cast<int16_t>(dims.y() * scale);
					Point<int16_t> pos(
						static_cast<int16_t>(tl.x() + (cell_w - dw) / 2),
						static_cast<int16_t>(tl.y() + (cell_h - dh) / 2 + icon_nudge));
					icon.draw(DrawArgument(pos, scale, scale, 1.0f));
				}
			}

			// Key label for EVERY cube (even empty ones), so the player always
			// knows which key fires each slot. Drawn from the real binding, so it
			// stays correct after rebinding and on the blank panel. Bottom-left
			// corner keeps it clear of the centred icon.
			static const uint8_t defcodes[8] = { 42, 82, 71, 73, 29, 83, 79, 81 };
			bool default_key = i < 8 && (static_cast<uint8_t>(keycode) == defcodes[i]
				|| (i == 0 && keycode == 54) || (i == 4 && keycode == 157));

			if (default_key && qs_key_sprites[i].is_valid())
			{
				qs_key_sprites[i].draw(tl + Point<int16_t>(1, cell_h - 12 + label_nudge));
			}
			else
			{
				qs_key_label.change_text(qs_keyname(static_cast<uint8_t>(keycode)));

				ColorBox label_bg(qs_key_label.width() + 4, 12, Color::Name::BLACK, 0.65f);
				label_bg.draw(DrawArgument(tl + Point<int16_t>(0, cell_h - 11 + label_nudge)));

				qs_key_label.draw(tl + Point<int16_t>(1, cell_h - 13 + label_nudge));
			}
		}
	}

	Point<int16_t> UIStatusBar::quickslot_panel_pos() const
	{
		if (v83_layout)
			return position + v83_quickslot_pos;

		return position + Point<int16_t>(QS_PANEL_OFFSET_X, QS_PANEL_OFFSET_Y - QS_LIFT);
	}

	Point<int16_t> UIStatusBar::quickslot_slot_pos(int16_t slot) const
	{
		int16_t col = slot % 4;
		int16_t row = slot / 4;

		if (v83_layout)
			return quickslot_panel_pos() + Point<int16_t>(QS83_CELL_OFFSET_X + col * QS83_COL_STEP,
			                                              QS83_CELL_OFFSET_Y + row * QS83_ROW_STEP);

		return quickslot_panel_pos() + Point<int16_t>(QS_CELL_OFFSET_X + col * QS_COL_STEP,
		                                               QS_CELL_OFFSET_Y + row * QS_ROW_STEP);
	}

	int16_t UIStatusBar::quickslot_slot_at(Point<int16_t> cursorpos) const
	{
		if (!show_quickslot)
			return -1;

		Point<int16_t> cell = v83_layout
			? Point<int16_t>(QS83_CELL_W, QS83_CELL_H)
			: Point<int16_t>(QS_CELL_W, QS_CELL_H);

		for (int16_t i = 0; i < 8; i++)
		{
			Point<int16_t> tl = quickslot_slot_pos(i);
			Rectangle<int16_t> rect(tl, tl + cell);
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

		uint8_t key = UI::get().get_keyboard().get_quickslot_keys()[slot];

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
				uint8_t key = UI::get().get_keyboard().get_quickslot_keys()[slot];
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
