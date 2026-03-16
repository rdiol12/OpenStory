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

#include "../Messages.h"
#include "../UIDragElement.h"

#include "../Components/Slider.h"
#include "../Components/Textfield.h"
#include "../../Graphics/Geometry.h"

namespace ms
{
	class UIChatBar : public UIDragElement<PosCHAT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHATBAR;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		enum LineType
		{
			UNK0,
			WHITE,
			RED,
			BLUE,
			YELLOW
		};

		UIChatBar();

		void draw(float inter) const override;
		void update() override;

		void send_key(int32_t keycode, bool pressed, bool escape) override;

		bool is_in_range(Point<int16_t> cursorpos) const override;
		Cursor::State send_cursor(bool clicking, Point<int16_t> cursorpos) override;

		UIElement::Type get_type() const override;

		Cursor::State check_dragtop(bool clicking, Point<int16_t> cursorpos);

		void send_chatline(const std::string& line, LineType type);
		void display_message(Messages::Type line, UIChatBar::LineType type);
		void toggle_chat();
		void toggle_chat(bool chat_open);
		void toggle_chatfield();
		void toggle_chatfield(bool chatfield_open);
		bool is_chatopen();
		bool is_chatfieldopen();

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		bool indragrange(Point<int16_t> cursorpos) const override;

		int16_t getchattop(bool chat_open) const;
		int16_t getchatbarheight() const;
		Rectangle<int16_t> getbounds(Point<int16_t> additional_area) const;

		static constexpr int16_t CHATROWHEIGHT = 16;
		static constexpr int16_t MINCHATROWS = 1;
		static constexpr int16_t MAXCHATROWS = 16;
		static constexpr int16_t DIMENSION_Y = 60;
		static constexpr time_t MESSAGE_COOLDOWN = 1'000;

		enum Buttons : uint16_t
		{
			BT_OPENCHAT,
			BT_CLOSECHAT,
			BT_SCROLLUP,
			BT_SCROLLDOWN,
			BT_CHAT,
			BT_HELP,
			BT_LINK,
			BT_TAB_0,
			BT_TAB_1,
			BT_TAB_2,
			BT_TAB_3,
			BT_TAB_4,
			BT_TAB_5,
			BT_TAB_ADD,
			BT_CHAT_TARGET
		};

		enum ChatTab
		{
			CHT_ALL,
			CHT_BATTLE,
			CHT_PARTY,
			CHT_FRIEND,
			CHT_GUILD,
			CHT_ALLIANCE,
			NUM_CHATTAB
		};

		enum ChatTarget : uint8_t
		{
			TARGET_ALL,
			TARGET_PARTY,
			TARGET_GUILD,
			TARGET_FRIEND,
			TARGET_EXPEDITION,
			TARGET_ASSOCIATION,
			TARGET_AFCTV,
			NUM_TARGETS
		};

		bool chatopen;
		bool chatopen_persist;
		bool chatfieldopen;
		Texture chatspace[4];
		Texture chatenter;
		Texture chatcover;
		Texture chattarget_textures[NUM_TARGETS];
		ChatTarget current_target;
		Textfield chatfield;

		Slider slider;

		EnumMap<Messages::Type, time_t> message_cooldowns;
		std::vector<std::string> lastentered;
		size_t lastpos;

		int16_t chatrows;
		int16_t rowpos;
		int16_t rowmax;
		std::unordered_map<int16_t, Text> rowtexts;

		bool dragchattop;
		ColorBox chat_background;
		int32_t chat_fade_timer;
		static constexpr int32_t CHAT_FADE_DURATION = 5000;
	};
}
