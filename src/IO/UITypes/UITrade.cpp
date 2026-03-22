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
#include "UITrade.h"

#include "UINotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UITrade::UITrade() : UIDragElement<PosTRADE>(Point<int16_t>(250, 20))
	{
		nl::node src = nl::nx::ui["UIWindow2.img"]["MiniRoom"];

		// Try to load NX background
		nl::node backgrnd = src["backgrnd"];

		if (backgrnd.size() > 0)
		{
			background = backgrnd;
			sprites.emplace_back(backgrnd);

			nl::node backgrnd2 = src["backgrnd2"];
			if (backgrnd2.size() > 0)
				sprites.emplace_back(backgrnd2);

			nl::node backgrnd3 = src["backgrnd3"];
			if (backgrnd3.size() > 0)
				sprites.emplace_back(backgrnd3);
		}

		// Try to load NX buttons, fallback to AreaButtons
		nl::node bt_ok = src["BtOK"];
		nl::node bt_cancel = src["BtCancel"];

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

		if (bt_cancel.size() > 0)
			buttons[BT_CANCEL] = std::make_unique<MapleButton>(bt_cancel);
		else
			buttons[BT_CANCEL] = std::make_unique<AreaButton>(
				Point<int16_t>(bg_dim.x() / 2 + 10, bg_dim.y() - 35),
				Point<int16_t>(70, 24)
			);

		// Labels
		std::string player_name = Stage::get().get_player().get_stats().get_name();
		my_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, player_name);
		partner_name_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "");
		my_meso_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, "0 meso");
		partner_meso_label = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, "0 meso");
		status_label = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::YELLOW, "Waiting for partner...");

		my_meso = 0;
		partner_meso = 0;
		my_confirmed = false;
		partner_confirmed = false;
		selected_slot = -1;

		dimension = bg_dim;
		dragarea = Point<int16_t>(bg_dim.x(), 20);

		active = true;
	}

	void UITrade::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Layout constants
		int16_t left_x = 12;         // My items column x
		int16_t right_x = 210;       // Partner items column x
		int16_t slot_y_start = 50;   // First slot y
		int16_t slot_w = 36;
		int16_t slot_h = 36;
		int16_t cols = 3;

		// Draw "My Items" header
		my_name_label.draw(position + Point<int16_t>(left_x, 30));
		partner_name_label.draw(position + Point<int16_t>(right_x, 30));

		// Draw item slots
		for (int i = 0; i < MAX_SLOTS; i++)
		{
			int16_t col = i % cols;
			int16_t row = i / cols;
			Point<int16_t> my_pos = position + Point<int16_t>(left_x + col * (slot_w + 2), slot_y_start + row * (slot_h + 2));
			Point<int16_t> partner_pos = position + Point<int16_t>(right_x + col * (slot_w + 2), slot_y_start + row * (slot_h + 2));

			// Draw slot background (simple box outline)
			// My slot
			if (!my_items[i].empty())
			{
				if (my_items[i].icon.is_valid())
					my_items[i].icon.draw(DrawArgument(my_pos + Point<int16_t>(2, slot_h - 4)));
			}

			// Partner slot
			if (!partner_items[i].empty())
			{
				if (partner_items[i].icon.is_valid())
					partner_items[i].icon.draw(DrawArgument(partner_pos + Point<int16_t>(2, slot_h - 4)));
			}
		}

		// Draw meso labels
		int16_t meso_y = slot_y_start + 3 * (slot_h + 2) + 10;
		my_meso_label.draw(position + Point<int16_t>(left_x + 100, meso_y));
		partner_meso_label.draw(position + Point<int16_t>(right_x + 100, meso_y));

		// Draw confirmation status
		int16_t status_y = meso_y + 20;
		if (my_confirmed)
		{
			Text confirmed_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::GREEN, "Confirmed");
			confirmed_text.draw(position + Point<int16_t>(left_x, status_y));
		}
		if (partner_confirmed)
		{
			Text confirmed_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::GREEN, "Confirmed");
			confirmed_text.draw(position + Point<int16_t>(right_x, status_y));
		}

		// Draw status
		status_label.draw(position + Point<int16_t>(dimension.x() / 2, dimension.y() - 50));

		// Draw button labels if using AreaButtons
		if (background.get_dimensions().x() == 0)
		{
			Text confirm_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Confirm");
			Text cancel_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE, "Cancel");
			confirm_text.draw(position + Point<int16_t>(dimension.x() / 2 - 45, dimension.y() - 30));
			cancel_text.draw(position + Point<int16_t>(dimension.x() / 2 + 45, dimension.y() - 30));
		}
	}

	void UITrade::update()
	{
		UIElement::update();

		std::string mesostr;

		mesostr = std::to_string(my_meso);
		string_format::split_number(mesostr);
		my_meso_label.change_text(mesostr + " meso");

		mesostr = std::to_string(partner_meso);
		string_format::split_number(mesostr);
		partner_meso_label.change_text(mesostr + " meso");
	}

	Button::State UITrade::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CONFIRM:
			confirm_trade();
			return Button::State::NORMAL;
		case BT_CANCEL:
			cancel_trade();
			return Button::State::NORMAL;
		}

		return Button::State::NORMAL;
	}

	Cursor::State UITrade::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UITrade::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			cancel_trade();
	}

	UIElement::Type UITrade::get_type() const
	{
		return TYPE;
	}

	void UITrade::set_partner(uint8_t slot, const std::string& name)
	{
		partner_name = name;
		partner_name_label.change_text(name);
		status_label.change_text("Trading with " + name);
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
		status_label.change_text("Partner confirmed. Your turn!");
	}

	void UITrade::trade_result(uint8_t result)
	{
		switch (result)
		{
		case 7:  // SUCCESSFUL
			status_label.change_text("Trade completed!");
			break;
		case 2:  // PARTNER_CANCEL
			status_label.change_text("Partner cancelled the trade.");
			break;
		case 8:  // UNSUCCESSFUL
			status_label.change_text("Trade failed.");
			break;
		case 9:  // UNSUCCESSFUL_UNIQUE_ITEM_LIMIT
			status_label.change_text("Trade failed: unique item limit.");
			break;
		default:
			status_label.change_text("Trade ended.");
			break;
		}
	}

	void UITrade::add_chat(const std::string& message)
	{
		// Could display in a chat area within the trade window
		// For now, just update status label with last message
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
			status_label.change_text("Waiting for partner to confirm...");
			buttons[BT_CONFIRM]->set_state(Button::State::DISABLED);
		}
	}

	int8_t UITrade::slot_by_position(Point<int16_t> cursoroff, bool& is_my_side) const
	{
		int16_t left_x = 12;
		int16_t right_x = 210;
		int16_t slot_y_start = 50;
		int16_t slot_w = 36;
		int16_t slot_h = 36;
		int16_t cols = 3;

		for (int i = 0; i < MAX_SLOTS; i++)
		{
			int16_t col = i % cols;
			int16_t row = i / cols;

			// Check my side
			Rectangle<int16_t> my_rect(
				left_x + col * (slot_w + 2),
				left_x + col * (slot_w + 2) + slot_w,
				slot_y_start + row * (slot_h + 2),
				slot_y_start + row * (slot_h + 2) + slot_h
			);

			if (my_rect.contains(cursoroff))
			{
				is_my_side = true;
				return static_cast<int8_t>(i);
			}

			// Check partner side
			Rectangle<int16_t> partner_rect(
				right_x + col * (slot_w + 2),
				right_x + col * (slot_w + 2) + slot_w,
				slot_y_start + row * (slot_h + 2),
				slot_y_start + row * (slot_h + 2) + slot_h
			);

			if (partner_rect.contains(cursoroff))
			{
				is_my_side = false;
				return static_cast<int8_t>(i);
			}
		}

		return -1;
	}
}
