#include "UIPartyInvite.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Net/Packets/GameplayPackets.h"
#include "../../Gameplay/Stage.h"
#include "../../Character/BuddyList.h"
#include "../../Character/OtherChar.h"
#include "../../Character/Job.h"
#include "../../Gameplay/MapleMap/MapChars.h"

#include <algorithm>
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
		constexpr int16_t ROW_GAP     = 2;           // visual gap between INVITE buttons
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

		// Per-row INVITE button — three states stamped manually since
		// the same sprite is reused for every visible row. NX origin
		// is already (0,0) for BtInvite, so we can just stamp it at
		// any row position.
		row_invite_tex         = Texture(src["BtInvite"]["normal"]["0"]);
		row_invite_hover_tex   = Texture(src["BtInvite"]["mouseOver"]["0"]);
		row_invite_pressed_tex = Texture(src["BtInvite"]["pressed"]["0"]);

		// Title-bar X close button. PopupInvite ships no close glyph
		// in NX, so reuse the standard Basic.img/BtClose3 (the same
		// sprite UIUserList itself uses).
		buttons[BT_CLOSE]       = std::make_unique<MapleButton>(
			nl::nx::ui["Basic.img"]["BtClose3"], Point<int16_t>(244, 7));
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

		// Nudge the typing-caret marker right + up so it sits inside
		// the input strip rather than flush against the lower-left.
		name_field.draw(position, Point<int16_t>(4, -3));

		// Refill the recommended list every draw so newly-online buddies
		// appear without needing an explicit refresh button.
		rebuild_rows();

		// Plain-text row labels. NAME from buddy list / map players,
		// JOB + LEVEL only known for map players (buddy packets don't
		// carry that info).
		Text row_text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::BLACK, "", 0);

		int16_t visible = std::min<int16_t>(static_cast<int16_t>(rows.size()), MAX_VISIBLE);
		for (int16_t r = 0; r < visible; ++r)
		{
			int16_t row_y = LIST_Y + r * ROW_STRIDE;
			int16_t idx   = r + list_scroll;
			if (idx >= static_cast<int16_t>(rows.size())) break;

			row_text.change_text(rows[idx].name);
			row_text.draw(position + Point<int16_t>(COL_NAME_X, row_y - 1));

			if (!rows[idx].job.empty())
			{
				row_text.change_text(rows[idx].job);
				row_text.draw(position + Point<int16_t>(COL_JOB_X, row_y - 1));
			}

			if (rows[idx].level > 0)
			{
				row_text.change_text(std::to_string(rows[idx].level));
				row_text.draw(position + Point<int16_t>(COL_LEVEL_X, row_y - 1));
			}

			// Pick the matching state sprite based on hover index.
			const Texture& bt = (hovered_row == idx)
				? row_invite_hover_tex
				: row_invite_tex;
			bt.draw(position + Point<int16_t>(COL_INV_X, row_y));
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
		// stand-in for inviteable acquaintances. Buddy packets carry
		// no job/level info, so those columns stay blank for buddy rows.
		const auto& entries =
			Stage::get().get_player().get_buddylist().get_entries();
		for (const auto& kv : entries)
		{
			const BuddyEntry& e = kv.second;
			if (!e.online()) continue;
			if (seen.count(e.name)) continue;
			InviteRow row;
			row.name = e.name;
			rows.push_back(row);
			seen.insert(e.name);
		}

		// Source 2 — other characters currently on the same map. We
		// do have job + level for these via OtherChar, so populate
		// those columns from there.
		MapObjects* map_chars = Stage::get().get_chars().get_chars();
		if (map_chars)
		{
			for (auto& kv : *map_chars)
			{
				if (auto* oc = dynamic_cast<OtherChar*>(kv.second.get()))
				{
					std::string nm = oc->get_name();
					if (nm.empty() || seen.count(nm)) continue;
					InviteRow row;
					row.name  = nm;
					row.level = static_cast<int16_t>(oc->get_level());
					row.job   = Job(oc->get_job()).get_name();
					rows.push_back(row);
					seen.insert(nm);
				}
			}
		}


		// Sort: below-or-equal-my-level first (ascending), then above
		// (ascending). Unknown-level rows (buddies, level=0) sort to
		// the very bottom so the player can scan inviteable peers from
		// closest match upward.
		int16_t my_level = static_cast<int16_t>(
			Stage::get().get_player().get_level());
		std::sort(rows.begin(), rows.end(),
			[my_level](const InviteRow& a, const InviteRow& b)
			{
				bool a_unknown = a.level <= 0;
				bool b_unknown = b.level <= 0;
				if (a_unknown != b_unknown)
					return !a_unknown;        // knowns before unknowns
				if (a_unknown && b_unknown)
					return a.name < b.name;   // alpha within unknowns

				bool a_above = a.level > my_level;
				bool b_above = b.level > my_level;
				if (a_above != b_above)
					return !a_above;          // below-or-equal first
				return a.level < b.level;     // ascending within group
			});

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
		// Track hovered row so the draw pass can swap to the
		// mouseOver sprite.
		hovered_row = -1;
		for (size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].hit.contains(cursorpos))
			{
				hovered_row = static_cast<int16_t>(i);
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
		case BT_CLOSE:
			deactivate();
			break;
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
