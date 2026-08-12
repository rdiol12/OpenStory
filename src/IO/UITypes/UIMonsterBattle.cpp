//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
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
#include "UIMonsterBattle.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Net/Handlers/MonsterHandlers.h"
#include "../../Net/Packets/MonsterPackets.h"

#include <algorithm>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMonsterBattle::UIMonsterBattle() : UIDragElement<PosMONSTERBATTLE>(Point<int16_t>(300, 20))
	{
		nl::node src = nl::nx::ui["UIWindowBT.img"]["MonsterBattleCollection"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg = Texture(backgrnd).get_dimensions();
		sprites.emplace_back(backgrnd);

		slot_empty = src["newSlot"];
		slot_select = src["select"];
		slot_selected = src["selectedSlot"];
		mark = src["bamonMark"];
		name_plate = src["bamonName"];
		tier_plate = src["tierTitle"];

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"],
			Point<int16_t>(bg.x() - 20 - 13, 8));
		buttons[BT_PREV] = std::make_unique<MapleButton>(src["arrowLeft"],
			Point<int16_t>(GRID_X - 4, GRID_Y + ROWS * STEP_Y + 8));
		buttons[BT_NEXT] = std::make_unique<MapleButton>(src["arrowRight"],
			Point<int16_t>(GRID_X + COLS * STEP_X - 22, GRID_Y + ROWS * STEP_Y + 8));
		buttons[BT_SETTEAM] = std::make_unique<MapleButton>(src["BtSetBamon"],
			Point<int16_t>(96, 352));

		page_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DARKGREY);
		detail_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DARKGREY);
		team_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::DARKGREY);

		dimension = bg;
		dragarea = Point<int16_t>(bg.x(), 20);

		MonsterBattleOpenPacket().dispatch();
	}

	void UIMonsterBattle::refresh()
	{
		const auto& units = MonsterBattleState::get().get_units();
		last_unit_count = units.size();

		num_pages = static_cast<int16_t>((units.size() + PER_PAGE - 1) / PER_PAGE);

		if (num_pages < 1)
			num_pages = 1;

		if (cur_page >= num_pages)
			cur_page = num_pages - 1;

		page_label.change_text(std::to_string(cur_page + 1) + " / " + std::to_string(num_pages));
		team_label.change_text("Team " + std::to_string(MonsterBattleState::get().get_team().size())
			+ " / " + std::to_string(TEAM_MAX));
	}

	void UIMonsterBattle::update()
	{
		auto& state = MonsterBattleState::get();

		if (state.get_units().size() != last_unit_count)
			refresh();

		if (state.get_error() != last_error)
		{
			last_error = state.get_error();

			switch (last_error)
			{
			case 1: detail_label.change_text("You do not own that unit."); break;
			case 2: detail_label.change_text("Team is full."); break;
			case 3: detail_label.change_text("That unit is already in the team."); break;
			default: break;
			}
		}

		UIElement::update();
	}

	Point<int16_t> UIMonsterBattle::slot_pos(int16_t index) const
	{
		int16_t col = index % COLS;
		int16_t row = index / COLS;

		return Point<int16_t>(GRID_X + col * STEP_X, GRID_Y + row * STEP_Y);
	}

	Texture UIMonsterBattle::card_texture(int32_t cardid) const
	{
		auto it = card_cache.find(cardid);

		if (it != card_cache.end())
			return it->second;

		nl::node n = nl::nx::ui["FamiliarCard.img"][std::to_string(cardid)]["normal"]["0"];
		Texture tx = n ? Texture(n) : Texture();
		card_cache[cardid] = tx;

		return tx;
	}

	bool UIMonsterBattle::in_team(int32_t cardid) const
	{
		const auto& team = MonsterBattleState::get().get_team();

		return std::find(team.begin(), team.end(), cardid) != team.end();
	}

	void UIMonsterBattle::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		const auto& units = MonsterBattleState::get().get_units();
		int16_t base = cur_page * PER_PAGE;

		for (int16_t i = 0; i < PER_PAGE; i++)
		{
			Point<int16_t> at = position + slot_pos(i);
			int16_t idx = base + i;
			bool has = idx < static_cast<int16_t>(units.size());

			if (has && in_team(units[idx].cardid))
				slot_selected.draw(DrawArgument(at));
			else if (i == hovered || i == selected)
				slot_select.draw(DrawArgument(at));
			else
				slot_empty.draw(DrawArgument(at));

			if (!has)
				continue;

			Texture card = card_texture(units[idx].cardid);

			if (card.is_valid())
			{
				Point<int16_t> d = card.get_dimensions();
				card.draw(DrawArgument(at + Point<int16_t>((SLOT_W - d.x()) / 2,
					(SLOT_H - d.y()) / 2)));
			}
		}

		page_label.draw(position + Point<int16_t>(GRID_X + COLS * STEP_X / 2 - 2,
			GRID_Y + ROWS * STEP_Y + 8));

		int16_t sel = base + selected;

		if (selected >= 0 && sel < static_cast<int16_t>(units.size()))
		{
			mark.draw(DrawArgument(position + Point<int16_t>(118, 96)));

			Texture card = card_texture(units[sel].cardid);

			if (card.is_valid())
			{
				Point<int16_t> d = card.get_dimensions();
				card.draw(DrawArgument(position + Point<int16_t>(152 - d.x(), 300 - d.y() * 2),
					2.0f, 2.0f));
			}

			name_plate.draw(DrawArgument(position + Point<int16_t>(78, 308)));
			tier_plate.draw(DrawArgument(position + Point<int16_t>(78, 330)));
		}

		team_label.draw(position + Point<int16_t>(152, 330));
		detail_label.draw(position + Point<int16_t>(152, 60));

		UIElement::draw_buttons(inter);
	}

	int16_t UIMonsterBattle::slot_at(Point<int16_t> cursorpos) const
	{
		Point<int16_t> rel = cursorpos - position;

		for (int16_t i = 0; i < PER_PAGE; i++)
		{
			Point<int16_t> tl = slot_pos(i);
			Rectangle<int16_t> r(tl, tl + Point<int16_t>(SLOT_W, SLOT_H));

			if (r.contains(rel))
				return i;
		}

		return -1;
	}

	Cursor::State UIMonsterBattle::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
		{
			hovered = -1;
			return dstate;
		}

		hovered = slot_at(cursorpos);

		if (hovered >= 0)
		{
			const auto& units = MonsterBattleState::get().get_units();
			int16_t idx = cur_page * PER_PAGE + hovered;

			if (idx < static_cast<int16_t>(units.size()))
			{
				if (clicked)
					selected = hovered;

				return Cursor::State::CANCLICK;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIMonsterBattle::set_page(int16_t page)
	{
		if (page < 0 || page >= num_pages)
			return;

		cur_page = page;
		selected = -1;
		refresh();
	}

	void UIMonsterBattle::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Button::State UIMonsterBattle::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			deactivate();
			return Button::State::NORMAL;

		case BT_PREV:
			set_page(cur_page - 1);
			return Button::State::NORMAL;

		case BT_NEXT:
			set_page(cur_page + 1);
			return Button::State::NORMAL;

		case BT_SETTEAM:
		{
			const auto& units = MonsterBattleState::get().get_units();
			int16_t idx = cur_page * PER_PAGE + selected;

			if (selected < 0 || idx >= static_cast<int16_t>(units.size()))
				return Button::State::NORMAL;

			int32_t cardid = units[idx].cardid;
			std::vector<int32_t> team = MonsterBattleState::get().get_team();
			auto it = std::find(team.begin(), team.end(), cardid);

			if (it != team.end())
				team.erase(it);
			else if (team.size() < TEAM_MAX)
				team.push_back(cardid);

			MonsterBattleSetTeamPacket(team).dispatch();
			return Button::State::NORMAL;
		}
		}

		return Button::State::NORMAL;
	}

	UIElement::Type UIMonsterBattle::get_type() const
	{
		return TYPE;
	}
}
