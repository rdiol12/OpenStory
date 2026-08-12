//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
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

#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"

#include <cstdint>
#include <map>

namespace ms
{
	class UIMonsterLife : public UIDragElement<PosMONSTERLIFE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MONSTERLIFE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIMonsterLife();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_REMOVE
		};

		// FarmUI has no placement-surface bitmap (in KMS the farm is a walkable map),
		// so the panel's ground band doubles as the surface. Measured off backgrnd:
		// blue header 9-85, tan 86-116, white box 117-172, brown ground 186-522.
		static constexpr int16_t SURF_X = 9;
		static constexpr int16_t SURF_Y = 186;
		static constexpr int16_t SURF_W = 428;
		static constexpr int16_t SURF_H = 336;
		static constexpr int16_t PAL_X = 16;
		static constexpr int16_t PAL_Y = 128;
		static constexpr int16_t PAL_STEP = 30;
		static constexpr int16_t PAL_MAX = 13;
		static constexpr int16_t CARD_W = 24;
		static constexpr int16_t CARD_H = 32;

		void refresh();
		Texture card_texture(int32_t cardid) const;
		int16_t palette_at(Point<int16_t> cursorpos) const;
		int32_t entry_at(Point<int16_t> cursorpos) const;
		bool surface_contains(Point<int16_t> rel) const;

		Texture cover;
		Texture placeholder;

		mutable std::map<int32_t, Texture> card_cache;

		Text title_label;
		Text status_label;

		int32_t sel_card = 0;
		int32_t sel_slot = 0;
		int32_t drag_slot = 0;
		size_t last_entry_count = 0;
		uint8_t last_error = 0;
		bool loaded_once = false;
	};
}
