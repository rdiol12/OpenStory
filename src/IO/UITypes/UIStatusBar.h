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

#include "../Components/Charset.h"
#include "../Components/Gauge.h"

#include "../../Character/CharStats.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Animation.h"
#include "../../Graphics/Geometry.h"

namespace ms
{
	class UIStatusBar : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::STATUSBAR;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIStatusBar(const CharStats& stats);

		void draw(float alpha) const override;
		void update() override;

		bool is_in_range(Point<int16_t> cursorpos) const override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void toggle_qs();
		void toggle_menu();
		void remove_menus();
		bool is_menu_active();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		float getexppercent() const;
		float gethppercent() const;
		float getmppercent() const;

		enum Buttons : uint16_t
		{
			// Main bar buttons
			BT_WHISPER,
			BT_CALLGM,
			BT_CASHSHOP,
			BT_TRADE,
			BT_MENU,
			BT_OPTIONS,
			BT_CHARACTER,
			BT_STATS,
			BT_QUEST,
			BT_INVENTORY,
			BT_EQUIPS,
			BT_SKILL,
			BT_CHANNEL,
			BT_KEYSETTING,
			BT_NOTICE,
			BT_FARM,
			BT_EXITDUNGEON,
			BF_BT_CASHSHOP,
			BT_CHATCLOSE,
			BT_CHATOPEN,
			BT_SCROLLUP,
			BT_SCROLLDOWN,
			// Quick slot
			BT_QS_OPEN,
			BT_QS_CLOSE,
			// Menu sub-panel
			BT_MENU_STAT,
			BT_MENU_SKILL,
			BT_MENU_QUEST,
			BT_MENU_ITEM,
			BT_MENU_EQUIP,
			BT_MENU_COMMUNITY,
			BT_MENU_EVENT,
			BT_MENU_RANK,
			BT_MENU_EPISODBOOK,
			BT_MENU_MONSTERBATTLE,
			BT_MENU_MONSTERLIFE,
			BT_MENU_MSN,
			BT_MENU_AFREECATV,
			// System sub-panel
			BT_SYS_CHANNEL,
			BT_SYS_GAMEOPTION,
			BT_SYS_GAMEQUIT,
			BT_SYS_JOYPAD,
			BT_SYS_KEYSETTING,
			BT_SYS_MONSTERLIFE,
			BT_SYS_OPTION,
			BT_SYS_ROOMCHANGE,
			BT_SYS_SYSTEMOPTION,
			// Chat tabs
			BT_TAP_ALL,
			BT_TAP_PARTY,
			BT_TAP_GUILD,
			BT_TAP_FRIEND,
			BT_TAP_EXPEDITION,
			BT_TAP_ASSOCIATION,
			BT_TAP_AFREECATV
		};

		const CharStats& stats;

		// Main gauges
		Gauge expbar;
		Gauge hpbar;
		Gauge mpbar;

		// Extra gauges
		Gauge barrier_bar;
		Gauge df_bar;
		Gauge relax_exp_bar;
		Gauge tf_bar;

		Charset statset;
		Charset levelset;
		Text namelabel;
		Text joblabel;

		// Main bar background (drawn stretched to screen width)
		Texture bar_backgrnd;

		// Gauge cover (overlay on top of gauges)
		Texture gauge_cover;

		// Class-variant gauge backgrounds
		Texture gauge_backgrd_ab;
		Texture gauge_backgrd_demon;
		Texture gauge_backgrd_kanna;
		Texture gauge_backgrd_zero;
		Texture gauge_cover_ab;
		Texture lv_backtrnd_sao;

		// Gauge animations (low HP/MP flash)
		Animation ani_hp_gauge;
		Animation ani_hp_gauge_ab;
		Animation ani_mp_gauge;

		// Notification animations
		Animation ap_notify;
		Animation sp_notify;
		Animation noncombat_notify;
		Texture cooltime_return;

		// Chat area
		Texture chat_cover;
		Texture chat_enter;
		Texture chat_space;
		Texture chat_space2;
		bool chat_open;

		// Chat target
		Texture chat_target_base;
		Texture chat_target_all;
		Texture chat_target_party;
		Texture chat_target_guild;
		Texture chat_target_friend;
		Texture chat_target_expedition;
		Texture chat_target_association;
		Texture chat_target_afctv;
		uint8_t chat_target_id;

		// Chat tab bar
		Texture tap_bar;
		Texture tap_bar_over;
		Texture chat_scroll_normal;
		Texture chat_scroll_over;

		// Menu sub-panel
		Texture menu_backgrnd;
		bool show_menu;

		// System sub-panel
		Texture system_backgrnd;
		bool show_system;

		// Quick slot
		Texture quickslot_bg;
		bool show_quickslot;

		// readyZero
		Texture ready_zero_backgrnd;
		Texture ready_zero_gauge_backgrnd;

		// Buff tray background (from StatusBar3.img)
		Texture buff_backgrnd;

		// Alarm/notification area
		Texture alarm_backgrnd;
		Animation alarm_anim;

		// Event shortcut icons
		Texture event_backgrnd;

		// Pre-allocated draw objects
		ColorBox menu_bg;
		ColorBox sys_bg;
	};
}
