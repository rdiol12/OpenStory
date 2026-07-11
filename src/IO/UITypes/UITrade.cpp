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
// TODO: finish trade UI
#include "UITrade.h"

#include "UINotice.h"
#include "UIChatBar.h"
#include "UIItemInventory.h"
#include "UIEquipInventory.h"
#include "UIStatsInfo.h"
#include "UISkillBook.h"
#include "UIReport.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"
#include "../KeyAction.h"
#include "../../Graphics/Geometry.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Character/Look/CharLook.h"
#include "../../Character/Player.h"
#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UITrade::UITrade() : UIDragElement<PosTRADE>(Point<int16_t>(250, 20))
	{
		// Post-Big-Bang trade panel lives at UIWindow2.img/TradingRoom.
		// Uses a 3-layer backdrop (526x472 main + 514x459 inset +
		// 442x341 item grid frame). BtTrade/BtReset/BtCoin/BtEnter
		// each carry NX origins so MapleButton positions them itself.
		nl::node src = nl::nx::ui["UIWindow2.img"]["TradingRoom"];

		nl::node backgrnd = src["backgrnd"];
		if (backgrnd)
		{
			background = backgrnd;
			sprites.emplace_back(backgrnd);

			nl::node backgrnd2 = src["backgrnd2"];
			if (backgrnd2)
				sprites.emplace_back(backgrnd2);

			nl::node backgrnd3 = src["backgrnd3"];
			if (backgrnd3)
				sprites.emplace_back(backgrnd3);
		}

		// Confirm = BtTrade, cancel = BtReset (its NX origin places it
		// directly below BtTrade, which matches the classic layout).
		nl::node bt_ok = src["BtTrade"];
		nl::node bt_cancel = src["BtReset"];

		auto bg_dim = background.get_dimensions();

		if (bg_dim.x() == 0 || bg_dim.y() == 0)
		{
			// No NX assets — use a fixed size
			bg_dim = Point<int16_t>(400, 310);
		}

		if (bt_ok.size() > 0)
			buttons[BT_CONFIRM] = std::make_unique<MapleButton>(bt_ok);
		else
			buttons[BT_CONFIRM] = std::make_unique<AreaButton>(
				Point<int16_t>(bg_dim.x() / 2 - 80, bg_dim.y() - 35),
				Point<int16_t>(70, 24)
			);

		// Greyed out until the other player accepts and joins the room.
		buttons[BT_CONFIRM]->set_state(Button::State::DISABLED);

		if (bt_cancel.size() > 0)
			buttons[BT_CANCEL] = std::make_unique<MapleButton>(bt_cancel);
		else
			buttons[BT_CANCEL] = std::make_unique<AreaButton>(
				Point<int16_t>(bg_dim.x() / 2 + 10, bg_dim.y() - 35),
				Point<int16_t>(70, 24)
			);

		// "Enter" send-chat button next to the input box (BtEnter has
		// its own NX origin placing it near the chat area).
		nl::node bt_enter = src["BtEnter"];
		if (bt_enter.size() > 0)
			buttons[BT_ENTER] = std::make_unique<MapleButton>(bt_enter);

		// Report button — BtClame in the v83 NX is the "complaint"
		// button used to report the trade partner. Uses its NX origin
		// so it appears near the partner avatar.
		nl::node bt_report = src["BtClame"];
		if (bt_report.size() > 0)
			buttons[BT_REPORT] = std::make_unique<MapleButton>(bt_report);

		// Meso button — BtCoin (NX origin puts it by the meso field)
		// opens the "how many mesos" prompt. Falls back to an invisible
		// AreaButton over my meso display so mesos can always be set.
		nl::node bt_coin = src["BtCoin"];
		if (bt_coin.size() > 0)
			buttons[BT_COIN] = std::make_unique<MapleButton>(bt_coin);
		else
			buttons[BT_COIN] = std::make_unique<AreaButton>(
				Point<int16_t>(148, 247),  // my meso display, see draw()
				Point<int16_t>(120, 20)
			);

		// Labels
		std::string player_name = Stage::get().get_player().get_stats().get_name();
		my_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, player_name);
		partner_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "");
		// Meso labels — left-aligned so they start right at the yellow
		// debug block's left edge. Plain number; the NX panel already
		// paints "meso" next to the input, we don't need to repeat it.
		my_meso_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "0");
		partner_meso_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "0");
		status_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "Waiting for partner...");

		my_meso = 0;
		partner_meso = 0;
		my_confirmed = false;
		partner_confirmed = false;
		selected_slot = -1;

		// Pre-allocate draw objects
		confirmed_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::GREEN, "Confirmed");
		confirm_btn_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Confirm");
		cancel_btn_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Cancel");

		dimension = bg_dim;
		dragarea = Point<int16_t>(bg_dim.x(), 20);

		// Chat input textfield at the bottom of the right panel.
		// Use the 6-arg ctor so the blinking cursor marker gets an
		// explicit visible color (the 5-arg ctor reuses text_color
		// for the marker — black-on-dark is invisible).
		// Use a high-contrast red marker — MEDIUMBLUE was hard to spot
		// on the dark chat area.
		// Left-aligned: text grows left→right and the cursor follows
		// the end of what you've typed. Marker opacity = 0 silences
		// the Textfield's built-in 1px cursor so we only show our
		// fat manual one.
		chat_input = Textfield(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::MEDIUMBLUE,           // text color
			Color::Name::MEDIUMBLUE, 0.0f,     // marker invisible
			Rectangle<int16_t>(241, 361, 176, 188),
			80);
		chat_input_text = Text(Text::Font::A11M, Text::Alignment::LEFT,
			Color::Name::MEDIUMBLUE, "", 0, false);

		chat_input.set_enter_callback([this](std::string msg)
		{
			if (msg.empty())
				return;
			// Don't echo locally — the server broadcasts the line back to
			// us as "<CharName> : msg", so a local copy would duplicate it.
			TradeChatPacket(msg).dispatch();
			chat_input.change_text("");
		});

		active = true;

		// set_state(FOCUSED) internally calls UI::focus_textfield, so
		// this single call is enough to route typed keys here.
		chat_input.set_state(Textfield::State::FOCUSED);
	}

	void UITrade::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Side-by-side trade grids on the left panel (right panel = chat):
		//   GREEN 3x3 on the left  → my items
		//   RED   3x3 on the right → partner items
		constexpr int16_t slot_w = 36;
		constexpr int16_t slot_h = 36;
		constexpr int16_t cols = 3;
		// Partner (red) sits on the left, my grid (green) on the right.
		constexpr int16_t partner_grid_x = 14;
		constexpr int16_t my_grid_x      = partner_grid_x + cols * (slot_w + 2) + 20;
		constexpr int16_t grid_y = 150;

		// Character avatars (my side green grid, partner side red grid).
		// The partner avatar is the CharLook built from the VISIT/ROOM
		// packet's addCharLook block — nothing is drawn on the partner
		// side until they have actually joined.
		const int16_t char_y = 115;
		CharLook& my_look = Stage::get().get_player().get_look();
		my_look.draw(position + Point<int16_t>(my_grid_x + 75, char_y),
			false, Stance::Id::STAND1, Expression::Id::DEFAULT);
		if (has_partner_look)
			partner_look.draw(position + Point<int16_t>(partner_grid_x + 45, char_y),
				true, // flipped so the partner faces the player
				Stance::Id::STAND1, Expression::Id::DEFAULT);

		// Names just below each avatar.
		const int16_t name_y = char_y + 0;
		partner_name_label.draw(position + Point<int16_t>(partner_grid_x + 20, name_y));
		my_name_label.draw(position + Point<int16_t>(my_grid_x + 60, name_y));

		// Chat log — OLDEST at the top, each new line drawn BELOW the
		// previous one. Only starts scrolling (oldest lines hidden off
		// the top) once the log has more lines than the visible area
		// can hold, so existing text stays put until the box is full.
		constexpr int16_t chat_x      = 280;
		constexpr int16_t chat_top    = 5;
		constexpr int16_t chat_bottom = 330;  // stay inside visible backdrop
		constexpr int16_t line_h      = 12;   // tighter so more lines fit

		// Top-pinned chat: oldest line at chat_top, new lines stack
		// below. When the log overflows the visible area, chat_scroll
		// shifts the window so older lines can be scrolled back into
		// view. No line is ever deleted — everything is reachable.
		// Use ceil division + loose bottom clamp so the last row of
		// text can start right up against chat_bottom instead of being
		// dropped one row early. Overflow only kicks in when the NEXT
		// line truly wouldn't fit inside the visible area.
		int total     = static_cast<int>(chat_lines.size());
		int max_lines = (chat_bottom - chat_top + line_h - 1) / line_h;
		int overflow  = std::max(0, total - max_lines);
		int scroll    = std::min(std::max(0, chat_scroll), overflow);
		int start     = overflow - scroll;

		int16_t cy = chat_top;
		for (int i = start;
			i < total && cy < chat_bottom;
			++i)
		{
			chat_lines[i].draw(position + Point<int16_t>(chat_x, cy));
			cy += line_h;
		}

		// NOTE: Textfield::draw would render the text at its internal
		// anchor (bounds.top) which is 100px above where we draw the
		// fat cursor. Instead of calling its draw, we render the text
		// ourselves at the cursor position so text + cursor stay aligned.
		// (We still rely on Textfield for input handling — just not its
		// built-in text rendering.)

		// Fat visible cursor — pinned at the RIGHT edge of the bounds
		// because the textfield is right-aligned, so typed text ends
		// here and the cursor stays fixed while the text grows left.
		// NOTE: Textfield::get_bounds() returns ABSOLUTE screen coords
		// (it already adds the parentpos captured via update()), so we
		// draw at those coords directly — adding `position` again would
		// double the offset and the cursor would drift on drag.
		{
			Rectangle<int16_t> r = chat_input.get_bounds();
			// Base (left) anchor where text starts, cursor follows the
			// end of what's typed so it moves right as you type. When
			// the cursor would reach the right edge, shift the base
			// left so the text scrolls instead of overflowing.
			int16_t base_x   = r.get_left_top().x() + 40;
			int16_t right_x  = r.get_right_bottom().x() - 4;
			int16_t cursor_x = base_x + static_cast<int16_t>(
				chat_input.get_text().size() * 6);
			if (cursor_x > right_x)
			{
				int16_t shift = cursor_x - right_x;
				base_x   -= shift;
				cursor_x -= shift;
			}
			int16_t cursor_y = r.get_left_top().y() + 104;
			// Blink ~2Hz using frame-counter from the cursor pulse tick
			// already present on the Textfield via update. Use a cheap
			// time-based approximation with static clock.
			static uint16_t blink = 0;
			blink++;
			if ((blink / 30) % 2 == 0)
			{
				ColorBox cursor_bar(2, 11, Color::Name::MEDIUMBLUE, 1.0f);
				cursor_bar.draw(DrawArgument(Point<int16_t>(cursor_x, cursor_y)));
			}

			// Render the typed text left-aligned starting at base_x,
			// growing right. Cursor stays at the end of the text.
			chat_input_text.change_text(chat_input.get_text());
			chat_input_text.draw(Point<int16_t>(base_x, cursor_y - 7));
		}

		// Draw items in their slots (cubes are painted into the NX
		// backdrop so no explicit slot rectangle is needed).
		for (int i = 0; i < MAX_SLOTS; i++)
		{
			int16_t col = i % cols;
			int16_t row = i / cols;
			Point<int16_t> my_pos      = position + Point<int16_t>(my_grid_x      + col * (slot_w + 2), grid_y + row * (slot_h + 2));
			Point<int16_t> partner_pos = position + Point<int16_t>(partner_grid_x + col * (slot_w + 2), grid_y + row * (slot_h + 2));

			if (!my_items[i].empty() && my_items[i].icon.is_valid())
				my_items[i].icon.draw(DrawArgument(my_pos + Point<int16_t>(2, slot_h - 4)));
			if (!partner_items[i].empty() && partner_items[i].icon.is_valid())
				partner_items[i].icon.draw(DrawArgument(partner_pos + Point<int16_t>(2, slot_h - 4)));
		}

		// Meso labels sit below each player's grid.
		int16_t meso_y = grid_y + 3 * (slot_h + 2) - 7;
		my_meso_label.draw(position      + Point<int16_t>(my_grid_x      + 35, meso_y));
		partner_meso_label.draw(position + Point<int16_t>(partner_grid_x + 35, meso_y));

		// Confirmation markers under each player's meso line.
		int16_t conf_y = meso_y + 20;
		if (my_confirmed)
			confirmed_text.draw(position + Point<int16_t>(my_grid_x, conf_y));
		if (partner_confirmed)
			confirmed_text.draw(position + Point<int16_t>(partner_grid_x, conf_y));

		// "Waiting for partner..." is now a separate generic UIWaitNotice
		// modal (see confirm_trade()) — no inline status label here.

		// Draw button labels if using AreaButtons
		if (background.get_dimensions().x() == 0)
		{
			confirm_btn_text.draw(position + Point<int16_t>(dimension.x() / 2 - 45, dimension.y() - 30));
			cancel_btn_text.draw(position + Point<int16_t>(dimension.x() / 2 + 45, dimension.y() - 30));
		}
	}

	void UITrade::update()
	{
		UIElement::update();

		std::string mesostr;

		mesostr = std::to_string(my_meso);
		string_format::split_number(mesostr);
		my_meso_label.change_text(mesostr);

		mesostr = std::to_string(partner_meso);
		string_format::split_number(mesostr);
		partner_meso_label.change_text(mesostr);

		// Advance the chat input's blinking-cursor timer + remember our
		// on-screen origin so typed characters show in the right place.
		chat_input.update(position);

		// Focus is claimed ONCE in the ctor. Don't steal it back every
		// frame — doing so made the trade chat hijack keyboard input
		// when the player opened another UI with its own textfield.
	}

	Button::State UITrade::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CONFIRM:
			// Can't confirm until the other player has actually accepted
			// and entered the trade room.
			if (!partner_joined)
			{
				status_label.change_text("Waiting for the other player to accept...");
				return Button::State::NORMAL;
			}
			if (my_confirmed)
				return Button::State::NORMAL;
			UI::get().emplace<UIYesNo>(
				"Are you sure you want to trade?",
				[this](bool yes) { if (yes) confirm_trade(); });
			return Button::State::NORMAL;
		case BT_CANCEL:
			UI::get().emplace<UIYesNo>(
				"Are you sure you want to cancel?",
				[this](bool yes) { if (yes) cancel_trade(); });
			return Button::State::NORMAL;
		case BT_ENTER:
		{
			// Same path as pressing Enter inside the chat input. No local
			// echo — the server broadcasts the line back to us.
			std::string msg = chat_input.get_text();
			if (!msg.empty())
			{
				TradeChatPacket(msg).dispatch();
				chat_input.change_text("");
			}
			return Button::State::NORMAL;
		}
		case BT_REPORT:
			UI::get().emplace<UIReport>();
			return Button::State::NORMAL;
		case BT_COIN:
		{
			// The meso offer is locked once you've hit Confirm.
			if (my_confirmed)
				return Button::State::NORMAL;

			// Cap the prompt at the mesos we actually carry. UIEnterNumber
			// takes an int32_t max while the inventory stores an int64_t.
			int64_t meso = Stage::get().get_player().get_inventory().get_meso();
			if (meso > 2147483647LL)
				meso = 2147483647LL;
			int32_t max_meso = static_cast<int32_t>(meso);

			if (max_meso < 1)
				return Button::State::NORMAL;

			auto on_enter = [](int32_t amount)
			{
				// The server echoes SET_MESO back to both players
				// (handler case 16 → set_meso), so my_meso is updated
				// from that echo rather than written locally here.
				TradeSetMesoPacket(amount).dispatch();
			};
			UI::get().emplace<UIEnterNumber>(
				"How many mesos would you like to trade?",
				on_enter, max_meso, 1, -22);
			return Button::State::NORMAL;
		}
		}

		return Button::State::NORMAL;
	}

	Cursor::State UITrade::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Any interaction with the trade panel — click OR hover —
		// refocuses the chat input so the player can type after they
		// had focus somewhere else.
		if (chat_input.get_state() != Textfield::State::FOCUSED)
			chat_input.set_state(Textfield::State::FOCUSED);
		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UITrade::doubleclick(Point<int16_t> cursorpos)
	{
		Point<int16_t> cursoroff = cursorpos - position;
		bool is_my_side = false;
		int8_t trade_slot = slot_by_position(cursoroff, is_my_side);
		if (trade_slot < 0 || !is_my_side)
			return;
		if (my_items[trade_slot].empty())
			return;

		// v83/Cosmic trades don't support taking an item back once it's
		// placed — the server rejects any remove attempt. Wiping the slot
		// locally used to leave the client showing the item as removed
		// while the server still held it committed (a desync that could
		// trade the item away anyway). So we don't remove; we tell the
		// player how to actually reclaim it.
		status_label.change_text("Items can't be taken back — press Cancel to reclaim them.");
	}

	void UITrade::send_scroll(double yoffset)
	{
		if (yoffset == 0)
			return;

		// On this platform the wheel sometimes reports the same sign
		// for both directions, so instead of trusting the sign we just
		// advance chat_scroll by one step per wheel tick and wrap back
		// to 0 once we pass the amount of hidden history. That gives
		// the player a way to reach every line by scrolling in either
		// direction.
		int total     = static_cast<int>(chat_lines.size());
		int max_lines = (300 /*chat_bottom*/ - 5 /*chat_top*/ + 12 - 1) / 12 /*line_h*/;
		int overflow  = std::max(0, total - max_lines);
		if (overflow <= 0)
			return; // nothing to scroll

		chat_scroll = (chat_scroll + 3) % (overflow + 1);
	}

	void UITrade::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
			return;

		if (escape)
		{
			cancel_trade();
			return;
		}

		// UITrade is FOCUSED so UIStateGame routes every key here first.
		// Without this pass-through, the inventory / equip / stats / skill
		// hotkeys would do nothing while trading — which broke the whole
		// drag-an-item-in workflow.
		switch (keycode)
		{
		case KeyAction::Id::ITEMS:
			UI::get().emplace<UIItemInventory>(
				Stage::get().get_player().get_inventory());
			break;
		case KeyAction::Id::EQUIPMENT:
			UI::get().emplace<UIEquipInventory>(
				Stage::get().get_player().get_inventory());
			break;
		case KeyAction::Id::STATS:
			UI::get().emplace<UIStatsInfo>(
				Stage::get().get_player().get_stats());
			break;
		case KeyAction::Id::SKILLS:
			UI::get().emplace<UISkillBook>(
				Stage::get().get_player().get_stats(),
				Stage::get().get_player().get_skills());
			break;
		default:
			break;
		}
	}

	UIElement::Type UITrade::get_type() const
	{
		return TYPE;
	}

	void UITrade::set_partner(uint8_t slot, const std::string& name, const LookEntry& look)
	{
		partner_name = name;
		partner_name_label.change_text(name);
		status_label.change_text("Trading with " + name);

		partner_look = CharLook(look);
		has_partner_look = true;
		partner_joined = true;

		// Partner is in the room — the Confirm button becomes usable.
		if (!my_confirmed && buttons.count(BT_CONFIRM) && buttons[BT_CONFIRM])
			buttons[BT_CONFIRM]->set_state(Button::State::NORMAL);
	}

	void UITrade::set_item(uint8_t player_num, uint8_t slot, int32_t itemid, int16_t count)
	{
		if (slot >= MAX_SLOTS)
			return;

		TradeItem& item = (player_num == 0) ? my_items[slot] : partner_items[slot];

		item.itemid = itemid;
		item.count = count;

		const ItemData& idata = ItemData::get(itemid);
		item.icon = idata.get_icon(false);

		std::string name = idata.get_name();
		if (name.length() > 20)
			name = name.substr(0, 20) + "..";

		item.namelabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);

		if (count > 1)
			item.countlabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY, "x" + std::to_string(count));
		else
			item.countlabel = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DUSTYGRAY);
	}

	void UITrade::set_meso(uint8_t player_num, int32_t meso)
	{
		if (player_num == 0)
			my_meso = meso;
		else
			partner_meso = meso;
	}

	void UITrade::set_partner_confirmed()
	{
		partner_confirmed = true;
		// Dismiss the "Waiting for partner..." modal if we were waiting.
		UI::get().remove(UIElement::Type::NOTICE);
	}

	void UITrade::trade_result(uint8_t result)
	{
		// Drop the waiting modal; UITrade itself will deactivate shortly.
		UI::get().remove(UIElement::Type::NOTICE);

		// Report why the trade ended. Codes match Cosmic's TradeResult
		// enum (server/Trade.java). The window closes right after, so a
		// chat-log line is the durable place for this.
		std::string msg;
		chat::LineType color = chat::LineType::RED;

		switch (result)
		{
		case 1:  msg = "The trade was cancelled — the other player did not respond."; break;
		case 2:  msg = partner_name.empty()
				? "The other player declined or cancelled the trade."
				: partner_name + " declined or cancelled the trade."; break;
		case 7:  msg = "Trade completed successfully."; color = chat::LineType::WHITE; break;
		case 8:  msg = "The trade could not be completed."; break;
		case 9:  msg = "Trade failed — that would exceed a one-of-a-kind item limit."; break;
		case 12: msg = "The trade failed because a player changed maps."; break;
		case 13: msg = "The trade failed due to a data error."; break;
		default: msg = "The trade has ended."; break;
		}

		chat::log(msg, color);
	}

	void UITrade::add_chat(const std::string& message)
	{
		// The server echoes every trade line back to both players as
		// "<CharName> : <message>", so we do NOT locally echo — that would
		// double the message. Colour our own lines blue by matching the
		// leading name against the player's own character name.
		Color::Name c = Color::Name::BLACK;

		const std::string& myname = Stage::get().get_player().get_name();
		if (!myname.empty() && message.rfind(myname + " :", 0) == 0)
			c = Color::Name::MEDIUMBLUE;

		chat_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT,
			c, message, 200);
		if (chat_lines.size() > MAX_CHAT_LINES)
			chat_lines.erase(chat_lines.begin());
	}

	void UITrade::cancel_trade()
	{
		TradeExitPacket().dispatch();
		deactivate();
	}

	void UITrade::confirm_trade()
	{
		if (!my_confirmed)
		{
			my_confirmed = true;
			TradeConfirmPacket().dispatch();
			buttons[BT_CONFIRM]->set_state(Button::State::DISABLED);
			// Generic button-less "Waiting for partner..." modal. The
			// trade_result / partner-confirm handler removes it.
			UI::get().emplace<UIWaitNotice>("Waiting for partner...");
		}
	}

	bool UITrade::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds(position, position + dimension);
		return bounds.contains(cursorpos);
	}

	bool UITrade::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		// Only item/equip icons can be placed in trade slots.
		InventoryType::Id src_tab = icon.get_source_tab();
		int16_t src_slot = icon.get_source_slot();
		if (src_tab == InventoryType::Id::NONE || src_slot == 0)
			return true;

		Point<int16_t> cursoroff = cursorpos - position;
		bool is_my_side = false;
		int8_t trade_slot = slot_by_position(cursoroff, is_my_side);
		if (trade_slot < 0 || !is_my_side)
			return true; // drop outside our panel → cancel drag silently

		// Mirror UIStorage::request_store — equips (and anything that
		// stacks to 1) go straight to the server, stackables pop the
		// classic "How many..." prompt using the same layout as the
		// storage-put dialog.
		int16_t max_quantity = std::max<int16_t>(1, icon.get_count());

		// Server trade slots are 1-based in the v83 protocol.
		int8_t server_trade_slot = trade_slot + 1;
		int8_t invtype = static_cast<int8_t>(src_tab);

		if (src_tab == InventoryType::Id::EQUIP || max_quantity <= 1)
		{
			TradeSetItemPacket(invtype, src_slot, 1, server_trade_slot).dispatch();
		}
		else
		{
			auto on_enter = [invtype, src_slot, server_trade_slot](int32_t quantity)
			{
				auto shortqty = static_cast<int16_t>(quantity);
				TradeSetItemPacket(invtype, src_slot, shortqty, server_trade_slot).dispatch();
			};
			UI::get().emplace<UIEnterNumber>(
				"How many would you like to trade?",
				on_enter, max_quantity, 1, -22);
		}
		return true;
	}

	int8_t UITrade::slot_by_position(Point<int16_t> cursoroff, bool& is_my_side) const
	{
		// Side-by-side grids — must match draw() constants exactly.
		// Red (partner) on the left, Green (me) on the right.
		constexpr int16_t slot_w = 36;
		constexpr int16_t slot_h = 36;
		constexpr int16_t cols = 3;
		constexpr int16_t partner_grid_x = 14;
		constexpr int16_t my_grid_x      = partner_grid_x + cols * (slot_w + 2) + 20;
		constexpr int16_t grid_y = 150;

		for (int i = 0; i < MAX_SLOTS; i++)
		{
			int16_t col = i % cols;
			int16_t row = i / cols;
			int16_t y = grid_y + row * (slot_h + 2);

			int16_t mx = my_grid_x + col * (slot_w + 2);
			Rectangle<int16_t> my_rect(mx, mx + slot_w, y, y + slot_h);
			if (my_rect.contains(cursoroff))
			{
				is_my_side = true;
				return static_cast<int8_t>(i);
			}

			int16_t px = partner_grid_x + col * (slot_w + 2);
			Rectangle<int16_t> pr_rect(px, px + slot_w, y, y + slot_h);
			if (pr_rect.contains(cursoroff))
			{
				is_my_side = false;
				return static_cast<int8_t>(i);
			}
		}

		return -1;
	}
}
