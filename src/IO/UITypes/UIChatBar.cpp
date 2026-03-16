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
#include "UIChatBar.h"

#include <iostream>

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Net/Packets/MessagingPackets.h"
#include "../../Constants.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIChatBar::UIChatBar() : UIDragElement<PosCHAT>(Point<int16_t>(500, -5))
	{
		chatopen = true;
		chatopen_persist = true;
		chatfieldopen = false;
		chatrows = 4;
		lastpos = 0;
		rowpos = 0;
		rowmax = -1;
		dragchattop = false;
		chat_fade_timer = CHAT_FADE_DURATION;
		current_target = TARGET_ALL;

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];

		// Load textures from mainBar (v83 compatible paths)
		buttons[Buttons::BT_OPENCHAT] = std::make_unique<MapleButton>(mainbar["chatOpen"]);
		buttons[Buttons::BT_CLOSECHAT] = std::make_unique<MapleButton>(mainbar["chatClose"]);
		buttons[Buttons::BT_SCROLLUP] = std::make_unique<MapleButton>(mainbar["scrollUp"]);
		buttons[Buttons::BT_SCROLLDOWN] = std::make_unique<MapleButton>(mainbar["scrollDown"]);

		chatspace[0] = mainbar["chatSpace"];
		chatspace[1] = mainbar["chatEnter"];
		chatspace[2] = mainbar["chatSpace2"];
		chatspace[3] = mainbar["chatCover"];

		// Load chat target channel indicator textures
		nl::node target_node = mainbar["chatTarget"];
		chattarget_textures[TARGET_ALL] = target_node["all"];
		chattarget_textures[TARGET_PARTY] = target_node["party"];
		chattarget_textures[TARGET_GUILD] = target_node["guild"];
		chattarget_textures[TARGET_FRIEND] = target_node["friend"];
		chattarget_textures[TARGET_EXPEDITION] = target_node["expedition"];
		chattarget_textures[TARGET_ASSOCIATION] = target_node["association"];
		chattarget_textures[TARGET_AFCTV] = target_node["afctv"];

		buttons[BT_OPENCHAT]->set_active(!chatopen);
		buttons[BT_CLOSECHAT]->set_active(chatopen);
		buttons[BT_SCROLLUP]->set_active(chatopen);
		buttons[BT_SCROLLDOWN]->set_active(chatopen);

		// Chat background box
		chat_background = ColorBox(502, 1 + chatrows * CHATROWHEIGHT, Color::Name::BLACK, 0.6f);

		// Textfield bounds are relative to the draw position (512, screen_h)
		// The chat input area is at screen coordinates ~(65, 738) to (460, 755)
		chatfield = Textfield(Text::A11M, Text::LEFT, Color::Name::WHITE, Rectangle<int16_t>(Point<int16_t>(65 - 512, -30), Point<int16_t>(460 - 512, -13)), 0);
		chatfield.set_state(chatopen ? Textfield::State::NORMAL : Textfield::State::DISABLED);

		chatfield.set_enter_callback(
			[&](std::string msg)
			{
				if (msg.size() > 0)
				{
					size_t last = msg.find_last_not_of(' ');

					if (last != std::string::npos)
					{
						msg.erase(last + 1);

						GeneralChatPacket(msg, true).dispatch();

						lastentered.push_back(msg);
						lastpos = lastentered.size();
					}
					else
					{
						toggle_chatfield();
					}

					chatfield.change_text("");
				}
				else
				{
					toggle_chatfield();
				}
			}
		);

		chatfield.set_key_callback(
			KeyAction::Id::UP,
			[&]()
			{
				if (lastpos > 0)
				{
					lastpos--;
					chatfield.change_text(lastentered[lastpos]);
				}
			}
		);

		chatfield.set_key_callback(
			KeyAction::Id::DOWN,
			[&]()
			{
				if (lastentered.size() > 0 && lastpos < lastentered.size() - 1)
				{
					lastpos++;
					chatfield.change_text(lastentered[lastpos]);
				}
			}
		);

		chatfield.set_key_callback(
			KeyAction::Id::ESCAPE,
			[&]()
			{
				toggle_chatfield(false);
			}
		);

		send_chatline("[Welcome] Welcome to MapleStory!!", LineType::YELLOW);

		// Position at StatusBar anchor point so NX textures draw correctly
		int16_t screen_h = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>(512, screen_h);
		dimension = Point<int16_t>(1366, 84);
	}

	void UIChatBar::draw(float inter) const
	{
		bool show_background = chatfieldopen || chat_fade_timer > 0;

		// Draw NX bar textures at StatusBar anchor (position = (512, screen_h))
		// z-order: chatSpace(0) → chatSpace2(1)/chatEnter(1) → chatCover(3)
		chatspace[0].draw(position);

		if (chatopen)
		{
			chatspace[2].draw(position);	// chatSpace2 — expanded chat area (z=1)
			chatspace[1].draw(position);	// chatEnter — input area background (z=1)
		}

		UIElement::draw_buttons(inter);

		if (chatopen)
		{
			int16_t chattop = getchattop(true);
			int16_t chatheight = chatrows * CHATROWHEIGHT;

			if (show_background)
			{
				// Chat background box — draw at left side of screen
				float opacity = 0.6f;
				if (!chatfieldopen && chat_fade_timer < 1000)
					opacity = 0.6f * (chat_fade_timer / 1000.0f);

				ColorBox bg(502, 1 + chatheight, Color::Name::BLACK, opacity);
				bg.draw(DrawArgument(Point<int16_t>(0, chattop + 2)));
			}

			// Draw chat messages
			if (show_background)
			{
				int16_t yshift = -chatheight;
				for (int16_t i = 0; i < chatrows; i++)
				{
					int16_t rowid = rowpos - i;

					if (!rowtexts.count(rowid))
						break;

					int16_t textheight = rowtexts.at(rowid).height() / CHATROWHEIGHT;

					while (textheight > 0)
					{
						yshift += CHATROWHEIGHT;
						textheight--;
					}

					rowtexts.at(rowid).draw(Point<int16_t>(4, chattop - yshift - 1));
				}
			}

			// Draw chatcover and chatfield at StatusBar anchor
			chatspace[3].draw(position);

			// Draw chat target indicator (shows current channel: all/party/guild/etc.)
			chattarget_textures[current_target].draw(position);

			chatfield.draw(position);
		}
		else if (rowtexts.count(rowmax))
		{
			// Show last message line when chat is closed
			rowtexts.at(rowmax).draw(Point<int16_t>(4, position.y() - 60));
		}
	}

	void UIChatBar::update()
	{
		UIElement::update();

		chatfield.update(position);

		if (chat_fade_timer > 0 && !chatfieldopen)
			chat_fade_timer -= Constants::TIMESTEP;
	}

	void UIChatBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (keycode == KeyAction::Id::RETURN)
				toggle_chatfield();
			else if (escape)
				toggle_chatfield(false);
		}
	}

	bool UIChatBar::is_in_range(Point<int16_t> cursorpos) const
	{
		int16_t chattop = getchattop(chatopen);
		// Chat area: x=0 to 500, y=chattop-16 to screen bottom
		Point<int16_t> absp(0, chattop - 16);
		Point<int16_t> rb(500, position.y());
		return Rectangle<int16_t>(absp, rb).contains(cursorpos);
	}

	Cursor::State UIChatBar::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		if (chatopen)
		{
			if (Cursor::State new_state = chatfield.send_cursor(cursorpos, clicking))
				return new_state;
		}

		return UIDragElement::send_cursor(clicking, cursorpos);
	}

	UIElement::Type UIChatBar::get_type() const
	{
		return TYPE;
	}

	Cursor::State UIChatBar::check_dragtop(bool clicking, Point<int16_t> cursorpos)
	{
		return Cursor::State::IDLE;
	}

	void UIChatBar::send_chatline(const std::string& line, LineType type)
	{
		rowmax++;
		rowpos = rowmax;
		chat_fade_timer = CHAT_FADE_DURATION;

		Color::Name color;

		switch (type)
		{
		case LineType::RED:
			color = Color::Name::RED;
			break;
		case LineType::BLUE:
			color = Color::Name::BLUE;
			break;
		case LineType::YELLOW:
			color = Color::Name::YELLOW;
			break;
		default:
			color = Color::Name::WHITE;
			break;
		}

		rowtexts.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(rowmax),
			std::forward_as_tuple(Text::Font::A12M, Text::Alignment::LEFT, color, line, 480)
		);
	}

	void UIChatBar::display_message(Messages::Type line, UIChatBar::LineType type)
	{
		if (message_cooldowns[line] > 0)
			return;

		std::string message = Messages::messages[line];
		send_chatline(message, type);

		message_cooldowns[line] = MESSAGE_COOLDOWN;
	}

	void UIChatBar::toggle_chat()
	{
		chatopen_persist = !chatopen_persist;
		toggle_chat(chatopen_persist);
	}

	void UIChatBar::toggle_chat(bool chat_open)
	{
		if (!chat_open && chatopen_persist)
			return;

		chatopen = chat_open;

		if (!chatopen && chatfieldopen)
			toggle_chatfield();

		buttons[Buttons::BT_OPENCHAT]->set_active(!chat_open);
		buttons[Buttons::BT_CLOSECHAT]->set_active(chat_open);
		buttons[Buttons::BT_SCROLLUP]->set_active(chat_open);
		buttons[Buttons::BT_SCROLLDOWN]->set_active(chat_open);
	}

	void UIChatBar::toggle_chatfield()
	{
		chatfieldopen = !chatfieldopen;
		toggle_chatfield(chatfieldopen);
	}

	void UIChatBar::toggle_chatfield(bool chatfield_open)
	{
		chatfieldopen = chatfield_open;

		toggle_chat(chatfieldopen);

		if (chatfieldopen)
		{
			chat_fade_timer = CHAT_FADE_DURATION;
			chatfield.set_state(Textfield::State::FOCUSED);
			UI::get().focus_textfield(&chatfield);
		}
		else
		{
			chatfield.set_state(Textfield::State::NORMAL);
			UI::get().remove_textfield();
		}
	}

	bool UIChatBar::is_chatopen()
	{
		return chatopen;
	}

	bool UIChatBar::is_chatfieldopen()
	{
		return chatfieldopen;
	}

	Button::State UIChatBar::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::BT_OPENCHAT:
		case Buttons::BT_CLOSECHAT:
			toggle_chat();
			break;
		case Buttons::BT_SCROLLUP:
			if (rowpos < rowmax)
				rowpos++;
			break;
		case Buttons::BT_SCROLLDOWN:
			if (rowpos > 0)
				rowpos--;
			break;
		case Buttons::BT_CHAT_TARGET:
			current_target = static_cast<ChatTarget>((current_target + 1) % NUM_TARGETS);
			break;
		default:
			break;
		}

		Setting<Chatopen>::get().save(chatopen);

		return Button::State::NORMAL;
	}

	int16_t UIChatBar::getchattop(bool chat_open) const
	{
		if (chat_open)
			return position.y() - chatrows * CHATROWHEIGHT - 65;
		else
			return position.y() - 1;
	}

	int16_t UIChatBar::getchatbarheight() const
	{
		return 15 + chatrows * CHATROWHEIGHT;
	}

	Rectangle<int16_t> UIChatBar::getbounds(Point<int16_t> additional_area) const
	{
		int16_t screen_adj = (chatopen) ? 35 : 16;

		auto absp = position + Point<int16_t>(0, getchattop(chatopen));
		auto da = absp + additional_area;

		absp = Point<int16_t>(absp.x(), absp.y() - screen_adj);
		da = Point<int16_t>(da.x(), da.y());

		return Rectangle<int16_t>(absp, da);
	}

	bool UIChatBar::indragrange(Point<int16_t> cursorpos) const
	{
		return false;
	}
}
