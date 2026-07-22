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
#include "UIPersonalShop.h"

#include "UINotice.h"

#include "../../Constants.h"
#include "../../Data/ItemData.h"
#include "../Components/Icon.h"
#include <algorithm>

#include "../Components/MapleButton.h"
#include "../UI.h"
#include "UIChatBar.h"

#include "../../Net/Packets/TradePackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIPersonalShop::UIPersonalShop() : UIDragElement<PosPERSONALSHOP>(Point<int16_t>(250, 20)),
		item_count(0), selected_slot(-1), is_owner(false)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["PersonalShop"]["main"];

		nl::node backgrnd = src["backgrnd"];
		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		sprites.emplace_back(backgrnd);

		nl::node backgrnd2 = src["backgrnd2"];

		if (backgrnd2.size() > 0)
			sprites.emplace_back(backgrnd2);

		nl::node backgrnd3 = src["backgrnd3"];

		if (backgrnd3.size() > 0)
			sprites.emplace_back(backgrnd3);

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(src["BtClose"]);
		buttons[Buttons::BT_BUY] = std::make_unique<MapleButton>(src["BtBuy"]);
		buttons[Buttons::BT_START] = std::make_unique<MapleButton>(src["BtStart"]);
		buttons[Buttons::BT_ENTER] = std::make_unique<MapleButton>(src["BtEnter"]);
		buttons[Buttons::BT_EXIT] = std::make_unique<MapleButton>(src["BtExit"]);
		buttons[Buttons::BT_ITEM] = std::make_unique<MapleButton>(src["BtItem"]);
		buttons[Buttons::BT_INFO] = std::make_unique<MapleButton>(src["BtInfo"]);
		buttons[Buttons::BT_COIN] = std::make_unique<MapleButton>(src["BtCoin"]);
		buttons[Buttons::BT_BAN] = std::make_unique<MapleButton>(src["BtBan"]);
		buttons[Buttons::BT_BLACKLIST] = std::make_unique<MapleButton>(src["BtBlackList"]);

		select_tex = src["select"];
		soldout_tex = src["soldout"];

		// Visitor buttons initially hidden; owner buttons initially hidden
		buttons[Buttons::BT_BUY]->set_active(false);
		buttons[Buttons::BT_START]->set_active(false);
		buttons[Buttons::BT_BAN]->set_active(false);
		buttons[Buttons::BT_BLACKLIST]->set_active(false);

		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0, false };

		owner_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "");
		slot_name_text = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);

		chat_field = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(Point<int16_t>(233, 516), Point<int16_t>(425, 534)),
			28
		);
		chat_field.set_enter_callback(
			[this](std::string msg)
			{
				if (!msg.empty())
				{
					TradeChatPacket(msg).dispatch();
					chat_field.change_text("");
				}
			}
		);
		item_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		price_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLUE);

		rebuild_chat_slider();

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(dimension.x(), 20);
	}

	bool UIPersonalShop::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (!is_owner)
			return true;

		InventoryType::Id src_tab = icon.get_source_tab();
		int16_t src_slot = icon.get_source_slot();

		if (src_tab == InventoryType::Id::NONE || src_slot == 0)
			return true;

		int8_t invtype = static_cast<int8_t>(src_tab);
		int16_t max_quantity = std::max<int16_t>(1, icon.get_count());

		auto ask_price = [invtype, src_slot](int16_t perbundle)
		{
			UI::get().emplace<UIEnterNumber>(
				"Price per bundle?",
				[invtype, src_slot, perbundle](int32_t price)
				{
					if (price > 0)
						AddShopItemPacket(invtype, src_slot, 1, perbundle, price).dispatch();
				},
				999999999, 1, -22);
		};

		if (src_tab == InventoryType::Id::EQUIP || max_quantity <= 1)
		{
			ask_price(1);
		}
		else
		{
			UI::get().emplace<UIEnterNumber>(
				"How many per bundle?",
				[ask_price, max_quantity](int32_t qty)
				{
					if (qty > 0 && qty <= max_quantity)
						ask_price(static_cast<int16_t>(qty));
				},
				std::min<int16_t>(max_quantity, 2000), 1, -22);
		}

		return true;
	}

	void UIPersonalShop::set_slot_look(int8_t slot, const LookEntry& entry, const std::string& name)
	{
		if (slot < 0 || slot > 3)
			return;

		slot_looks[slot] = CharLook(entry);
		slot_looks[slot].set_stance(Stance::Id::STAND1);
		slot_used[slot] = true;
		slot_names[slot] = name;
	}

	void UIPersonalShop::remove_visitor(int8_t slot)
	{
		if (slot >= 1 && slot <= 3)
		{
			slot_used[slot] = false;
			slot_names[slot].clear();
		}
	}

	void UIPersonalShop::rebuild_item_slider()
	{
		int16_t rows = item_count;

		item_slider = Slider(
			Slider::Type::THIN_MINESHAFT,
			Range<int16_t>(184, 388),
			198,
			5,
			rows < 1 ? 1 : rows,
			[this](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				int16_t max_off = item_count - 5;

				if (max_off < 0)
					max_off = 0;

				item_offset += shift;

				if (item_offset < 0)
					item_offset = 0;

				if (item_offset > max_off)
					item_offset = max_off;
			}
		);
	}

	void UIPersonalShop::rebuild_chat_slider()
	{
		int16_t rows = static_cast<int16_t>(chat_lines.size());

		chat_slider = Slider(
			Slider::Type::THIN_MINESHAFT,
			Range<int16_t>(279, 500),
			503,
			16,
			rows < 1 ? 1 : rows,
			[this](bool upwards)
			{
				int16_t shift = upwards ? -1 : 1;
				int16_t max_off = static_cast<int16_t>(chat_lines.size()) - 16;

				if (max_off < 0)
					max_off = 0;

				chat_offset += shift;

				if (chat_offset < 0)
					chat_offset = 0;

				if (chat_offset > max_off)
					chat_offset = max_off;
			}
		);
	}

	void UIPersonalShop::add_chat(const std::string& line, int8_t speaker)
	{
		Color::Name color;

		switch (speaker)
		{
		case 1: color = Color::Name::MALIBU; break;
		case 2: color = Color::Name::JAPANESELAUREL; break;
		case 3: color = Color::Name::ORANGE; break;
		default: color = Color::Name::BLACK; break;
		}

		chat_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, color, line, 258);

		if (chat_lines.size() > 60)
			chat_lines.erase(chat_lines.begin());

		int16_t max_off = static_cast<int16_t>(chat_lines.size()) - 16;
		chat_offset = max_off < 0 ? 0 : max_off;
		rebuild_chat_slider();
		rebuild_item_slider();
	}

	Cursor::State UIPersonalShop::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> off = cursorpos - position;

		if (chat_lines.size() > 16)
		{
			Cursor::State sstate = chat_slider.send_cursor(off, clicked);

			if (sstate != Cursor::State::IDLE)
				return sstate;
		}

		if (item_count > 5)
		{
			Cursor::State istate = item_slider.send_cursor(off, clicked);

			if (istate != Cursor::State::IDLE)
				return istate;
		}

		if (clicked)
		{
			if (off.x() >= 5 && off.x() <= 195 && off.y() >= 186 && off.y() < 186 + 5 * 42)
			{
				int8_t row = static_cast<int8_t>((off.y() - 186) / 42);
				int8_t idx = row + item_offset;

				if (idx < item_count && items[idx].itemid > 0)
					selected_slot = idx;
			}

			Rectangle<int16_t> field_rect(Point<int16_t>(229, 512), Point<int16_t>(429, 538));

			if (field_rect.contains(off))
			{
				chat_field.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			if (chat_field.get_state() == Textfield::State::FOCUSED)
				chat_field.set_state(Textfield::State::NORMAL);
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIPersonalShop::doubleclick(Point<int16_t> cursorpos)
	{
		if (!is_owner)
			return;

		Point<int16_t> off = cursorpos - position;

		if (off.x() >= 5 && off.x() <= 195 && off.y() >= 186 && off.y() < 186 + 5 * 42)
		{
			int8_t idx = static_cast<int8_t>((off.y() - 186) / 42) + item_offset;

			if (idx < item_count && !items[idx].sold_out)
				ShopRemoveItemPacket(idx).dispatch();
		}
	}

	void UIPersonalShop::set_mode(bool owner)
	{
		is_owner = owner;
		buttons[Buttons::BT_BUY]->set_active(!owner);
		buttons[Buttons::BT_EXIT]->set_active(!owner);
		buttons[Buttons::BT_INFO]->set_active(!owner);
		buttons[Buttons::BT_CLOSE]->set_active(owner);
		buttons[Buttons::BT_START]->set_active(owner);
		buttons[Buttons::BT_ITEM]->set_active(owner);
		buttons[Buttons::BT_BAN]->set_active(false);
		buttons[Buttons::BT_BLACKLIST]->set_active(owner);
	}

	void UIPersonalShop::draw(float inter) const
	{
		UIElement::draw(inter);

		owner_label.draw(position + Point<int16_t>(dimension.x() / 2, 9));

		if (slot_used[0])
		{
			slot_looks[0].draw(DrawArgument(position + Point<int16_t>(52, 127)), inter);
			slot_name_text.change_color(Color::Name::BLACK);
			slot_name_text.change_text(slot_names[0]);
			slot_name_text.draw(position + Point<int16_t>(49, 131));
		}

		for (int8_t v = 1; v <= 3; v++)
		{
			if (!slot_used[v])
				continue;

			int16_t cx = 276 + (v - 1) * 96;
			slot_looks[v].draw(DrawArgument(position + Point<int16_t>(cx, 158)), inter);
			slot_name_text.change_color(Color::Name::WHITE);
			slot_name_text.change_text(slot_names[v]);
			slot_name_text.draw(position + Point<int16_t>(cx, 160));
		}

		int16_t cy = 279;

		for (size_t i = chat_offset; i < chat_lines.size(); i++)
		{
			int16_t lh = std::max<int16_t>(14, chat_lines[i].height());

			if (cy + lh > 515)
				break;

			chat_lines[i].draw(position + Point<int16_t>(230, cy));
			cy += lh;
		}

		if (chat_lines.size() > 16)
			chat_slider.draw(position);

		if (item_count > 5)
			item_slider.draw(position);

		chat_field.draw(position, Point<int16_t>(0, -5));

		for (int8_t r = 0; r < 5; r++)
		{
			int8_t i = r + item_offset;

			if (i >= item_count)
				break;

			int16_t row_y = 186 + r * 42;

			if (i == selected_slot)
				select_tex.draw(position + Point<int16_t>(33, row_y - 2));

			if (items[i].itemid > 0)
			{
				const ItemData& data = ItemData::get(items[i].itemid);

				if (data.is_valid())
				{
					data.get_icon(false).draw(DrawArgument(position + Point<int16_t>(10, row_y + 30)));
					item_label.change_text(data.get_name() + "  x" + std::to_string(items[i].quantity));
				}
				else
				{
					item_label.change_text("#" + std::to_string(items[i].itemid) + "  x" + std::to_string(items[i].quantity));
				}

				item_label.draw(position + Point<int16_t>(49, row_y - 1));

				price_label.change_text(std::to_string(items[i].price) + " meso");
				price_label.draw(position + Point<int16_t>(49, row_y + 13));

				if (items[i].sold_out)
					soldout_tex.draw(position + Point<int16_t>(33, row_y));
			}
		}
	}

	void UIPersonalShop::send_scroll(double yoffset)
	{
		Point<int16_t> off = UI::get().get_cursor_position() - position;
		int16_t shift = yoffset > 0 ? -1 : 1;

		if (off.x() < 205 && off.y() >= 180 && off.y() < 400)
		{
			int16_t max_off = item_count - 5;

			if (max_off < 0)
				max_off = 0;

			item_offset += shift;

			if (item_offset < 0)
				item_offset = 0;

			if (item_offset > max_off)
				item_offset = max_off;
		}
		else
		{
			int16_t max_off = static_cast<int16_t>(chat_lines.size()) - 16;

			if (max_off < 0)
				max_off = 0;

			chat_offset += shift;

			if (chat_offset < 0)
				chat_offset = 0;

			if (chat_offset > max_off)
				chat_offset = max_off;
		}
	}

	void UIPersonalShop::update()
	{
		UIElement::update();

		chat_field.update(position);

		for (int i = 0; i < 4; i++)
			if (slot_used[i])
				slot_looks[i].update(Constants::TIMESTEP);
	}

	void UIPersonalShop::set_owner(const std::string& name)
	{
		owner_name = name;
		owner_label.change_text(name + "'s Shop");
	}

	void UIPersonalShop::add_item(int8_t slot, int32_t itemid, int16_t quantity, int32_t price)
	{
		if (slot >= 0 && slot < MAX_ITEMS)
		{
			items[slot] = { itemid, quantity, price, false };

			if (slot >= item_count)
				item_count = slot + 1;
		}

		rebuild_item_slider();
	}

	void UIPersonalShop::set_sold_out(int8_t slot)
	{
		if (slot >= 0 && slot < MAX_ITEMS)
			items[slot].sold_out = true;
	}

	void UIPersonalShop::clear_items()
	{
		item_offset = 0;
		selected_slot = -1;
		for (int i = 0; i < MAX_ITEMS; i++)
			items[i] = { 0, 0, 0, false };

		item_count = 0;
		selected_slot = -1;

		rebuild_item_slider();
	}

	void UIPersonalShop::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			TradeExitPacket().dispatch();
			deactivate();
		}
	}

	UIElement::Type UIPersonalShop::get_type() const
	{
		return TYPE;
	}

	Button::State UIPersonalShop::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_CLOSE:
		case Buttons::BT_EXIT:
			TradeExitPacket().dispatch();
			deactivate();
			break;
		case Buttons::BT_BUY:
			if (selected_slot >= 0 && !items[selected_slot].sold_out)
				ShopBuyPacket(selected_slot, items[selected_slot].quantity).dispatch();
			break;
		case Buttons::BT_START:
			ShopOpenPacket().dispatch();
			break;
		case Buttons::BT_ENTER:
		{
			std::string msg = chat_field.get_text();

			if (!msg.empty())
			{
				TradeChatPacket(msg).dispatch();
				chat_field.change_text("");
			}

			break;
		}
		case Buttons::BT_ITEM:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line("Use the inventory to add items.", UIChatBar::YELLOW);
			break;
		case Buttons::BT_INFO:
			if (auto chatbar = UI::get().get_element<UIChatBar>())
				chatbar->send_line(owner_name + "'s Shop - " + std::to_string(item_count) + " item(s)", UIChatBar::YELLOW);
			break;
		case Buttons::BT_COIN:
			ShopTakeMesoPacket().dispatch();
			break;
		case Buttons::BT_BAN:
			ShopBanVisitorPacket().dispatch();
			break;
		case Buttons::BT_BLACKLIST:
			ShopBlacklistPacket().dispatch();
			break;
		default:
			break;
		}

		return Button::State::NORMAL;
	}
}
