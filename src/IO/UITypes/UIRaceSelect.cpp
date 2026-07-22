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
//////////////////////////////////////////////////////////////////////////////////
#include "UIRaceSelect.h"

#include "UIAranCreation.h"
#include "UICharSelect.h"
#include "UICygnusCreation.h"
#include "UIExplorerCreation.h"

#include "../UI.h"
#include "../UIScale.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"

#include "../../Configuration.h"
#include "../../Constants.h"

#include "../../Audio/Audio.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Design-space layout (800x600). Nudge these while iterating on screenshots.
	namespace
	{
		constexpr int16_t HEADER_X = 400; // textGL is centre-anchored (origin 91,19)
		constexpr int16_t HEADER_Y = 88;
		constexpr int16_t PANEL_Y = 150;  // common top for the three panels
		constexpr int16_t PANEL_GAP = 14; // gap between panels
		constexpr int16_t DESC_Y = 396;   // description bar top
		// BtSelect sits in the empty slot at the bottom-right of the description
		// panel (not centred over the text).
		constexpr int16_t SELECT_X = 535;
		constexpr int16_t SELECT_Y = 520;
	}

	UIRaceSelect::UIRaceSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600), ScaleMode::CENTER_OFFSET)
	{
		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);

		nl::node Login = nl::nx::ui["Login.img"];
		nl::node Common = Login["Common"];
		nl::node RaceSelect = Login["RaceSelect"];

		// v83 login sky, stretched over the whole view (matches char select).
		backdrop = Texture(nl::nx::map["Back"]["login.img"]["back"]["11"]);

		header = Texture(RaceSelect["textGL"]["0"]);

		// The three creatable Explorer-era classes, in v83 order.
		const char* nodes[CLASS_COUNT] = { "normal", "knight", "aran" };
		const char* btns[CLASS_COUNT] = { "BtNormal", "BtKnight", "BtAran" };

		int16_t total_w = 0;
		for (uint16_t i = 0; i < CLASS_COUNT; i++)
		{
			nl::node cls = RaceSelect[nodes[i]];
			nl::node bt = cls[btns[i]];

			classes[i].panel[false] = Texture(bt["normal"]["0"]);
			classes[i].panel[true] = Texture(bt["mouseOver"]["0"]);
			classes[i].glow = Texture(cls["OnAnimation"]["0"]);
			classes[i].desc = Texture(cls["text"]["0"]);
			classes[i].dim = classes[i].panel[false].get_dimensions();

			total_w += classes[i].dim.x();
		}
		total_w += PANEL_GAP * (CLASS_COUNT - 1);

		// centre the row of panels; store each panel's top-left
		int16_t x = (800 - total_w) / 2;
		for (uint16_t i = 0; i < CLASS_COUNT; i++)
		{
			classes[i].pos = Point<int16_t>(x, PANEL_Y);
			buttons[Buttons::CLASS0 + i] = std::make_unique<AreaButton>(classes[i].pos, classes[i].dim);
			x += classes[i].dim.x() + PANEL_GAP;
			hovered[i] = false;
		}

		buttons[Buttons::BACK] = std::make_unique<MapleButton>(Common["BtStart"], Point<int16_t>(0, 515));
		buttons[Buttons::SELECT] = std::make_unique<MapleButton>(RaceSelect["BtSelect"], Point<int16_t>(SELECT_X, SELECT_Y));

		selected = Classes::EXPLORER;

		Sound(Sound::Name::RACESELECT).play();
	}

	Point<int16_t> UIRaceSelect::lay(int16_t x, int16_t y) const
	{
		return scaled(x, y);
	}

	void UIRaceSelect::draw(float inter) const
	{
		if (backdrop.is_valid())
		{
			Point<int16_t> o = backdrop.get_origin();
			backdrop.draw(DrawArgument(
				o, o,
				Point<int16_t>(
					static_cast<int16_t>(UIScale::view_width()),
					static_cast<int16_t>(UIScale::view_height())),
				1.0f, 1.0f, 1.0f, 0.0f));
		}

		UIElement::draw_sprites(inter);

		header.draw(lay(HEADER_X, HEADER_Y));

		// panels: the selected one lights up (glow), the rest stay greyed out
		for (uint16_t i = 0; i < CLASS_COUNT; i++)
		{
			Point<int16_t> p = lay(classes[i].pos.x(), classes[i].pos.y());

			if (i == selected)
				classes[i].glow.draw(p);
			else
				classes[i].panel[hovered[i]].draw(p);
		}

		// description bar for the selected class, centred at the bottom
		const Texture& desc = classes[selected].desc;
		int16_t dx = (800 - desc.get_dimensions().x()) / 2;
		desc.draw(lay(dx, DESC_Y));

		UIElement::draw_buttons(inter);

		version.draw(scaled(707, 4));
	}

	void UIRaceSelect::update()
	{
		UIElement::update();
	}

	Cursor::State UIRaceSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		for (auto& btit : buttons)
		{
			bool inside = btit.second->is_active()
				&& btit.second->bounds(get_draw_position()).contains(cursorpos);

			if (inside)
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();

					if (btit.first >= Buttons::CLASS0)
						hovered[btit.first - Buttons::CLASS0] = true;

					btit.second->set_state(Button::State::MOUSEOVER);
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER && clicked)
				{
					Sound(Sound::Name::BUTTONCLICK).play();

					btit.second->set_state(button_pressed(btit.first));
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				if (btit.first >= Buttons::CLASS0)
					hovered[btit.first - Buttons::CLASS0] = false;

				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return Cursor::State::LEAF;
	}

	void UIRaceSelect::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (!pressed)
			return;

		if (escape)
		{
			show_charselect();
		}
		else if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::DOWN)
		{
			if (selected > 0)
				select_class(selected - 1);
		}
		else if (keycode == KeyAction::Id::RIGHT || keycode == KeyAction::Id::UP)
		{
			if (selected + 1 < CLASS_COUNT)
				select_class(selected + 1);
		}
		else if (keycode == KeyAction::Id::RETURN)
		{
			launch_creation();
		}
	}

	UIElement::Type UIRaceSelect::get_type() const
	{
		return TYPE;
	}

	bool UIRaceSelect::check_name(std::string name) const
	{
		nl::node ForbiddenName = nl::nx::etc["ForbiddenName.img"];

		for (std::string forbiddenName : ForbiddenName)
		{
			std::string lName = to_lower(name);
			std::string fName = to_lower(forbiddenName);

			if (lName.find(fName) != std::string::npos)
				return false;
		}

		return true;
	}

	void UIRaceSelect::send_naming_result(bool nameused)
	{
		if (selected == Classes::EXPLORER)
		{
			if (auto explorercreation = UI::get().get_element<UIExplorerCreation>())
				explorercreation->send_naming_result(nameused);
		}
		else if (selected == Classes::CYGNUSKNIGHTS)
		{
			if (auto cygnuscreation = UI::get().get_element<UICygnusCreation>())
				cygnuscreation->send_naming_result(nameused);
		}
		else if (selected == Classes::ARAN)
		{
			if (auto arancreation = UI::get().get_element<UIAranCreation>())
				arancreation->send_naming_result(nameused);
		}
	}

	Button::State UIRaceSelect::button_pressed(uint16_t buttonid)
	{
		if (buttonid == Buttons::BACK)
		{
			show_charselect();

			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::SELECT)
		{
			launch_creation();

			return Button::State::NORMAL;
		}
		else if (buttonid >= Buttons::CLASS0)
		{
			select_class(buttonid - Buttons::CLASS0);

			return Button::State::NORMAL;
		}

		return Button::State::DISABLED;
	}

	void UIRaceSelect::select_class(uint16_t index)
	{
		if (index >= CLASS_COUNT || index == selected)
			return;

		selected = index;

		Sound(Sound::Name::RACESELECT).play();
	}

	void UIRaceSelect::launch_creation()
	{
		Sound(Sound::Name::SCROLLUP).play();

		uint16_t cls = selected;

		deactivate();

		if (cls == Classes::EXPLORER)
			UI::get().emplace<UIExplorerCreation>();
		else if (cls == Classes::CYGNUSKNIGHTS)
			UI::get().emplace<UICygnusCreation>();
		else if (cls == Classes::ARAN)
			UI::get().emplace<UIAranCreation>();
	}

	void UIRaceSelect::show_charselect()
	{
		Sound(Sound::Name::SCROLLUP).play();

		deactivate();

		if (auto charselect = UI::get().get_element<UICharSelect>())
			charselect->makeactive();
	}

	std::string UIRaceSelect::to_lower(std::string value) const
	{
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);

		return value;
	}
}
