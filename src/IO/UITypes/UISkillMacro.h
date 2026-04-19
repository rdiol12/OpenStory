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

#include "../UIDragElement.h"
#include "../Components/Textfield.h"

#include "../../Data/SkillData.h"

namespace ms
{
	struct SkillMacro
	{
		std::string name;
		bool shout;
		int32_t skill1;
		int32_t skill2;
		int32_t skill3;

		SkillMacro() : name(""), shout(false), skill1(0), skill2(0), skill3(0) {}
		SkillMacro(std::string n, bool s, int32_t s1, int32_t s2, int32_t s3)
			: name(std::move(n)), shout(s), skill1(s1), skill2(s2), skill3(s3) {}
	};

	class UISkillMacro : public UIDragElement<PosSKILLMACRO>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::SKILLMACRO;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		static constexpr int16_t NUM_MACROS = 5;

		UISkillMacro();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;
		Cursor::State send_cursor(bool clicking, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		void set_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3);

		bool send_icon(const Icon& icon, Point<int16_t> cursorpos) override;

		// Called by SkillMacrosHandler at login, before this UI is ever emplaced.
		static void cache_macro(uint8_t index, const std::string& name, bool shout, int32_t s1, int32_t s2, int32_t s3);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void save_macros();

		static SkillMacro cached_macros[NUM_MACROS];
		static bool cache_loaded;

		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_OK,
			BT_CANCEL
		};

		SkillMacro macros[NUM_MACROS];
		Textfield name_fields[NUM_MACROS];
		Texture skill_slot;
		Texture shout_on;
		Texture shout_off;

		// Row layout
		Point<int16_t> row_origin;
		int16_t row_height;
	};
}
