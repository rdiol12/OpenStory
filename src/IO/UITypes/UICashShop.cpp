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
#include "UICashShop.h"

#include "../UI.h"
#include "../Window.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Handlers/CashShopHandlers.h"
#include "../../Net/Packets/CashShopPackets.h"
#include "../../Net/Packets/GameplayPackets.h"
#include "../../Net/Packets/LoginPackets.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include <fstream>

namespace ms
{
	// Debug helper — recursively dumps a single NX node (with children up to
	// max_depth) into ofs. Same shape as UIQuestHelper's dump_nx_node.
	static void dump_cs_node(std::ofstream& ofs, nl::node n, int depth, int max_depth = 3)
	{
		if (!n) return;

		for (int i = 0; i < depth; i++) ofs << "  ";
		ofs << n.name() << " [" << (int)n.data_type() << "]";

		switch (n.data_type())
		{
		case nl::node::type::integer: ofs << " = " << (int64_t)n; break;
		case nl::node::type::real:    ofs << " = " << (double)n; break;
		case nl::node::type::string:  ofs << " = \"" << n.get_string() << "\""; break;
		case nl::node::type::vector:  ofs << " = (" << n.x() << "," << n.y() << ")"; break;
		case nl::node::type::bitmap:
		{
			auto bmp = n.get_bitmap();
			if (bmp)
				ofs << " <bitmap " << bmp.width() << "x" << bmp.height() << ">";
			else
				ofs << " <bitmap (null)>";
			break;
		}
		default: break;
		}
		ofs << "  (" << n.size() << " children)\n";

		if (depth >= max_depth) return;
		for (auto child : n)
			dump_cs_node(ofs, child, depth + 1, max_depth);
	}

	// One-shot CashShopGL.img dump — reveals origins/vectors of every node
	// under the English cash-shop tree so button positions can be computed
	// from the data rather than guessed.
	static void dump_cashshop_nodes_once()
	{
		static bool done = false;
		if (done) return;
		done = true;

		std::ofstream ofs("cashshopgl_ui_dump.txt");
		if (!ofs) return;

		ofs << "=== CashShopGL.img (top-level children) ===\n";
		for (auto c : nl::nx::ui["CashShopGL.img"])
			ofs << "  " << c.name() << " (" << c.size() << ")\n";

		ofs << "\n=== CashShopGL.img / BaseFrame (depth 5) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["BaseFrame"], 0, 5);
		ofs << "\n=== CashShopGL.img / MainMenu (depth 5) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["MainMenu"], 0, 5);
		ofs << "\n=== CashShopGL.img / LeftMenu (depth 5) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["LeftMenu"], 0, 5);
		ofs << "\n=== CashShopGL.img / RightMenu (depth 5) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["RightMenu"], 0, 5);
		ofs << "\n=== CashShopGL.img / Popup (depth 5) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["Popup"], 0, 5);
		ofs << "\n=== CashShopGL.img / Effect (depth 4) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["Effect"], 0, 4);
		ofs << "\n=== CashShopGL.img / PicturePlate (depth 4) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["PicturePlate"], 0, 4);
		ofs << "\n=== CashShopGL.img / ToolTip (depth 4) ===\n";
		dump_cs_node(ofs, nl::nx::ui["CashShopGL.img"]["ToolTip"], 0, 4);

		ofs.close();
	}

	UICashShop::UICashShop() : preview_index(0), menu_index(1), promotion_index(0), mvp_grade(1), mvp_exp(0.07f), list_offset(0)
	{
		dump_cashshop_nodes_once();

		// Primary English cash-shop sprites live in CashShopGL.img. A few
		// elements (item search/effect/char/list) don't have CashShopGL
		// equivalents, so they fall back to CashShop.img's Korean nodes.
		nl::node CashShop = nl::nx::ui["CashShop.img"];
		nl::node CashShopGL = nl::nx::ui["CashShopGL.img"];

		nl::node Base = CashShop["Base"];
		nl::node BaseFrame = CashShopGL["BaseFrame"];
		nl::node backgrnd = BaseFrame["background"];
		nl::node Preview = Base["Preview"];
		nl::node CSItemSearch = BaseFrame["CSItemSearch"];
		nl::node CSList = CashShop["CSList"];
		nl::node MainItem = CashShopGL["MainMenu"]["MainItem"];
		nl::node Popup = CashShopGL["Popup"];

		sprites.emplace_back(backgrnd);

		// BestNew banner has Korean text baked in — omit. Leave dimensions
		// zeroed out (item_none fallback still works).
		BestNew_dim = Point<int16_t>();

		for (size_t i = 0; i < 3; i++)
			preview_sprites[i] = Preview[i];

		for (size_t i = 0; i < 3; i++)
			buttons[Buttons::BtPreview1 + i] = std::make_unique<TwoSpriteButton>(Base["Tab"]["Disable"][i], Base["Tab"]["Enable"][i], Point<int16_t>(957 + (i * 17), 46));

		buttons[Buttons::BtPreview1]->set_state(Button::State::PRESSED);

		// English header/exit buttons from CashShopGL/BaseFrame.
		// The bitmaps carry absolute 1024x768 positions via negative origin
		// vectors (e.g. BtExit origin = (-962,-10) → draws at (962,10)), so
		// pass Point(0,0) and let the origin place them on the canvas.
		buttons[Buttons::BtExit] = std::make_unique<MapleButton>(BaseFrame["BtExit"]);
		buttons[Buttons::BtHelp] = std::make_unique<MapleButton>(BaseFrame["BtHelp"]);
		buttons[Buttons::BtCoupon] = std::make_unique<MapleButton>(BaseFrame["BtCoupon"]);

		// Korean-only elements (CSTab/Tab menu banners, CSStatus/BtWish,
		// CSStatus/BtMileage, CSPromotionBanner, CSChar avatar buttons,
		// CSMVPBanner) are deliberately not instantiated — there are no
		// equivalents in CashShopGL.img that match this layout 1:1.

		Player& player = Stage::get().get_player();
		std::string pname = player.get_stats().get_name();
		std::string pjob = player.get_stats().get_jobname();

		job_label = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::SUPERNOVA, pjob);
		name_label = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::WHITE, pname);

		promotion_pos = Point<int16_t>();
		mvp_pos = Point<int16_t>();

		// English search bar under BaseFrame/CSItemSearch.
		sprites.emplace_back(CSItemSearch["background"]);

		// Charge NX numeric charset — fall back to CashShop/Base/Number.
		charge_charset = Charset(Base["Number"], Charset::Alignment::RIGHT);

		item_base = CSList["Base"];
		item_line = Base["line"];
		item_none = Base["noItem"];
		// Korean CSEffect labels omitted; item_labels stays empty so no
		// HOT/NEW/SALE sticker draws with Korean text.

		items.push_back({ 5220000, 20000001, Item::Label::HOT,		34000,	11 });
		items.push_back({ 5220000, 20000002, Item::Label::HOT,		34000,	11 });
		items.push_back({ 5220000, 20000003, Item::Label::HOT,		0,		0 });
		items.push_back({ 5220000, 20000004, Item::Label::HOT,		0,		0 });
		items.push_back({ 5220000, 20000005, Item::Label::HOT,		10000,	11 });
		items.push_back({ 5220000, 20000006, Item::Label::NEW,		0,		0 });
		items.push_back({ 5220000, 20000007, Item::Label::SALE,	7000,	0 });
		items.push_back({ 5220000, 20000008, Item::Label::NEW,		13440,	0 });
		items.push_back({ 5220000, 20000009, Item::Label::NEW,		7480,	0 });
		items.push_back({ 5220000, 20000010, Item::Label::NEW,		7480,	0 });
		items.push_back({ 5220000, 20000011, Item::Label::NEW,		7480,	0 });
		items.push_back({ 5220000, 20000012, Item::Label::NONE,	12000,	11 });
		items.push_back({ 5220000, 20000013, Item::Label::NONE,	22000,	11 });
		items.push_back({ 5220000, 20000014, Item::Label::NONE,	0,		0 });
		items.push_back({ 5220000, 20000015, Item::Label::NONE,	0,		0 });
		items.push_back({ 5220000, 20000016, Item::Label::MASTER,	0,		15 });

		for (size_t i = 0; i < MAX_ITEMS; i++)
		{
			div_t div = std::div(i, 7);

			// CashShopGL's MainItem/BtBuy is the English "Buy" button. Its
			// bitmap has origin (-81, -54) relative to its containing cell,
			// so subtract that offset so Point(..., ...) still addresses the
			// cell's top-left corner.
			buttons[Buttons::BtBuy + i] = std::make_unique<MapleButton>(MainItem["BtBuy"], Point<int16_t>(146 - 81, 523 - 54) + Point<int16_t>(124 * div.rem, 205 * div.quot));

			item_name[i] = Text(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::MINESHAFT);
			item_price[i] = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::GRAY);
			item_discount[i] = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::SILVERCHALICE);
			item_percent[i] = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::TORCHRED);
		}

		Point<int16_t> slider_pos = Point<int16_t>(1007, 372);

		list_slider = Slider(
			Slider::Type::THIN_MINESHAFT,
			Range<int16_t>(slider_pos.y(), slider_pos.y() + 381),
			slider_pos.x(),
			2,
			7,
			[&](bool upwards)
			{
				int16_t shift = upwards ? -7 : 7;
				bool above = list_offset >= 0;
				bool below = list_offset + shift < items.size();

				if (above && below)
				{
					list_offset += shift;

					update_items();
				}
			}
		);

		update_items();

		// === Sub-panel backgrounds (English — CashShopGL/Popup) ===
		// Popup/Buy/backgrnd, Popup/Cupon/backgrnd, Popup/Gift/backgrnd,
		// Popup/Notice/backgrnd are the English popup chrome. Wishlist and
		// Search have no dedicated Popup entry, so leave those empty.
		gift_bg = Texture(Popup["Gift"]["backgrnd"]);
		coupon_bg = Texture(Popup["Cupon"]["backgrnd"]);
		purchase_bg = Texture(Popup["Buy"]["backgrnd"]);
		active_subpanel = SUBPANEL_NONE;

		dimension = Texture(backgrnd).get_dimensions();
	}

	void UICashShop::draw(float inter) const
	{
		preview_sprites[preview_index].draw(position + Point<int16_t>(644, 65), inter);

		UIElement::draw_sprites(inter);

		// Korean menu_tabs / promotion_sprites / mvp_sprites / charge_charset
		// draws removed — those elements have no English equivalent.

		Point<int16_t> label_pos = position + Point<int16_t>(4, 3);
		job_label.draw(label_pos);

		size_t length = job_label.width();
		name_label.draw(label_pos + Point<int16_t>(length + 10, 0));

		if (items.size() > 0)
			item_line.draw(position + Point<int16_t>(139, 566), inter);
		else
			item_none.draw(position + Point<int16_t>(137, 372) - item_none.get_dimensions() / 2 + Point<int16_t>(0, list_slider.getvertical().length() / 2), inter);

		for (size_t i = 0; i < MAX_ITEMS; i++)
		{
			int16_t index = i + list_offset;

			if (index < items.size())
			{
				div_t div = std::div(i, 7);
				Item item = items[index];

				item_base.draw(position + Point<int16_t>(137, 372) + Point<int16_t>(124 * div.rem, 205 * div.quot), inter);
				item.draw(DrawArgument(position + Point<int16_t>(164, 473) + Point<int16_t>(124 * div.rem, 205 * div.quot), 2.0f, 2.0f));

				// Korean CSEffect labels omitted.

				item_name[i].draw(position + Point<int16_t>(192, 480) + Point<int16_t>(124 * div.rem, 205 * div.quot));

				if (item_discount[i].get_text() == "")
				{
					item_price[i].draw(position + Point<int16_t>(195, 499) + Point<int16_t>(124 * div.rem, 205 * div.quot));
				}
				else
				{
					item_price[i].draw(position + Point<int16_t>(196, 506) + Point<int16_t>(124 * div.rem, 205 * div.quot));

					item_discount[i].draw(position + Point<int16_t>(185, 495) + Point<int16_t>(124 * div.rem, 205 * div.quot));
					item_percent[i].draw(position + Point<int16_t>(198 + (item_discount[i].width() / 2), 495) + Point<int16_t>(124 * div.rem, 205 * div.quot));
				}
			}
		}

		list_slider.draw(position);

		UIElement::draw_buttons(inter);

		// Draw active sub-panel (English popups only).
		switch (active_subpanel)
		{
		case SUBPANEL_GIFT:
			if (gift_bg.is_valid())
				gift_bg.draw(DrawArgument(position + Point<int16_t>(200, 50)));
			break;
		case SUBPANEL_COUPON:
			if (coupon_bg.is_valid())
				coupon_bg.draw(DrawArgument(position + Point<int16_t>(200, 50)));
			break;
		case SUBPANEL_PURCHASE:
			if (purchase_bg.is_valid())
				purchase_bg.draw(DrawArgument(position + Point<int16_t>(200, 50)));
			break;
		default:
			break;
		}
	}

	void UICashShop::update()
	{
		UIElement::update();
	}

	Button::State UICashShop::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BtPreview1:
		case Buttons::BtPreview2:
		case Buttons::BtPreview3:
			buttons[preview_index]->set_state(Button::State::NORMAL);

			preview_index = buttonid;
			return Button::State::PRESSED;
		case Buttons::BtExit:
		{
			uint16_t width = Setting<Width>::get().load();
			uint16_t height = Setting<Height>::get().load();

			// Restore the user's pre-cashshop UI scale (transition() forced
			// it to 1.0 so the 1024x768 cash shop UI fit the window exactly).
			Constants::Constants::get().set_ui_scale(get_pre_cashshop_ui_scale());
			Constants::Constants::get().set_viewwidth(width);
			Constants::Constants::get().set_viewheight(height);

			float fadestep = 0.025f;

			Window::get().fadeout(
				fadestep,
				[]()
				{
					GraphicsGL::get().clear();
					ChangeMapPacket().dispatch();
				}
			);

			GraphicsGL::get().lock();
			Stage::get().clear();
			Timer::get().start();

			return Button::State::NORMAL;
		}
		case Buttons::BtNext:
		{
			size_t size = promotion_sprites.size() - 1;

			promotion_index++;

			if (promotion_index > size)
				promotion_index = 0;

			return Button::State::NORMAL;
		}
		case Buttons::BtPrev:
		{
			size_t size = promotion_sprites.size() - 1;

			promotion_index--;

			if (promotion_index < 0)
				promotion_index = size;

			return Button::State::NORMAL;
		}
		case Buttons::BtChargeNX:
		{
			std::string url = Configuration::get().get_chargenx();

#ifdef _WIN32
			ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif

			return Button::State::NORMAL;
		}
		case Buttons::BtWish:
			active_subpanel = (active_subpanel == SUBPANEL_WISHLIST) ? SUBPANEL_NONE : SUBPANEL_WISHLIST;
			return Button::State::NORMAL;
		case Buttons::BtCoupon:
			active_subpanel = (active_subpanel == SUBPANEL_COUPON) ? SUBPANEL_NONE : SUBPANEL_COUPON;
			return Button::State::NORMAL;
		case Buttons::BtInventory:
			active_subpanel = (active_subpanel == SUBPANEL_INVENTORY) ? SUBPANEL_NONE : SUBPANEL_INVENTORY;
			return Button::State::NORMAL;
		default:
			break;
		}

		if (buttonid >= Buttons::BtBuy)
		{
			int16_t index = buttonid - Buttons::BtBuy + list_offset;

			if (index >= 0 && index < items.size())
			{
				Item item = items[index];

				// Currency type 1 = NX Credit (standard purchase)
				int8_t currency = 1;
				BuyCashItemPacket(currency, item.sn).dispatch();
			}

			return Button::State::NORMAL;
		}

		return Button::State::DISABLED;
	}

	Cursor::State UICashShop::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursor_relative = cursorpos - position;

		if (list_slider.isenabled())
		{
			Cursor::State state = list_slider.send_cursor(cursor_relative, clicked);

			if (state != Cursor::State::IDLE)
				return state;
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	UIElement::Type UICashShop::get_type() const
	{
		return TYPE;
	}

	void UICashShop::exit_cashshop()
	{
		UI& ui = UI::get();
		ui.change_state(UI::State::GAME);

		// Restore UI scale + physical window size that were overridden in
		// SetCashShopHandler::transition(). Without this the game world would
		// remain confined to the 1024x768 cash-shop window at scale 1.0.
		uint16_t width = Setting<Width>::get().load();
		uint16_t height = Setting<Height>::get().load();
		Constants::Constants::get().set_ui_scale(get_pre_cashshop_ui_scale());
		Constants::Constants::get().set_viewwidth(width);
		Constants::Constants::get().set_viewheight(height);

		Stage& stage = Stage::get();
		Player& player = stage.get_player();

		PlayerLoginPacket(player.get_oid()).dispatch();

		int32_t mapid = player.get_stats().get_mapid();
		uint8_t portalid = player.get_stats().get_portal();

		stage.load(mapid, portalid);
		stage.transfer_player();

		ui.enable();
		Timer::get().start();
		GraphicsGL::get().unlock();
	}

	void UICashShop::update_items()
	{
		for (size_t i = 0; i < MAX_ITEMS; i++)
		{
			int16_t index = i + list_offset;
			bool found_item = index < items.size();

			buttons[Buttons::BtBuy + i]->set_active(found_item);

			std::string name = "";
			std::string price_text = "";
			std::string discount_text = "";
			std::string percent_text = "";

			if (found_item)
			{
				Item item = items[index];

				name = item.get_name();

				int32_t price = item.get_price();
				price_text = std::to_string(price);

				if (item.discount_price > 0 && price > 0)
				{
					discount_text = price_text;

					uint32_t discount = item.discount_price;
					price_text = std::to_string(discount);

					float_t percent = (float)discount / price;
					std::string percent_str = std::to_string(percent);
					percent_text = "(" + percent_str.substr(2, 1) + "%)";
				}

				string_format::split_number(price_text);
				string_format::split_number(discount_text);

				price_text += " NX";

				if (discount_text != "")
					discount_text += " NX";

				if (item.count > 0)
					price_text += "(" + std::to_string(item.count) + ")";
			}

			item_name[i].change_text(name);
			item_price[i].change_text(price_text);
			item_discount[i].change_text(discount_text);
			item_percent[i].change_text(percent_text);

			string_format::format_with_ellipsis(item_name[i], 92);
		}
	}
}