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

namespace ms
{
	class UIMessenger : public UIDragElement<PosMESSENGER>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MESSENGER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIMessenger();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Called from packet handlers
		void add_player(int8_t slot, const std::string& name);
		void remove_player(int8_t slot);
		void add_chat(const std::string& name, const std::string& message);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_ENTER
		};

		static constexpr int8_t MAX_PLAYERS = 3;
		static constexpr int16_t MAX_CHAT_LINES = 8;

		// Player slots (up to 3 people in a messenger)
		std::string player_names[MAX_PLAYERS];
		bool player_occupied[MAX_PLAYERS];

		// Name bar textures (0-3 states)
		Texture name_bars[4];
		Texture blink_tex;

		// Chat balloon textures
		Texture chat_balloons[5];

		// Chat history
		struct ChatLine
		{
			std::string name;
			std::string message;
		};

		std::vector<ChatLine> chat_lines;
		mutable Text name_label;
		mutable Text chat_label;
	};
}
