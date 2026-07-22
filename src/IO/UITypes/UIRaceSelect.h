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
#pragma once

#include "../UIElement.h"

#include "../../Graphics/Text.h"
#include "../../Template/BoolPair.h"

namespace ms
{
	// Authentic v83 race selection: three class illustrations (Explorer, Cygnus
	// Knights, Aran). The unselected ones are greyed out; the selected one lights
	// up in colour, its description bar shows at the bottom, and a scenic v83
	// login background sits behind it all.
	class UIRaceSelect : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::RACESELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIRaceSelect();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		bool check_name(std::string name) const;
		void send_naming_result(bool nameused);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void select_class(uint16_t index);
		void show_charselect();
		void launch_creation();
		std::string to_lower(std::string value) const;

		// v83 exposes exactly three creatable Explorer-era classes.
		static constexpr uint16_t CLASS_COUNT = 3;

		// The 800x600 scene renders centered in the view (CENTER_OFFSET); lay()
		// maps a design point to its on-screen position.
		Point<int16_t> lay(int16_t x, int16_t y) const;

		enum Buttons : uint16_t
		{
			BACK,
			SELECT,
			CLASS0 // CLASS0 .. CLASS0 + CLASS_COUNT-1 are the class panels
		};

		enum Classes : uint16_t
		{
			EXPLORER,
			CYGNUSKNIGHTS,
			ARAN
		};

		struct ClassPanel
		{
			BoolPair<Texture> panel; // [false]=grey normal, [true]=hover
			Texture glow;            // OnAnimation — the lit, coloured art
			Texture desc;            // 579x163 bottom description bar
			Point<int16_t> pos;      // top-left in design space
			Point<int16_t> dim;      // panel dimensions
		};

		Text version;
		Texture backdrop;
		Texture header;             // "Choose Character Type"
		ClassPanel classes[CLASS_COUNT];
		uint16_t selected;
		bool hovered[CLASS_COUNT];
	};
}
