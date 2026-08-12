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
#include "UIMonsterLife.h"

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
	UIMonsterLife::UIMonsterLife() : UIDragElement<PosMONSTERLIFE>(Point<int16_t>(220, 20))
	{
		nl::node src = nl::nx::ui["FarmUI.img"]["myHome"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg = Texture(backgrnd).get_dimensions();
		sprites.emplace_back(backgrnd);

		cover = src["cover"];
		placeholder = src["default"];

		buttons[BT_CLOSE] = std::make_unique<MapleButton>(src["btClose"],
			Point<int16_t>(bg.x() - 26, 6));
		buttons[BT_REMOVE] = std::make_unique<MapleButton>(src["btModify"],
			Point<int16_t>(bg.x() - 96, bg.y() - 30));

		title_label = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		status_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE);

		dimension = bg;
		dragarea = Point<int16_t>(bg.x(), 20);

		MonsterLifeEnterPacket(0).dispatch();
	}

	void UIMonsterLife::refresh()
	{
		auto& st = MonsterLifeState::get();
		last_entry_count = st.get_entries().size();
		loaded_once = st.is_loaded();

		title_label.change_text(st.get_owner_name().empty()
			? std::string("Farm")
			: st.get_owner_name() + "'s Farm   Lv." + std::to_string(st.get_level()));
	}

	void UIMonsterLife::update()
	{
		auto& st = MonsterLifeState::get();

		if (st.get_entries().size() != last_entry_count || st.is_loaded() != loaded_once)
			refresh();

		if (st.get_error() != last_error)
		{
			last_error = st.get_error();

			switch (last_error)
			{
			case 1: status_label.change_text("Farm is full."); break;
			case 2: status_label.change_text("You do not own that monster."); break;
			case 3: status_label.change_text("You cannot edit this farm."); break;
			case 4: status_label.change_text("Invalid position."); break;
			case 5: status_label.change_text("That monster is no longer there."); break;
			default: break;
			}
		}

		UIElement::update();
	}

	Texture UIMonsterLife::card_texture(int32_t cardid) const
	{
		auto it = card_cache.find(cardid);

		if (it != card_cache.end())
			return it->second;

		nl::node n = nl::nx::ui["FamiliarCard.img"][std::to_string(cardid)]["normal"]["0"];
		Texture tx = n ? Texture(n) : Texture();
		card_cache[cardid] = tx;

		return tx;
	}

	bool UIMonsterLife::surface_contains(Point<int16_t> rel) const
	{
		return rel.x() >= SURF_X && rel.x() < SURF_X + SURF_W
			&& rel.y() >= SURF_Y && rel.y() < SURF_Y + SURF_H;
	}

	int16_t UIMonsterLife::palette_at(Point<int16_t> cursorpos) const
	{
		Point<int16_t> rel = cursorpos - position;

		if (rel.y() < PAL_Y || rel.y() >= PAL_Y + CARD_H)
			return -1;

		int16_t idx = (rel.x() - PAL_X) / PAL_STEP;

		if (idx < 0 || idx >= PAL_MAX)
			return -1;

		if (rel.x() < PAL_X + idx * PAL_STEP || rel.x() >= PAL_X + idx * PAL_STEP + CARD_W)
			return -1;

		return idx;
	}

	int32_t UIMonsterLife::entry_at(Point<int16_t> cursorpos) const
	{
		Point<int16_t> rel = cursorpos - position;

		const auto& entries = MonsterLifeState::get().get_entries();

		for (auto it = entries.rbegin(); it != entries.rend(); ++it)
		{
			Point<int16_t> tl(SURF_X + it->x, SURF_Y + it->y);
			Rectangle<int16_t> r(tl, tl + Point<int16_t>(CARD_W, CARD_H));

			if (r.contains(rel))
				return it->slotid;
		}

		return 0;
	}

	void UIMonsterLife::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		title_label.draw(position + Point<int16_t>(16, 8));

		const auto& units = MonsterBattleState::get().get_units();

		for (int16_t i = 0; i < PAL_MAX && i < static_cast<int16_t>(units.size()); i++)
		{
			Point<int16_t> at = position + Point<int16_t>(PAL_X + i * PAL_STEP, PAL_Y);
			Texture card = card_texture(units[i].cardid);

			if (card.is_valid())
				card.draw(DrawArgument(at, units[i].cardid == sel_card ? 1.0f : 0.75f));
		}

		const auto& entries = MonsterLifeState::get().get_entries();

		for (const auto& e : entries)
		{
			Point<int16_t> at = position + Point<int16_t>(SURF_X + e.x, SURF_Y + e.y);
			Texture card = card_texture(e.cardid);

			if (card.is_valid())
				card.draw(DrawArgument(at));
			else if (placeholder.is_valid())
				placeholder.draw(DrawArgument(at));

			if (e.slotid == sel_slot && cover.is_valid())
				cover.draw(DrawArgument(at - Point<int16_t>((54 - CARD_W) / 2,
					(54 - CARD_H) / 2)));
		}

		status_label.draw(position + Point<int16_t>(16, SURF_Y + SURF_H + 4));

		UIElement::draw_buttons(inter);
	}

	Cursor::State UIMonsterLife::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> rel = cursorpos - position;

		if (drag_slot != 0)
		{
			if (!clicked)
			{
				if (surface_contains(rel))
				{
					int16_t nx = std::clamp<int16_t>(rel.x() - SURF_X, 0, SURF_W - CARD_W);
					int16_t ny = std::clamp<int16_t>(rel.y() - SURF_Y, 0, SURF_H - CARD_H);
					MonsterLifeMovePacket(drag_slot, nx, ny).dispatch();
				}

				drag_slot = 0;
			}

			return Cursor::State::GRABBING;
		}

		if (clicked)
		{
			int16_t pal = palette_at(cursorpos);

			if (pal >= 0)
			{
				const auto& units = MonsterBattleState::get().get_units();

				if (pal < static_cast<int16_t>(units.size()))
				{
					sel_card = units[pal].cardid;
					sel_slot = 0;
					status_label.change_text("Click the farm to place.");
				}

				return Cursor::State::CANCLICK;
			}

			int32_t hit = entry_at(cursorpos);

			if (hit != 0)
			{
				sel_slot = hit;
				drag_slot = hit;
				return Cursor::State::GRABBING;
			}

			if (surface_contains(rel) && sel_card != 0)
			{
				int16_t nx = std::clamp<int16_t>(rel.x() - SURF_X, 0, SURF_W - CARD_W);
				int16_t ny = std::clamp<int16_t>(rel.y() - SURF_Y, 0, SURF_H - CARD_H);
				MonsterLifePlacePacket(sel_card, nx, ny).dispatch();
				return Cursor::State::CANCLICK;
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIMonsterLife::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	Button::State UIMonsterLife::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			MonsterLifeLeavePacket().dispatch();
			deactivate();
			return Button::State::NORMAL;

		case BT_REMOVE:
			if (sel_slot != 0)
			{
				MonsterLifeRemovePacket(sel_slot).dispatch();
				sel_slot = 0;
			}

			return Button::State::NORMAL;
		}

		return Button::State::NORMAL;
	}

	UIElement::Type UIMonsterLife::get_type() const
	{
		return TYPE;
	}
}
