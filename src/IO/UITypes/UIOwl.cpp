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
#include "UIOwl.h"

#include "UIStatusMessenger.h"
#include "UINotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"
#include "../../Configuration.h"
#include "../../Data/ItemData.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Net/Packets/TradePackets.h"

#include <algorithm>
#include <cctype>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		std::string lower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}

		const std::vector<std::pair<std::string, int32_t>>& name_index()
		{
			static std::vector<std::pair<std::string, int32_t>> index;
			static bool built = false;

			if (built)
				return index;

			built = true;

			auto add_flat = [&](nl::node cat)
			{
				for (const nl::node& item : cat)
				{
					std::string name = item["name"];
					if (!name.empty())
						index.emplace_back(lower(name), std::stoi(item.name()));
				}
			};

			nl::node str = nl::nx::string;
			add_flat(str["Consume.img"]);
			add_flat(str["Ins.img"]);
			add_flat(str["Cash.img"]);
			add_flat(str["Pet.img"]);
			add_flat(str["Etc.img"]["Etc"]);

			for (const nl::node& cat : str["Eqp.img"]["Eqp"])
				add_flat(cat);

			return index;
		}
	}

	namespace
	{
		const std::vector<std::string>& category_names()
		{
			static std::vector<std::string> names;

			if (names.empty())
			{
				for (const nl::node& cat : nl::nx::string["Eqp.img"]["Eqp"])
					names.push_back(cat.name());

				names.push_back("Use");
				names.push_back("Setup");
				names.push_back("Etc");
				names.push_back("Cash");
				names.push_back("Pet");
			}

			return names;
		}

		std::vector<std::pair<std::string, int32_t>> category_items(const std::string& cat)
		{
			std::vector<std::pair<std::string, int32_t>> items;
			nl::node str = nl::nx::string;
			nl::node src;

			if (cat == "Use") src = str["Consume.img"];
			else if (cat == "Setup") src = str["Ins.img"];
			else if (cat == "Etc") src = str["Etc.img"]["Etc"];
			else if (cat == "Cash") src = str["Cash.img"];
			else if (cat == "Pet") src = str["Pet.img"];
			else src = str["Eqp.img"]["Eqp"][cat];

			for (const nl::node& item : src)
			{
				std::string name = item["name"];

				if (!name.empty())
					items.emplace_back(name, std::stoi(item.name()));

				if (items.size() >= 260)
					break;
			}

			return items;
		}
	}

	UIOwl::UIOwl(int16_t slot, int32_t itemid) : UIDragElement<PosOWL>(),
		owl_slot(slot), owl_itemid(itemid), results_mode(false),
		result_itemid(0), selected(-1), result_offset(0)
	{
		nl::node src = nl::nx::ui["UIWindow.img"]["itemSearch"];

		search_bg = src["backgrnd"];
		result_bg = src["resultback"];
		panel_bg = src["searchback"];
		check_off = nl::nx::ui["Basic.img"]["CheckBox"]["0"];
		check_on = nl::nx::ui["Basic.img"]["CheckBox"]["1"];
		info_btn_tex = src["BtInfo"];
		go_btn_tex = src["BtLink"]["normal"]["0"];
		go_btn_over = src["BtLink"]["mouseOver"]["0"];
		go_btn_pressed = src["BtLink"]["pressed"]["0"];
		search_dim = search_bg.get_dimensions();
		result_dim = result_bg.get_dimensions();

		buttons[Buttons::BT_SEARCH] = std::make_unique<MapleButton>(src["BtSearch"], Point<int16_t>(178, 77));
		buttons[Buttons::BT_RETRY] = std::make_unique<MapleButton>(src["BtRetry"], Point<int16_t>(160, 403));
		buttons[Buttons::BT_GO] = std::make_unique<MapleButton>(src["BtLink"], Point<int16_t>(232, 403));
		buttons[Buttons::BT_CANCEL] = std::make_unique<MapleButton>(src["BtCancel"], Point<int16_t>(300, 403));

		buttons[Buttons::BT_PAGE_LEFT] = std::make_unique<MapleButton>(src["BtLeft"], Point<int16_t>(414, 24));
		buttons[Buttons::BT_PAGE_RIGHT] = std::make_unique<MapleButton>(src["BtRight"], Point<int16_t>(430, 24));
		buttons[Buttons::BT_SEARCH2] = std::make_unique<MapleButton>(src["BtSearch2"], Point<int16_t>(381, 248));
		buttons[Buttons::BT_TOP10_GO] = std::make_unique<MapleButton>(src["BtGo"], Point<int16_t>(196, 218));
		buttons[Buttons::BT_CATEGORY_GO] = std::make_unique<MapleButton>(src["BtGo"], Point<int16_t>(196, 243));
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(nl::nx::ui["Basic.img"]["BtClose3"], Point<int16_t>(230, 5));
		buttons[Buttons::BT_PANEL_CLOSE] = std::make_unique<MapleButton>(nl::nx::ui["Basic.img"]["BtHide"], Point<int16_t>(430, 6));
		buttons[Buttons::BT_PANEL_CLOSE]->set_active(false);
		buttons[Buttons::BT_OK] = std::make_unique<MapleButton>(src["BtOK"], Point<int16_t>(190, 397));
		buttons[Buttons::BT_OK]->set_active(false);

		buttons[Buttons::BT_RETRY]->set_active(false);
		buttons[Buttons::BT_GO]->set_active(false);
		buttons[Buttons::BT_CANCEL]->set_active(false);
		buttons[Buttons::BT_PAGE_LEFT]->set_active(false);
		buttons[Buttons::BT_PAGE_RIGHT]->set_active(false);
		buttons[Buttons::BT_SEARCH2]->set_active(false);

		search_field = Textfield(
			Text::A11M, Text::LEFT, Color::Name::BLACK,
			Rectangle<int16_t>(
				Point<int16_t>(22, 73),
				Point<int16_t>(168, 89)
			),
			40
		);
		search_field.set_enter_callback(
			[this](std::string) { do_search(); }
		);
		search_field.set_state(Textfield::State::FOCUSED);

		rowtext = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
		titletext = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK);

		dimension = search_dim;
		dragarea = Point<int16_t>(search_dim.x(), 20);

		OwlActionPacket().dispatch();
	}

	void UIOwl::draw(float inter) const
	{
		if (!results_mode)
		{
			search_bg.draw(position);
			search_field.draw(position);

			const auto& rows = panel_rows;

			if (!rows.empty())
			{
				panel_bg.draw(position + Point<int16_t>(248, 0));

				const Texture& cb = sort_by_price ? check_on : check_off;
				cb.draw(position + Point<int16_t>(262, 251));

				int16_t y = 38;
				int16_t shown = 0;

				for (size_t i = static_cast<size_t>(page) * 13; i < rows.size(); i++)
				{
					if (shown >= 13)
						break;

					bool marked = static_cast<int16_t>(i) == picked || static_cast<int16_t>(i) == hovered;

					if (marked)
					{
						ColorBox sel(190, 15, Color::Name::BLUE, 0.75f);
						sel.draw(DrawArgument(position + Point<int16_t>(254, y)));
					}

					rowtext.change_color(marked ? Color::Name::WHITE : Color::Name::BLACK);
					rowtext.change_text(rows[i].first);
					rowtext.draw(position + Point<int16_t>(262, y));
					y += 16;
					shown++;
				}

				int16_t pages = static_cast<int16_t>((rows.size() + 12) / 13);
				rowtext.change_color(Color::Name::BLACK);
				rowtext.change_text(std::to_string(page + 1) + "/" + std::to_string(pages));
				rowtext.draw(position + Point<int16_t>(384, 24));
			}
		}
		else
		{
			result_bg.draw(position);

			const ItemData& data = ItemData::get(result_itemid);
			std::string iname = data.is_valid() ? data.get_name() : ("#" + std::to_string(result_itemid));

			rowtext.change_color(Color::Name::BLACK);
			rowtext.change_text("Search results for ");
			rowtext.draw(position + Point<int16_t>(26, 81));
			int16_t adv = rowtext.width();
			rowtext.change_color(Color::Name::RED);
			rowtext.change_text(iname);
			rowtext.draw(position + Point<int16_t>(26 + adv, 81));
			int16_t adv2 = rowtext.width() + 5;
			rowtext.change_color(Color::Name::BLACK);
			rowtext.change_text("that you entered.");
			rowtext.draw(position + Point<int16_t>(26 + adv + adv2, 81));

			rowtext.change_text("Total of ");
			rowtext.draw(position + Point<int16_t>(26, 97));
			adv = rowtext.width();
			rowtext.change_color(Color::Name::RED);
			rowtext.change_text(std::to_string(results.size()));
			rowtext.draw(position + Point<int16_t>(26 + adv, 97));
			adv2 = rowtext.width() + 5;
			rowtext.change_color(Color::Name::BLACK);
			rowtext.change_text("results available.");
			rowtext.draw(position + Point<int16_t>(26 + adv + adv2, 97));

			rowtext.change_color(Color::Name::RED);
			rowtext.change_text("You may not use \"GO\" if the store is either on a different channel or closed.");
			rowtext.draw(position + Point<int16_t>(26, 113));

			int16_t y = 174;
			int16_t shown = 0;

			for (size_t i = static_cast<size_t>(result_offset); i < results.size(); i++)
			{
				if (shown >= 8)
					break;

				const Result& r = results[i];

				rowtext.change_color(Color::Name::BLACK);
				rowtext.change_text(r.owner.size() > 9 ? r.owner.substr(0, 8) + ".." : r.owner);
				rowtext.draw(position + Point<int16_t>(16, y));
				rowtext.change_text(r.desc.size() > 11 ? r.desc.substr(0, 10) + ".." : r.desc);
				rowtext.draw(position + Point<int16_t>(96, y));
				rowtext.change_text(std::to_string(r.bundles));
				rowtext.draw(position + Point<int16_t>(189, y));
				rowtext.change_color(Color::Name::ORANGE);
				rowtext.change_text(std::to_string(r.price));
				rowtext.draw(position + Point<int16_t>(224, y));
				rowtext.change_color(Color::Name::BLACK);
				rowtext.change_text(std::to_string(r.perbundle));
				rowtext.draw(position + Point<int16_t>(277, y));

				info_btn_tex.draw(position + Point<int16_t>(308, y));

				if (r.channel == Configuration::get().get_channelid())
				{
					const Texture& go = static_cast<int16_t>(i) == go_hover
						? (go_btn_over.is_valid() ? go_btn_over : go_btn_tex)
						: go_btn_tex;
					go.draw(position + Point<int16_t>(368, y - 1));
				}
				else
				{
					rowtext.change_color(Color::Name::RED);
					rowtext.change_text("Ch. " + std::to_string(r.channel + 1));
					rowtext.draw(position + Point<int16_t>(378, y));
				}

				y += 23;
				shown++;
			}

			if (results.empty())
			{
				rowtext.change_color(Color::Name::RED);
				rowtext.change_text("No shops are selling this item right now.");
				rowtext.draw(position + Point<int16_t>(80, 250));
			}

			int16_t pages = std::max<int16_t>(1, static_cast<int16_t>((results.size() + 7) / 8));
			rowtext.change_color(Color::Name::BLACK);
			rowtext.change_text("[" + std::to_string(result_offset / 8 + 1) + "/" + std::to_string(pages) + "]");
			rowtext.draw(position + Point<int16_t>(205, 367));
		}

		UIElement::draw_buttons(inter);
	}

	void UIOwl::update()
	{
		UIElement::update();
		search_field.update(position);

		bool panel_open = !results_mode && !panel_rows.empty();
		dimension = results_mode ? result_dim
			: Point<int16_t>(panel_open ? search_dim.x() + 205 : search_dim.x(), search_dim.y());

		if (!results_mode)
		{
			buttons[Buttons::BT_PAGE_LEFT]->set_position(Point<int16_t>(414, 24));
			buttons[Buttons::BT_PAGE_RIGHT]->set_position(Point<int16_t>(430, 24));
			buttons[Buttons::BT_PAGE_LEFT]->set_active(panel_open);
			buttons[Buttons::BT_PAGE_RIGHT]->set_active(panel_open);
			buttons[Buttons::BT_SEARCH2]->set_active(panel_open && !panel_cats);
			buttons[Buttons::BT_TOP10_GO]->set_active(!results_mode);
			buttons[Buttons::BT_CATEGORY_GO]->set_active(!results_mode);
			buttons[Buttons::BT_PANEL_CLOSE]->set_active(panel_open);
		}

		std::string text = search_field.get_text();

		if (text != last_input)
		{
			last_input = text;
			suggestions.clear();

			if (text.size() >= 2)
			{
				std::string needle = lower(text);

				for (const auto& entry : name_index())
				{
					if (entry.first.find(needle) != std::string::npos)
					{
						suggestions.emplace_back(entry.first, entry.second);

						if (suggestions.size() >= 65)
							break;
					}
				}
			}

			panel_rows = suggestions;
			panel_cats = false;
			page = 0;
			picked = -1;
		}
	}

	Cursor::State UIOwl::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> off = cursorpos - position;
		hovered = -1;
		go_hover = -1;

		if (!results_mode)
		{
			if (search_field.get_state() == Textfield::State::NORMAL)
			{
				Cursor::State fstate = search_field.send_cursor(cursorpos, clicked);

				if (fstate != Cursor::State::IDLE)
					return fstate;
			}
			else if (clicked && search_field.get_state() != Textfield::State::FOCUSED
				&& Rectangle<int16_t>(Point<int16_t>(18, 82), Point<int16_t>(172, 106)).contains(off))
			{
				search_field.set_state(Textfield::State::FOCUSED);
				return Cursor::State::CLICKING;
			}

			const auto& rows = panel_rows;

			if (!rows.empty()
				&& off.x() >= 252 && off.x() <= 448 && off.y() >= 36 && off.y() < 36 + 16 * 13)
			{
				size_t idx = static_cast<size_t>(page) * 13 + (off.y() - 36) / 16;

				if (idx < rows.size())
				{
					hovered = static_cast<int16_t>(idx);

					if (!panel_cats)
						UI::get().show_item(Tooltip::Parent::SHOP, rows[idx].second);

					if (clicked)
					{
						if (panel_cats)
						{
							panel_rows = category_items(rows[idx].first);
							panel_cats = false;
							page = 0;
							picked = -1;
						}
						else
						{
							picked = static_cast<int16_t>(idx);
							search_field.change_text(rows[idx].first);
							last_input = rows[idx].first;
						}

						return Cursor::State::CLICKING;
					}

					return Cursor::State::CANCLICK;
				}
			}

			UI::get().clear_tooltip(Tooltip::Parent::SHOP);

			if (clicked && !rows.empty()
				&& off.x() >= 258 && off.x() <= 276 && off.y() >= 249 && off.y() <= 266)
			{
				sort_by_price = !sort_by_price;
				return Cursor::State::CLICKING;
			}

			if (clicked && off.x() >= 10 && off.x() <= 238 && off.y() >= 230 && off.y() <= 248)
			{
				top10_open = !top10_open;
				return Cursor::State::CLICKING;
			}
		}
		else
		{
			if (off.y() >= 172 && off.y() < 172 + 23 * 8)
			{
				int16_t row = (off.y() - 172) / 23 + result_offset;

				if (row < static_cast<int16_t>(results.size()))
				{
					int16_t rel_y = 172 + ((off.y() - 172) / 23) * 23;

					if (off.x() >= 308 && off.x() <= 365 && off.y() - rel_y <= 16)
					{
						UI::get().show_item(Tooltip::Parent::SHOP, result_itemid);
						return Cursor::State::CANCLICK;
					}

					if (off.x() >= 368 && off.x() <= 427 && off.y() - rel_y <= 18
						&& results[row].channel == Configuration::get().get_channelid())
					{
						go_hover = row;

						if (clicked)
						{
							OwlWarpPacket(results[row].ownerid, results[row].mapid).dispatch();
							return Cursor::State::CLICKING;
						}

						return Cursor::State::CANCLICK;
					}
				}
			}

			UI::get().clear_tooltip(Tooltip::Parent::SHOP);
		}

		return UIDragElement::send_cursor(clicked, cursorpos);
	}

	void UIOwl::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			deactivate();
	}

	void UIOwl::doubleclick(Point<int16_t> cursorpos)
	{
		if (!results_mode)
			return;

		Point<int16_t> off = cursorpos - position;

		if (off.x() >= 14 && off.x() <= 424 && off.y() >= 172 && off.y() < 172 + 23 * 8)
		{
			int16_t row = (off.y() - 172) / 23 + result_offset;

			if (row < static_cast<int16_t>(results.size()))
			{
				selected = row;
				OwlWarpPacket(results[row].ownerid, results[row].mapid).dispatch();
			}
		}
	}

	UIElement::Type UIOwl::get_type() const
	{
		return TYPE;
	}

	void UIOwl::set_top10(const std::vector<int32_t>& itemids)
	{
		top10.clear();

		for (int32_t id : itemids)
		{
			const ItemData& data = ItemData::get(id);

			if (data.is_valid())
				top10.emplace_back(data.get_name(), id);
		}
	}

	void UIOwl::show_results(int32_t itemid, InPacket& recv)
	{
		result_itemid = itemid;
		results.clear();
		selected = -1;
		result_offset = 0;

		int32_t count = recv.read_int();

		for (int32_t i = 0; i < count; i++)
		{
			Result r;
			r.owner = recv.read_string();
			r.mapid = recv.read_int();
			r.desc = recv.read_string();
			r.bundles = recv.read_int();
			r.perbundle = recv.read_int();
			r.price = recv.read_int();
			r.ownerid = recv.read_int();
			r.channel = recv.read_byte();
			results.push_back(std::move(r));
		}

		if (sort_by_price)
			std::sort(results.begin(), results.end(),
				[](const Result& a, const Result& b) { return a.price < b.price; });

		results_mode = true;
		search_field.set_state(Textfield::State::DISABLED);
		dimension = result_dim;
		dragarea = Point<int16_t>(result_dim.x(), 20);

		buttons[Buttons::BT_SEARCH]->set_active(false);
		buttons[Buttons::BT_SEARCH2]->set_active(false);
		buttons[Buttons::BT_TOP10_GO]->set_active(false);
		buttons[Buttons::BT_CATEGORY_GO]->set_active(false);
		buttons[Buttons::BT_PANEL_CLOSE]->set_active(false);
		buttons[Buttons::BT_CLOSE]->set_active(false);
		buttons[Buttons::BT_OK]->set_active(true);
		buttons[Buttons::BT_PAGE_LEFT]->set_position(Point<int16_t>(155, 369));
		buttons[Buttons::BT_PAGE_RIGHT]->set_position(Point<int16_t>(263, 369));
		buttons[Buttons::BT_PAGE_LEFT]->set_active(true);
		buttons[Buttons::BT_PAGE_RIGHT]->set_active(true);
	}

	void UIOwl::do_search()
	{
		std::string text = search_field.get_text();

		if (text.empty())
			return;

		int32_t target = 0;
		bool digits = std::all_of(text.begin(), text.end(),
			[](unsigned char c) { return std::isdigit(c); });

		if (digits)
		{
			target = std::stoi(text);
		}
		else
		{
			std::string needle = lower(text);
			const auto& index = name_index();

			for (const auto& entry : index)
			{
				if (entry.first == needle)
				{
					target = entry.second;
					break;
				}
			}

			if (target == 0)
			{
				for (const auto& entry : index)
				{
					if (entry.first.find(needle) != std::string::npos)
					{
						target = entry.second;
						break;
					}
				}
			}
		}

		if (target == 0)
		{
			if (auto messenger = UI::get().get_element<UIStatusMessenger>())
				messenger->show_status(Color::Name::RED, "No item matching '" + text + "' was found.");

			return;
		}

		int16_t slot = owl_slot;
		int32_t owl = owl_itemid;
		std::string name = ItemData::get(target).get_name();

		UI::get().emplace<UIYesNo>(
			"Would you like to search for [" + name + "]? The Owl of Minerva will disappear if there's a search result after pressing OK.",
			[slot, owl, target](bool yes)
			{
				if (yes)
					UseOwlSearchPacket(slot, owl, target).dispatch();
			},
			Text::Alignment::CENTER
		);
	}

	Button::State UIOwl::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_SEARCH:
			do_search();
			break;
		case Buttons::BT_RETRY:
			results_mode = false;
			results.clear();
			selected = -1;
			dimension = search_dim;
			dragarea = Point<int16_t>(search_dim.x(), 20);
			buttons[Buttons::BT_SEARCH]->set_active(true);
			buttons[Buttons::BT_RETRY]->set_active(false);
			buttons[Buttons::BT_GO]->set_active(false);
			buttons[Buttons::BT_CANCEL]->set_active(false);
			search_field.set_state(Textfield::State::FOCUSED);
			break;
		case Buttons::BT_GO:
			if (selected >= 0 && selected < static_cast<int16_t>(results.size()))
				OwlWarpPacket(results[selected].ownerid, results[selected].mapid).dispatch();
			break;
		case Buttons::BT_CANCEL:
		case Buttons::BT_CLOSE:
		case Buttons::BT_OK:
			deactivate();
			break;
		case Buttons::BT_PANEL_CLOSE:
			panel_rows.clear();
			suggestions.clear();
			panel_cats = false;
			picked = -1;
			page = 0;
			break;
		case Buttons::BT_PAGE_LEFT:
			if (results_mode)
			{
				if (result_offset >= 8)
					result_offset -= 8;
			}
			else if (page > 0)
				page--;
			break;
		case Buttons::BT_PAGE_RIGHT:
			if (results_mode)
			{
				if (result_offset + 8 < static_cast<int16_t>(results.size()))
					result_offset += 8;
			}
			else
			{
				int16_t pages = static_cast<int16_t>((panel_rows.size() + 12) / 13);
				if (page + 1 < pages)
					page++;
			}
			break;
		case Buttons::BT_SEARCH2:
		{
			if (!panel_cats && picked >= 0 && picked < static_cast<int16_t>(panel_rows.size()))
			{
				search_field.change_text(panel_rows[picked].first);
				last_input = panel_rows[picked].first;
			}

			do_search();
			break;
		}
		case Buttons::BT_TOP10_GO:
			panel_rows = top10;
			panel_cats = false;
			page = 0;
			picked = -1;
			break;
		case Buttons::BT_CATEGORY_GO:
		{
			panel_rows.clear();

			for (const std::string& name : category_names())
				panel_rows.emplace_back(name, 0);

			panel_cats = true;
			page = 0;
			picked = -1;
			break;
		}
		}

		return Button::State::NORMAL;
	}
}
