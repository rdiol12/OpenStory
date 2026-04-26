#include "UIPartyInvite.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Net/Packets/GameplayPackets.h"
#include "../../Gameplay/Stage.h"
#include "../../Character/BuddyList.h"
#include "../../Character/OtherChar.h"
#include "../../Gameplay/MapleMap/MapChars.h"

#include <set>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		// Layout pulled directly from the NX origins of PopupInvite:
		//   title         at (-10,-30)        →  top banner
		//   backgrnd3     at (-10,-60)        →  header band (78px tall)
		//   BtInviteName  at (-212,-85)       →  inline INVITE button
		//   table         at (-11,-152)       →  219x191 list panel
		//   BtName        at (-10,-133)       →  NAME column header
		//   BtJob         at (-88,-133)       →  JOB column header
		//   BtLevel       at (-164,-133)      →  LEVEL column header
		//   invite        at (-203,-133)      →  INVITE column label
		//
		// We use those baked origins for chrome, but the per-row INVITE
		// button needs to render at multiple y values, so it's loaded
		// as a plain Texture and origin-stripped before use.
		constexpr int16_t NAME_FIELD_X = 18;
		constexpr int16_t NAME_FIELD_R = 200;
		constexpr int16_t NAME_FIELD_Y = 86;
		constexpr int16_t NAME_FIELD_H = 16;

		// Table rows live below the column-header strip.
		constexpr int16_t LIST_X      = 11;
		constexpr int16_t LIST_Y      = 152;
		constexpr int16_t LIST_W      = 191;
		constexpr int16_t LIST_H      = 219;
		constexpr int16_t ROW_H       = 18;          // sprite height
		constexpr int16_t ROW_GAP     = 3;           // visual gap between INVITE buttons
		constexpr int16_t ROW_STRIDE  = ROW_H + ROW_GAP;
		constexpr int16_t MAX_VISIBLE = LIST_H / ROW_STRIDE;

		constexpr int16_t COL_NAME_X  = LIST_X + 4;
		constexpr int16_t COL_JOB_X   = LIST_X + 80;
		constexpr int16_t COL_LEVEL_X = LIST_X + 156;
		constexpr int16_t COL_INV_X   = LIST_X + 192;          // INVITE column right
	}

	UIPartyInvite::UIPartyInvite()
		: UIDragElement<PosPARTYINVITE>(Point<int16_t>(264, 22))
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["UserList"]["PopupInvite"];

		sprites.emplace_back(src["backgrnd"]);
		backgrnd2    = Texture(src["backgrnd2"]);
		backgrnd3    = Texture(src["backgrnd3"]);
		title        = Texture(src["title"]);
		table_bg     = Texture(src["table"]);
		invite_label = Texture(src["invite"]);

		// Per-row INVITE button — load the normal-state sprite. NX
		// origin is already (0,0) for BtInvite, so we can just stamp
		// it at any row position.
		row_invite_tex = Texture(src["BtInvite"]["normal"]["0"]);

		buttons[BT_INVITE_NAME] = std::make_unique<MapleButton>(src["BtInviteName"]);
		buttons[BT_COL_NAME]    = std::make_unique<MapleButton>(src["BtName"]);
		buttons[BT_COL_JOB]     = std::make_unique<MapleButton>(src["BtJob"]);
		buttons[BT_COL_LEVEL]   = std::make_unique<MapleButton>(src["BtLevel"]);

		name_field = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(NAME_FIELD_X, NAME_FIELD_Y),
				Point<int16_t>(NAME_FIELD_R, NAME_FIELD_Y + NAME_FIELD_H)),
			13);

		dimension = Point<int16_t>(264, 382);
		dragarea  = Point<int16_t>(264, 22);
	}

	void UIPartyInvite::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		backgrnd2.draw(position);
		backgrnd3.draw(position);
		title.draw(position);
		table_bg.draw(position);
		invite_label.draw(position);

		name_field.draw(position);

		// Refill the recommended list every draw so newly-online buddies
		// appear without needing an explicit refresh button.
		rebuild_rows();

		// Plain-text row labels — names from BuddyList. Job/Level stay
		// blank because the buddy packet doesn't carry that info.
		Text row_name(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::BLACK, "", 0);

		int16_t visible = std::min<int16_t>(static_cast<int16_t>(rows.size()), MAX_VISIBLE);
		for (int16_t r = 0; r < visible; ++r)
		{
			int16_t row_y = LIST_Y + r * ROW_STRIDE;
			int16_t idx   = r + list_scroll;
			if (idx >= static_cast<int16_t>(rows.size())) break;

			row_name.change_text(rows[idx].name);
			row_name.draw(position + Point<int16_t>(COL_NAME_X, row_y - 1));

			row_invite_tex.draw(position + Point<int16_t>(COL_INV_X, row_y));
		}

		UIElement::draw_buttons(inter);
	}

	void UIPartyInvite::update()
	{
		UIElement::update();
		name_field.update(position);
	}

	void UIPartyInvite::rebuild_rows() const
	{
		rows.clear();

		std::set<std::string> seen;       // dedupe by name
		std::string my_name = Stage::get().get_player().get_name();
		seen.insert(my_name);

		// Source 1 — online buddies. Cosmic doesn't ship a "recommended
		// roster" packet, so we use the buddy list as the obvious
		// stand-in for inviteable acquaintances.
		const auto& entries =
			Stage::get().get_player().get_buddylist().get_entries();
		for (const auto& kv : entries)
		{
			const BuddyEntry& e = kv.second;
			if (!e.online()) continue;
			if (seen.count(e.name)) continue;
			rows.push_back({ e.name, Rectangle<int16_t>() });
			seen.insert(e.name);
		}

		// Source 2 — other characters currently on the same map.
		MapObjects* map_chars = Stage::get().get_chars().get_chars();
		if (map_chars)
		{
			for (auto& kv : *map_chars)
			{
				if (auto* oc = dynamic_cast<OtherChar*>(kv.second.get()))
				{
					std::string nm = oc->get_name();
					if (nm.empty() || seen.count(nm)) continue;
					rows.push_back({ nm, Rectangle<int16_t>() });
					seen.insert(nm);
				}
			}
		}

		// Refresh per-row hit-rects so click routing tracks the
		// current scroll position.
		int16_t visible = std::min<int16_t>(static_cast<int16_t>(rows.size()), MAX_VISIBLE);
		for (int16_t r = 0; r < visible; ++r)
		{
			int16_t row_y = LIST_Y + r * ROW_STRIDE;
			int16_t idx   = r + list_scroll;
			if (idx >= static_cast<int16_t>(rows.size())) break;

			rows[idx].hit = Rectangle<int16_t>(
				position + Point<int16_t>(COL_INV_X, row_y),
				position + Point<int16_t>(COL_INV_X + 37, row_y + 17));
		}

		// Clamp scroll so the user can't push past the last row.
		int16_t max_scroll = std::max<int16_t>(0,
			static_cast<int16_t>(rows.size()) - MAX_VISIBLE);
		if (list_scroll > max_scroll) list_scroll = max_scroll;
		if (list_scroll < 0) list_scroll = 0;
	}

	Cursor::State UIPartyInvite::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Textfield::get_bounds returns SCREEN coords (bounds + parentpos),
		// so we forward cursorpos verbatim instead of localising it.
		Cursor::State tstate = name_field.send_cursor(cursorpos, clicked);
		if (tstate != Cursor::State::IDLE)
			return tstate;

		// Per-row INVITE hits — mapped to the buddy at that index.
		for (size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].hit.contains(cursorpos))
			{
				if (clicked)
					dispatch_invite(rows[i].name);
				return clicked ? Cursor::State::CLICKING : Cursor::State::CANCLICK;
			}
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIPartyInvite::send_scroll(double yoffset)
	{
		Rectangle<int16_t> list_rect(
			position + Point<int16_t>(LIST_X, LIST_Y),
			position + Point<int16_t>(LIST_X + LIST_W, LIST_Y + LIST_H));
		if (!list_rect.contains(UI::get().get_cursor_position())) return;

		int16_t step = yoffset > 0 ? -1 : 1;
		list_scroll += step;
		if (list_scroll < 0) list_scroll = 0;
		int16_t max_scroll = std::max<int16_t>(0,
			static_cast<int16_t>(rows.size()) - MAX_VISIBLE);
		if (list_scroll > max_scroll) list_scroll = max_scroll;
	}

	void UIPartyInvite::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
			return;
		if (escape)
		{
			deactivate();
			return;
		}
		if (keycode == 13 /* enter */)
		{
			std::string name = name_field.get_text();
			if (!name.empty())
				dispatch_invite(name);
		}
	}

	UIElement::Type UIPartyInvite::get_type() const
	{
		return TYPE;
	}

	Button::State UIPartyInvite::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_INVITE_NAME:
		{
			std::string name = name_field.get_text();
			if (!name.empty())
				dispatch_invite(name);
			break;
		}
		case BT_COL_NAME:
		case BT_COL_JOB:
		case BT_COL_LEVEL:
			// Sortable column headers — Cosmic doesn't supply ordering
			// metadata for buddy rows, so the click is currently a
			// no-op visual.
			break;
		}
		return Button::State::NORMAL;
	}

	void UIPartyInvite::dispatch_invite(const std::string& name)
	{
		if (name.empty()) return;
		InviteToPartyPacket(name).dispatch();
	}
}
