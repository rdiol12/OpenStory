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

#include "UIEquipInventory.h"
#include "UIItemInventory.h"
#include "UIKeyConfig.h"
#include "UIQuestLog.h"
#include "UINotice.h"
#include "UIOptionMenu.h"
#include "UIQuit.h"
#include "UISkillBook.h"
#include "UIStatsInfo.h"

#include "../../Net/OutPacket.h"
#include "../../Net/Session.h"
#include "../../IO/Window.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"

#include <iostream>

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

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];

		sprites.emplace_back(mainbar["backgrnd"]);
		sprites.emplace_back(mainbar["gaugeBackgrd"]);
		sprites.emplace_back(mainbar["notice"]);
		sprites.emplace_back(mainbar["lvBacktrnd"]);
		sprites.emplace_back(mainbar["lvCover"]);

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

		statset = Charset(mainbar["gauge"]["number"], Charset::Alignment::RIGHT);
		levelset = Charset(mainbar["lvNumber"], Charset::Alignment::LEFT);

		joblabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::YELLOW);
		namelabel = Text(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::WHITE);

		buttons[BT_WHISPER]   = std::make_unique<MapleButton>(mainbar["BtChat"]);
		buttons[BT_CALLGM]    = std::make_unique<MapleButton>(mainbar["BtClaim"]);

		buttons[BT_CASHSHOP]  = std::make_unique<MapleButton>(mainbar["BtCashShop"], Point<int16_t>(0, 0));
		buttons[BT_TRADE]     = std::make_unique<MapleButton>(mainbar["BtMTS"],      Point<int16_t>(18, 0));
		buttons[BT_MENU]      = std::make_unique<MapleButton>(mainbar["BtMenu"],     Point<int16_t>(55, 0));
		buttons[BT_OPTIONS]   = std::make_unique<MapleButton>(mainbar["BtSystem"],   Point<int16_t>(54, 0));

		buttons[BT_CHARACTER] = std::make_unique<MapleButton>(mainbar["BtCharacter"]);
		buttons[BT_STATS]     = std::make_unique<MapleButton>(mainbar["BtStat"]);
		buttons[BT_QUEST]     = std::make_unique<MapleButton>(mainbar["BtQuest"]);
		buttons[BT_INVENTORY] = std::make_unique<MapleButton>(mainbar["BtInven"]);
		buttons[BT_EQUIPS]    = std::make_unique<MapleButton>(mainbar["BtEquip"]);
		buttons[BT_SKILL]     = std::make_unique<MapleButton>(mainbar["BtSkill"]);
	}

	void UIStatusBar::draw(float alpha) const
	{
		UIElement::draw(alpha);

		expbar.draw(position + Point<int16_t>(-261, -15));
		hpbar.draw(position + Point<int16_t>(-261, -31));
		mpbar.draw(position + Point<int16_t>(-90, -31));

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
	}

	Button::State UIStatusBar::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case BT_STATS:
			UI::get().emplace<UIStatsInfo>(
				Stage::get().get_player().get_stats()
			);
			return Button::State::NORMAL;
		case BT_INVENTORY:
			UI::get().emplace<UIItemInventory>(
				Stage::get().get_player().get_inventory()
			);
			return Button::State::NORMAL;
		case BT_EQUIPS:
			UI::get().emplace<UIEquipInventory>(
				Stage::get().get_player().get_inventory()
			);
			return Button::State::NORMAL;
		case BT_SKILL:
			UI::get().emplace<UISkillBook>(
				Stage::get().get_player().get_stats(),
				Stage::get().get_player().get_skills()
			);
			return Button::State::NORMAL;
		case BT_QUEST:
			UI::get().emplace<UIQuestLog>(
				Stage::get().get_player().get_quests()
			);
			return Button::State::NORMAL;
		case BT_CASHSHOP:
			OutPacket(OutPacket::Opcode::ENTER_CASHSHOP).dispatch();
			return Button::State::NORMAL;
		case BT_TRADE:
			return Button::State::NORMAL;
		case BT_MENU:
			UI::get().emplace<UIKeyConfig>(
				Stage::get().get_player().get_inventory(),
				Stage::get().get_player().get_skills()
			);
			return Button::State::NORMAL;
		case BT_OPTIONS:
			UI::get().emplace<UIOptionMenu>();
			return Button::State::NORMAL;
		default:
			return Button::State::NORMAL;
		}
	}

	bool UIStatusBar::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds(
			position - Point<int16_t>(512, 84),
			position - Point<int16_t>(512, 84) + dimension
		);

		return bounds.contains(cursorpos);
	}

	UIElement::Type UIStatusBar::get_type() const
	{
		return TYPE;
	}

	void UIStatusBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
	}

	void UIStatusBar::toggle_qs()
	{
	}

	void UIStatusBar::toggle_menu()
	{
	}

	void UIStatusBar::remove_menus()
	{
	}

	bool UIStatusBar::is_menu_active()
	{
		return false;
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
