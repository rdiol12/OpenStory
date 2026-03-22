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
#include "UIOptionMenu.h"

#include "../KeyAction.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIOptionMenu::UIOptionMenu() : UIDragElement<PosOPTIONMENU>(Point<int16_t>(200, 20)), selected_tab(0), nx_missing(false)
	{
		nl::node OptionMenu = nl::nx::ui["StatusBar2.img"]["OptionMenu"];
		nl::node backgrnd = OptionMenu["backgrnd"];

		nx_missing = !OptionMenu;

		if (!nx_missing)
		{
			sprites.emplace_back(backgrnd);
			sprites.emplace_back(OptionMenu["backgrnd2"]);

			nl::node graphic = OptionMenu["graphic"];

			tab_background[Buttons::TAB0] = graphic["layer:backgrnd"];
			tab_background[Buttons::TAB1] = OptionMenu["sound"]["layer:backgrnd"];
			tab_background[Buttons::TAB2] = OptionMenu["game"]["layer:backgrnd"];
			tab_background[Buttons::TAB3] = OptionMenu["invite"]["layer:backgrnd"];
			tab_background[Buttons::TAB4] = OptionMenu["screenshot"]["layer:backgrnd"];

			buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(OptionMenu["button:Cancel"]);
			buttons[Buttons::OK] = std::make_unique<MapleButton>(OptionMenu["button:OK"]);
			buttons[Buttons::UIRESET] = std::make_unique<MapleButton>(OptionMenu["button:UIReset"]);

			nl::node tab = OptionMenu["tab"];
			nl::node tab_disabled = tab["disabled"];
			nl::node tab_enabled = tab["enabled"];

			for (size_t i = Buttons::TAB0; i < Buttons::CANCEL; i++)
				buttons[i] = std::make_unique<TwoSpriteButton>(tab_disabled[i], tab_enabled[i]);

			std::string sButtonUOL = graphic["combo:resolution"]["sButtonUOL"].get_string();
			MapleComboBox::Type type = MapleComboBox::Type::DEFAULT;

			if (!sButtonUOL.empty())
			{
				std::string ctype = std::string(1, sButtonUOL.back());
				type = static_cast<MapleComboBox::Type>(std::stoi(ctype));
			}

			int64_t combobox_width = graphic["combo:resolution"]["boxWidth"].get_integer();

			if (combobox_width <= 0)
				combobox_width = 150;

			Point<int16_t> lt = Point<int16_t>(graphic["combo:resolution"]["lt"]);

			Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

			if (bg_dimensions.x() <= 0 || bg_dimensions.y() <= 0)
				bg_dimensions = Point<int16_t>(300, 300);

			dimension = bg_dimensions;
			dragarea = Point<int16_t>(bg_dimensions.x(), 20);
		}
		else
		{
			// Fallback UI when NX data is missing
			int16_t menu_w = 280;
			int16_t menu_h = 200;

			background = ColorBox(menu_w, menu_h, Color::Name::BLACK, 0.85f);
			title_bar = ColorBox(menu_w, 24, Color::Name::EMPEROR, 0.95f);
			title_text = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Game Settings");
			resolution_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "Resolution:");
			ok_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "OK");
			cancel_label = Text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE, "Cancel");

			dimension = Point<int16_t>(menu_w, menu_h);
			dragarea = Point<int16_t>(menu_w, 24);

			// Center on screen
			int16_t screen_w = Constants::Constants::get().get_viewwidth();
			int16_t screen_h = Constants::Constants::get().get_viewheight();
			position = Point<int16_t>((screen_w - menu_w) / 2, (screen_h - menu_h) / 2);
		}

		// Resolution options (shared)
		std::vector<std::string> resolutions =
		{
			"800 x 600 ( 4 : 3 )",
			"1024 x 768 ( 4 : 3 )",
			"1280 x 720 ( 16 : 9 )",
			"1366 x 768 ( 16 : 9 )",
			"1920 x 1080 ( 16 : 9 )"
		};

		int16_t max_width = Configuration::get().get_max_width();
		int16_t max_height = Configuration::get().get_max_height();

		if (max_width >= 1920 && max_height >= 1200)
			resolutions.emplace_back("1920 x 1200 ( 16 : 10 ) - Beta");

		uint16_t default_option = 0;
		int16_t screen_width = Constants::Constants::get().get_viewwidth();
		int16_t screen_height = Constants::Constants::get().get_viewheight();

		switch (screen_width)
		{
		case 800:
			default_option = 0;
			break;
		case 1024:
			default_option = 1;
			break;
		case 1280:
			default_option = 2;
			break;
		case 1366:
			default_option = 3;
			break;
		case 1920:
			switch (screen_height)
			{
			case 1080:
				default_option = 4;
				break;
			case 1200:
				default_option = 5;
				break;
			}

			break;
		}

		MapleComboBox::Type ctype = MapleComboBox::Type::DEFAULT;
		Point<int16_t> lt = nx_missing ? Point<int16_t>(10, 55) : Point<int16_t>(0, 0);
		int64_t combobox_width = 150;

		buttons[Buttons::SELECT_RES] = std::make_unique<MapleComboBox>(ctype, resolutions, default_option, position, lt, combobox_width);

		if (nx_missing)
			selected_tab = Buttons::TAB0;
		else
			change_tab(Buttons::TAB2);
	}

	void UIOptionMenu::draw(float inter) const
	{
		if (nx_missing)
		{
			// Draw fallback UI
			background.draw(DrawArgument(position));
			title_bar.draw(DrawArgument(position));
			title_text.draw(position + Point<int16_t>(140, 4));
			resolution_label.draw(position + Point<int16_t>(15, 40));

			// Draw OK / Cancel button areas
			ColorBox ok_bg(80, 26, Color::Name::EMPEROR, 0.9f);
			ColorBox cancel_bg(80, 26, Color::Name::EMPEROR, 0.9f);
			ok_bg.draw(DrawArgument(position + Point<int16_t>(50, 160)));
			cancel_bg.draw(DrawArgument(position + Point<int16_t>(150, 160)));
			ok_label.draw(position + Point<int16_t>(90, 164));
			cancel_label.draw(position + Point<int16_t>(190, 164));

			// Draw the combobox
			UIElement::draw_buttons(inter);
		}
		else
		{
			UIElement::draw_sprites(inter);
			tab_background[selected_tab].draw(position);
			UIElement::draw_buttons(inter);
		}
	}

	Button::State UIOptionMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::TAB0:
		case Buttons::TAB1:
		case Buttons::TAB2:
		case Buttons::TAB3:
		case Buttons::TAB4:
			change_tab(buttonid);
			return Button::State::IDENTITY;
		case Buttons::CANCEL:
			deactivate();
			return Button::State::NORMAL;
		case Buttons::OK:
			switch (selected_tab)
			{
			case Buttons::TAB0:
			{
				uint16_t selected_value = buttons[Buttons::SELECT_RES]->get_selected();

				int16_t width = Constants::Constants::get().get_viewwidth();
				int16_t height = Constants::Constants::get().get_viewheight();

				switch (selected_value)
				{
				case 0:
					width = 800;
					height = 600;
					break;
				case 1:
					width = 1024;
					height = 768;
					break;
				case 2:
					width = 1280;
					height = 720;
					break;
				case 3:
					width = 1366;
					height = 768;
					break;
				case 4:
					width = 1920;
					height = 1080;
					break;
				case 5:
					width = 1920;
					height = 1200;
					break;
				}

				Setting<Width>::get().save(width);
				Setting<Height>::get().save(height);

				Constants::Constants::get().set_viewwidth(width);
				Constants::Constants::get().set_viewheight(height);

				// Flush settings to disk immediately so resolution persists
				Configuration::get().save();
			}
			break;
			case Buttons::TAB1:
			case Buttons::TAB2:
			case Buttons::TAB3:
			case Buttons::TAB4:
			default:
				break;
			}

			deactivate();
			return Button::State::NORMAL;
		case Buttons::UIRESET:
			return Button::State::DISABLED;
		case Buttons::SELECT_RES:
			buttons[Buttons::SELECT_RES]->toggle_pressed();
			return Button::State::NORMAL;
		default:
			return Button::State::DISABLED;
		}
	}

	Cursor::State UIOptionMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (nx_missing && clicked)
		{
			// Check OK button area
			auto ok_pos = position + Point<int16_t>(50, 160);
			if (cursorpos.x() >= ok_pos.x() && cursorpos.x() <= ok_pos.x() + 80 &&
				cursorpos.y() >= ok_pos.y() && cursorpos.y() <= ok_pos.y() + 26)
			{
				button_pressed(Buttons::OK);
				return Cursor::State::CLICKING;
			}

			// Check Cancel button area
			auto cancel_pos = position + Point<int16_t>(150, 160);
			if (cursorpos.x() >= cancel_pos.x() && cursorpos.x() <= cancel_pos.x() + 80 &&
				cursorpos.y() >= cancel_pos.y() && cursorpos.y() <= cancel_pos.y() + 26)
			{
				button_pressed(Buttons::CANCEL);
				return Cursor::State::CLICKING;
			}
		}

		auto& button = buttons[Buttons::SELECT_RES];

		if (button->is_pressed())
		{
			if (button->in_combobox(cursorpos))
			{
				if (Cursor::State new_state = button->send_cursor(clicked, cursorpos))
					return new_state;
			}
			else
			{
				remove_cursor();
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIOptionMenu::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				deactivate();
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(Buttons::OK);
		}
	}

	UIElement::Type UIOptionMenu::get_type() const
	{
		return TYPE;
	}

	void UIOptionMenu::change_tab(uint16_t tabid)
	{
		if (!nx_missing)
		{
			buttons[selected_tab]->set_state(Button::State::NORMAL);
			buttons[tabid]->set_state(Button::State::PRESSED);
		}

		selected_tab = tabid;

		switch (tabid)
		{
		case Buttons::TAB0:
			buttons[Buttons::SELECT_RES]->set_active(true);
			break;
		case Buttons::TAB1:
		case Buttons::TAB2:
		case Buttons::TAB3:
		case Buttons::TAB4:
			buttons[Buttons::SELECT_RES]->set_active(false);
			break;
		default:
			break;
		}
	}
}
