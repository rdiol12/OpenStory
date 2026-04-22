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
#include "UICharInfo.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/PlayerInteractionPackets.h"
#include "../../Net/Packets/TradePackets.h"
#include "../../Net/Packets/BotInventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UICharInfo::UICharInfo(int32_t cid) : UIDragElement<PosCHARINFO>(Point<int16_t>(250, 20)), bot_tab_active(false), target_char_id(cid), bot_scroll_offset(0)
	{
		nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
		nl::node UserInfo = nl::nx::ui["UIWindow.img"]["UserInfo"];

		// Use UIWindow2.img backgrounds (correct layer structure) with UIWindow.img buttons
		nl::node UserInfo2 = nl::nx::ui["UIWindow2.img"]["UserInfo"];
		nl::node character = UserInfo2["character"];

		sprites.emplace_back(character["backgrnd"]);
		sprites.emplace_back(character["backgrnd2"]);
		sprites.emplace_back(character["name"]);

		Point<int16_t> backgrnd_dim = Texture(character["backgrnd"]).get_dimensions();

		Point<int16_t> close_dimensions = Point<int16_t>(backgrnd_dim.x() - 21, 6);
		buttons[Buttons::BtClose] = std::make_unique<MapleButton>(close, close_dimensions);

		auto add_button = [&](uint16_t id, nl::node src) {
			if (src.size() > 0)
				buttons[id] = std::make_unique<MapleButton>(src);
		};

		// Buttons from UIWindow2.img/UserInfo/character (have built-in positions)
		add_button(Buttons::BtFamily, character["BtFamily"]);
		add_button(Buttons::BtParty, character["BtParty"]);
		add_button(Buttons::BtPet, character["BtPet"]);
		add_button(Buttons::BtRide, character["BtRide"]);
		add_button(Buttons::BtTrad, character["BtTrad"]);
		add_button(Buttons::BtItem, character["BtItem"]);
		add_button(Buttons::BtPopUp, character["BtPopUp"]);
		add_button(Buttons::BtPopDown, character["BtPopDown"]);

		name = Text(Text::Font::A12M, Text::Alignment::CENTER, Color::Name::WHITE);
		job = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		level = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		fame = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		guild = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		alliance = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);

		if (buttons.count(Buttons::BtPet))
			buttons[Buttons::BtPet]->set_state(Button::State::DISABLED);
		if (buttons.count(Buttons::BtRide))
			buttons[Buttons::BtRide]->set_state(Button::State::DISABLED);

		dimension = backgrnd_dim;
		charinfo_dim = backgrnd_dim;
		dragarea = Point<int16_t>(dimension.x(), 20);

		target_character = Stage::get().get_character(cid).get();

		// Bot inventory labels
		bot_name_label = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		bot_level_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);
		bot_meso_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::EMPEROR);

		// Load NX sprites for bot inventory (reuse Item inventory assets)
		nl::node Item = nl::nx::ui["UIWindow2.img"]["Item"];
		bot_backgrnd = Item["productionBackgrnd"];
		bot_backgrnd2 = Item["productionBackgrnd2"];
		bot_backgrnd3 = Item["productionBackgrnd3"];
		bot_slot_disabled = Item["disabled"];

		CharInfoRequestPacket(cid).dispatch();
	}

	void UICharInfo::draw(float inter) const
	{
		if (bot_tab_active && bot_inventory.is_valid())
		{
			draw_bot_inventory(inter);

			auto it = buttons.find(static_cast<uint16_t>(Buttons::BtClose));
			if (it != buttons.end() && it->second)
				it->second->draw(position);
			return;
		}

		UIElement::draw_sprites(inter);

		for (auto& btn : buttons)
		{
			if (btn.first < Buttons::BtBotEquip && btn.second)
				btn.second->draw(position);
		}

		/// Main Window
		int16_t row_height = 18;
		Point<int16_t> text_pos = position + Point<int16_t>(153, 65);

		if (target_character)
			target_character->draw_preview(position + Point<int16_t>(63, 129), inter);

		name.draw(position + Point<int16_t>(59, 131));
		level.draw(text_pos + Point<int16_t>(0, row_height * 0));
		job.draw(text_pos + Point<int16_t>(0, row_height * 1));
		fame.draw(text_pos + Point<int16_t>(0, row_height * 2));
		guild.draw(text_pos + Point<int16_t>(0, row_height * 3) + Point<int16_t>(0, 1));
		alliance.draw(text_pos + Point<int16_t>(0, row_height * 4));
	}

	void UICharInfo::update() {}

	Button::State UICharInfo::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BtClose:
			if (bot_tab_active)
			{
				bot_tab_active = false;
				dimension = charinfo_dim;
				dragarea = Point<int16_t>(dimension.x(), 20);
			}
			else
			{
				deactivate();
			}
			return Button::State::NORMAL;
		case Buttons::BtFamily:
		case Buttons::BtParty:
			break;
		case Buttons::BtTrad:
			if (target_character)
			{
				TradeCreatePacket().dispatch();
				TradeInvitePacket(target_character->get_oid()).dispatch();
			}
			deactivate();
			return Button::State::NORMAL;
		case Buttons::BtItem:
		{
			if (bot_inventory.is_valid())
			{
				bot_tab_active = true;
				dimension = bot_backgrnd.get_dimensions();
				dragarea = Point<int16_t>(dimension.x(), 20);
				load_bot_icons();
			}
			return Button::State::NORMAL;
		}
		case Buttons::BtRide:
		case Buttons::BtPet:
			return Button::State::NORMAL;
		case Buttons::BtPopUp:
			if (target_character)
				GiveFamePacket(target_character->get_oid(), true).dispatch();
			return Button::State::NORMAL;
		case Buttons::BtPopDown:
			if (target_character)
				GiveFamePacket(target_character->get_oid(), false).dispatch();
			return Button::State::NORMAL;
		case Buttons::BtBotEquip:
			bot_sub_tab = 0;
			bot_scroll_offset = 0;
			load_bot_icons();
			return Button::State::NORMAL;
		case Buttons::BtBotUse:
			bot_sub_tab = 1;
			bot_scroll_offset = 0;
			load_bot_icons();
			return Button::State::NORMAL;
		case Buttons::BtBotSetup:
			bot_sub_tab = 2;
			bot_scroll_offset = 0;
			load_bot_icons();
			return Button::State::NORMAL;
		case Buttons::BtBotEtc:
			bot_sub_tab = 3;
			bot_scroll_offset = 0;
			load_bot_icons();
			return Button::State::NORMAL;
		case Buttons::BtBotEquipped:
			bot_sub_tab = 4;
			bot_scroll_offset = 0;
			load_bot_icons();
			return Button::State::NORMAL;
		default:
			break;
		}

		return Button::State::DISABLED;
	}

	bool UICharInfo::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + dimension);
		return bounds.contains(cursorpos);
	}

	void UICharInfo::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			if (bot_tab_active)
			{
				bot_tab_active = false;
				dimension = charinfo_dim;
				dragarea = Point<int16_t>(dimension.x(), 20);
			}
			else
			{
				deactivate();
			}
		}
	}

	UIElement::Type UICharInfo::get_type() const
	{
		return TYPE;
	}

	void UICharInfo::update_stats(int32_t character_id, int16_t job_id, int8_t lv, int16_t f, std::string g, std::string a)
	{
		int32_t player_id = Stage::get().get_player().get_oid();

		auto disable_button = [&](uint16_t id) {
			if (buttons.count(id) && buttons[id])
				buttons[id]->set_state(Button::State::DISABLED);
		};

		if (character_id == player_id)
		{
			disable_button(Buttons::BtParty);
			disable_button(Buttons::BtTrad);
			disable_button(Buttons::BtPopUp);
			disable_button(Buttons::BtPopDown);
		}

		Job character_job = Job(job_id);

		if (target_character)
			name.change_text(target_character->get_name());

		job.change_text(character_job.get_name());
		level.change_text(std::to_string(lv));
		fame.change_text(std::to_string(f));
		guild.change_text((g == "" ? "-" : g));
		alliance.change_text(a);

		if (buttons.count(Buttons::BtPet))
		{
			if (target_character && target_character->has_pet())
				buttons[Buttons::BtPet]->set_state(Button::State::NORMAL);
			else
				buttons[Buttons::BtPet]->set_state(Button::State::DISABLED);
		}

		if (buttons.count(Buttons::BtRide))
		{
			if (target_character && target_character->has_mount())
				buttons[Buttons::BtRide]->set_state(Button::State::NORMAL);
			else
				buttons[Buttons::BtRide]->set_state(Button::State::DISABLED);
		}
	}

	int32_t UICharInfo::get_char_id() const
	{
		return target_char_id;
	}

	void UICharInfo::set_bot_inventory(BotInventoryData data)
	{
		bot_inventory = std::move(data);
		bot_scroll_offset = 0;

		// Create tab buttons if they don't exist
		if (!buttons.count(Buttons::BtBotEquip))
		{
			nl::node Item = nl::nx::ui["UIWindow2.img"]["Item"];
			nl::node Tab = Item["Tab"];
			nl::node taben = Tab["enabled"];
			nl::node tabdis = Tab["disabled"];

			buttons[Buttons::BtBotEquip] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
			buttons[Buttons::BtBotUse] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
			buttons[Buttons::BtBotSetup] = std::make_unique<TwoSpriteButton>(tabdis["3"], taben["3"]);
			buttons[Buttons::BtBotEtc] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
			buttons[Buttons::BtBotEquipped] = std::make_unique<TwoSpriteButton>(tabdis["4"], taben["4"]);
		}

		bot_sub_tab = 4;
		bot_tab_active = true;
		dimension = bot_backgrnd.get_dimensions();
		dragarea = Point<int16_t>(dimension.x(), 20);

		bot_name_label.change_text(bot_inventory.name);
		bot_level_label.change_text("Lv. " + std::to_string(bot_inventory.level));

		std::string meso_str = std::to_string(bot_inventory.meso);
		string_format::split_number(meso_str);
		bot_meso_label.change_text("Meso: " + meso_str);

		load_bot_icons();
	}

	void UICharInfo::draw_bot_inventory(float inter) const
	{
		bot_backgrnd.draw(position);
		bot_backgrnd2.draw(position);
		bot_backgrnd3.draw(position);

		for (uint16_t i = Buttons::BtBotEquip; i <= Buttons::BtBotEquipped; i++)
		{
			auto it = buttons.find(i);
			if (it != buttons.end() && it->second)
				it->second->draw(position);
		}

		const auto& items = bot_inventory.get_tab(bot_sub_tab);
		draw_bot_item_grid(items, BOT_GRID_Y - bot_scroll_offset);

		bot_name_label.draw(position + Point<int16_t>(10, 262));
		bot_level_label.draw(position + Point<int16_t>(80, 262));
		bot_meso_label.draw(position + Point<int16_t>(10, 278));
	}

	void UICharInfo::draw_bot_item_grid(const std::vector<BotItem>& items, int16_t start_y) const
	{
		for (size_t i = 0; i < BOT_MAX_SLOTS; i++)
		{
			Point<int16_t> slotpos = position + get_bot_slotpos(static_cast<int16_t>(i));

			if (i >= items.size())
			{
				bot_slot_disabled.draw(slotpos);
			}
			else
			{
				auto it = bot_icons.find(static_cast<int16_t>(i));
				if (it != bot_icons.end() && it->second)
					it->second->draw(slotpos);
				else
					bot_slot_disabled.draw(slotpos);
			}
		}
	}

	Cursor::State UICharInfo::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (bot_tab_active && bot_inventory.is_valid())
		{
			Point<int16_t> cursor_rel = cursorpos - position;
			int16_t slot_index = bot_slot_by_position(cursor_rel);
			Icon* icon = get_bot_icon(slot_index);

			if (icon)
			{
				if (clicked)
				{
					Point<int16_t> slotpos = get_bot_slotpos(slot_index);
					icon->start_drag(cursor_rel - slotpos);
					UI::get().drag_icon(icon);

					return Cursor::State::GRABBING;
				}
				else
				{
					const auto& items = bot_inventory.get_tab(bot_sub_tab);
					if (slot_index < static_cast<int16_t>(items.size()))
						UI::get().show_item(Tooltip::Parent::CHARINFO, items[slot_index].item_id);

					return Cursor::State::CANGRAB;
				}
			}
		}

		return dstate;
	}

	void UICharInfo::send_scroll(double yoffset)
	{
		if (bot_tab_active && bot_inventory.is_valid())
		{
			const auto& items = bot_inventory.get_tab(bot_sub_tab);
			int rows = (static_cast<int>(items.size()) + BOT_COLS - 1) / BOT_COLS;
			int16_t max_scroll = std::max(0, rows * BOT_ICON_H - (dimension.y() - BOT_GRID_Y - 10));

			bot_scroll_offset -= static_cast<int16_t>(yoffset * BOT_ICON_H);

			if (bot_scroll_offset < 0)
				bot_scroll_offset = 0;
			else if (bot_scroll_offset > max_scroll)
				bot_scroll_offset = max_scroll;
		}
	}

	bool UICharInfo::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (!bot_tab_active || !bot_inventory.is_valid())
			return false;

		InventoryType::Id src_tab = icon.get_source_tab();
		int16_t src_slot = icon.get_source_slot();

		if (src_tab == InventoryType::Id::NONE || src_slot < 0)
			return false;

		int8_t inv_type = static_cast<int8_t>(src_tab);
		BotInvGivePacket(bot_inventory.char_id, inv_type, src_slot).dispatch();

		return true;
	}

	void UICharInfo::load_bot_icons()
	{
		bot_icons.clear();
		const auto& items = bot_inventory.get_tab(bot_sub_tab);
		int8_t inv_type = bot_inventory.get_inv_type(bot_sub_tab);

		for (size_t i = 0; i < items.size(); i++)
		{
			const auto& bi = items[i];
			int32_t item_prefix = bi.item_id / 10000;
			bool is_card = (item_prefix == 238 || item_prefix == 239);
			const Texture& texture = ItemData::get(bi.item_id).get_icon(is_card);
			int16_t count = (bot_sub_tab == 0 || bot_sub_tab == 4) ? -1 : bi.count;

			bot_icons[static_cast<int16_t>(i)] = std::make_unique<Icon>(
				std::make_unique<BotItemIcon>(bot_inventory.char_id, inv_type, bi.slot, bi.item_id, bi.count, bot_sub_tab),
				texture, count
			);
		}
	}

	Point<int16_t> UICharInfo::get_bot_slotpos(int16_t slot_index) const
	{
		div_t d = std::div(slot_index, BOT_COLS);
		int16_t start_y = BOT_GRID_Y - bot_scroll_offset;
		return Point<int16_t>(BOT_GRID_X + BOT_ICON_W * d.rem, start_y + BOT_ICON_H * d.quot);
	}

	int16_t UICharInfo::bot_slot_by_position(Point<int16_t> cursor_rel) const
	{
		int16_t start_y = BOT_GRID_Y - bot_scroll_offset;
		int16_t rx = cursor_rel.x() - BOT_GRID_X;
		int16_t ry = cursor_rel.y() - start_y;

		if (rx < 0 || ry < 0)
			return -1;

		int16_t col = rx / BOT_ICON_W;
		int16_t row = ry / BOT_ICON_H;

		if (col >= BOT_COLS || col < 0)
			return -1;

		return row * BOT_COLS + col;
	}

	Icon* UICharInfo::get_bot_icon(int16_t slot_index)
	{
		if (slot_index < 0)
			return nullptr;

		auto it = bot_icons.find(slot_index);
		if (it != bot_icons.end())
			return it->second.get();

		return nullptr;
	}

	void UICharInfo::remove_cursor()
	{
		UIDragElement::remove_cursor();
		UI::get().clear_tooltip(Tooltip::Parent::CHARINFO);
	}

	// BotItemIcon implementation
	UICharInfo::BotItemIcon::BotItemIcon(int32_t bid, int8_t it, int16_t s, int32_t iid, int16_t c, int8_t st)
		: bot_id(bid), inv_type(it), slot(s), item_id(iid), count(c), sub_tab(st) {}

	void UICharInfo::BotItemIcon::drop_on_stage() const
	{
		if (sub_tab == 4)
		{
			BotInvEquipPacket(bot_id, slot, static_cast<int16_t>(-slot)).dispatch();
		}
		else
		{
			BotInvTakePacket(bot_id, inv_type, slot).dispatch();
		}
	}

	void UICharInfo::BotItemIcon::drop_on_equips(EquipSlot::Id) const {}

	bool UICharInfo::BotItemIcon::drop_on_items(InventoryType::Id, EquipSlot::Id, int16_t, bool) const
	{
		if (sub_tab == 4)
		{
			BotInvEquipPacket(bot_id, slot, static_cast<int16_t>(-slot)).dispatch();
		}
		else
		{
			BotInvTakePacket(bot_id, inv_type, slot).dispatch();
		}
		return true;
	}

	void UICharInfo::BotItemIcon::drop_on_bindings(Point<int16_t>, bool) const {}
	void UICharInfo::BotItemIcon::set_count(int16_t c) { count = c; }
	Icon::IconType UICharInfo::BotItemIcon::get_type() { return Icon::IconType::ITEM; }
}
