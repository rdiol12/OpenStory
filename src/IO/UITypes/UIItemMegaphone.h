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
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "../Components/Textfield.h"

#include <cstdint>
#include <string>

namespace ms
{
	// Compose dialog for Item Megaphone cash items (5076xxx). Renders
	// UIWindow2.img/ItemMegaphone/backgrnd (236x182) with a single message
	// field, a whisper-ear checkbox, and an item-attach drop area that
	// lets the broadcaster attach one inventory item for preview.
	class UIItemMegaphone : public UIDragElement<PosITEMMEGAPHONE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::ITEMMEGAPHONE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIItemMegaphone();

		void configure_item(int16_t slot, int32_t itemid);

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
			BT_SEND
		};

		void send_broadcast();

		Textfield msg_field;
		Texture check_icon;
		Text prompt_label;

		bool whisper_enabled;

		int16_t item_slot;
		int32_t item_id;
	};
}
