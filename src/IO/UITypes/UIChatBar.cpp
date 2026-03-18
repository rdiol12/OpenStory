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

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Net/Packets/GameplayPackets.h"
#include "../../Net/Packets/MessagingPackets.h"
#include "../../Audio/Audio.h"
#include "../../Constants.h"

#include <algorithm>
#include <iostream>
#include <cctype>
#include <list>
#include <sstream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		std::string trim(const std::string& value)
		{
			size_t first = value.find_first_not_of(' ');
			if (first == std::string::npos)
				return "";

			size_t last = value.find_last_not_of(' ');
			return value.substr(first, last - first + 1);
		}

		std::string lowercase(std::string value)
		{
			std::transform(value.begin(), value.end(), value.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return value;
		}

		bool parse_int32(const std::string& value, int32_t& out)
		{
			if (value.empty())
				return false;

			size_t start = 0;
			if (value[0] == '+' || value[0] == '-')
				start = 1;

			for (size_t i = start; i < value.size(); i++)
				if (!std::isdigit(static_cast<unsigned char>(value[i])))
					return false;

			try { out = std::stoi(value); }
			catch (...) { return false; }
			return true;
		}
	}

	UIChatBar::UIChatBar(Point<int16_t> pos) : UIElement(pos, Point<int16_t>(500, 60))
	{
		chatopen = true;
		chatfieldopen = false;
		chattarget = CHT_ALL;
		party_id = -1;
		party_leader_id = -1;
		pending_party_invite_id = -1;
		chatrows = 4;
		rowpos = 0;
		rowmax = -1;
		lastpos = 0;
		dragchattop = false;

		nl::node mainbar = nl::nx::ui["StatusBar2.img"]["mainBar"];

		buttons[BT_OPENCHAT] = std::make_unique<MapleButton>(mainbar["chatOpen"]);
		buttons[BT_CLOSECHAT] = std::make_unique<MapleButton>(mainbar["chatClose"]);
		buttons[BT_SCROLLUP] = std::make_unique<MapleButton>(mainbar["scrollUp"]);
		buttons[BT_SCROLLDOWN] = std::make_unique<MapleButton>(mainbar["scrollDown"]);
		buttons[BT_CHATTARGETS] = std::make_unique<MapleButton>(mainbar["chatTarget"]["base"]);

		chatspace[0] = mainbar["chatSpace"];
		chatspace[1] = mainbar["chatEnter"];
		chatenter = mainbar["chatSpace2"];
		chatcover = mainbar["chatCover"];

		std::cout << "[CHATBAR] position=(" << pos.x() << "," << pos.y() << ")" << std::endl;
		std::cout << "[CHATBAR] chatSpace origin=(" << chatspace[0].get_origin().x() << "," << chatspace[0].get_origin().y() << ") dim=(" << chatspace[0].get_dimensions().x() << "," << chatspace[0].get_dimensions().y() << ")" << std::endl;
		std::cout << "[CHATBAR] chatEnter origin=(" << chatspace[1].get_origin().x() << "," << chatspace[1].get_origin().y() << ") dim=(" << chatspace[1].get_dimensions().x() << "," << chatspace[1].get_dimensions().y() << ")" << std::endl;
		std::cout << "[CHATBAR] chatSpace2 origin=(" << chatenter.get_origin().x() << "," << chatenter.get_origin().y() << ") dim=(" << chatenter.get_dimensions().x() << "," << chatenter.get_dimensions().y() << ")" << std::endl;
		std::cout << "[CHATBAR] chatCover origin=(" << chatcover.get_origin().x() << "," << chatcover.get_origin().y() << ") dim=(" << chatcover.get_dimensions().x() << "," << chatcover.get_dimensions().y() << ")" << std::endl;

		chattargets[CHT_ALL] = mainbar["chatTarget"]["all"];
		chattargets[CHT_BUDDY] = mainbar["chatTarget"]["friend"];
		chattargets[CHT_GUILD] = mainbar["chatTarget"]["guild"];
		chattargets[CHT_ALLIANCE] = mainbar["chatTarget"]["association"];
		chattargets[CHT_PARTY] = mainbar["chatTarget"]["party"];
		chattargets[CHT_SQUAD] = mainbar["chatTarget"]["expedition"];

		nl::node chat = nl::nx::ui["StatusBar2.img"]["chat"];
		tapbar = chat["tapBar"];
		tapbartop = chat["tapBarOver"];

		chatbox = ColorBox(502, 1 + chatrows * CHATROWHEIGHT, Color::Name::BLACK, 0.6f);

		chatfield = Textfield(Text::A11M, Text::LEFT, Color::Name::BLACK, Rectangle<int16_t>(Point<int16_t>(-435, -58), Point<int16_t>(-40, -35)), 0);

		set_chat_open(chatopen);

		chatfield.set_enter_callback(
			[&](std::string msg)
			{
				msg = trim(msg);
				if (msg.empty())
					return;

				if (!handle_party_command(msg))
					send_chat_message(msg);

				lastentered.push_back(msg);
				lastpos = lastentered.size();
				chatfield.change_text("");
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

		slider = Slider(11, Range<int16_t>(0, CHATROWHEIGHT * chatrows - 14), -22, chatrows, 1,
			[&](bool up)
			{
				int16_t next = up ? rowpos - 1 : rowpos + 1;
				if (next >= 0 && next <= rowmax)
					rowpos = next;
			}
		);

		send_line("[Welcome] Welcome to MapleStory!", YELLOW);
	}

	void UIChatBar::set_chat_open(bool open)
	{
		static FILE* dbg = fopen("C:\\Users\\rdiol\\OpenStory2\\wz\\ui.txt", "a");
		if (dbg) { fprintf(dbg, "[set_chat_open] open=%d (was %d)\n", open, chatopen); fflush(dbg); }
		chatopen = open;
		buttons[BT_OPENCHAT]->set_active(!open);
		buttons[BT_CLOSECHAT]->set_active(open);
		buttons[BT_CHATTARGETS]->set_active(open);
		buttons[BT_SCROLLUP]->set_active(open);
		buttons[BT_SCROLLDOWN]->set_active(open);

		if (!open)
		{
			chatfieldopen = false;
			chatfield.set_state(Textfield::State::DISABLED);
		}
		else
		{
			chatfield.set_state(Textfield::State::NORMAL);
		}
	}

	void UIChatBar::focus_chatfield()
	{
		if (!chatopen)
			set_chat_open(true);

		chatfieldopen = true;
		chatfield.set_state(Textfield::State::FOCUSED);
	}

	void UIChatBar::draw(float inter) const
	{
		chatspace[chatopen ? 1 : 0].draw(position);
		chatenter.draw(position);

		UIElement::draw_buttons(inter);

		if (chatopen)
		{
			tapbartop.draw(Point<int16_t>(position.x() - 576, getchattop()));

			chatbox.draw(DrawArgument(Point<int16_t>(0, getchattop() + 2)));

			int16_t chatheight = CHATROWHEIGHT * chatrows;
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

				rowtexts.at(rowid).draw(Point<int16_t>(4, getchattop() - yshift - 1));
			}

			slider.draw(Point<int16_t>(position.x(), getchattop() + 5));

			if (chattarget >= CHT_ALL && chattarget < NUM_TARGETS)
				chattargets[chattarget].draw(DrawArgument(position + Point<int16_t>(0, 2)));

			chatcover.draw(position);
			chatfield.draw(position);
		}
		else if (rowtexts.count(rowmax))
		{
			rowtexts.at(rowmax).draw(position + Point<int16_t>(-500, -60));
		}
	}

	void UIChatBar::update()
	{
		UIElement::update();
		chatfield.update(position);
	}

	void UIChatBar::set_position(Point<int16_t> pos)
	{
		position = pos;
	}

	void UIChatBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
		static FILE* dbg = fopen("C:\\Users\\rdiol\\OpenStory2\\wz\\ui.txt", "a");
		if (dbg) { fprintf(dbg, "[UIChatBar::send_key] keycode=%d pressed=%d escape=%d RETURN=%d\n", keycode, pressed, escape, KeyAction::Id::RETURN); fflush(dbg); }

		if (pressed)
		{
			if (keycode == KeyAction::Id::RETURN)
			{
				if (dbg) { fprintf(dbg, "[UIChatBar::send_key] calling toggle_chatfield, chatfieldopen=%d\n", chatfieldopen); fflush(dbg); }
				toggle_chatfield();
				if (dbg) { fprintf(dbg, "[UIChatBar::send_key] after toggle, chatfieldopen=%d state=%d\n", chatfieldopen, (int)chatfield.get_state()); fflush(dbg); }
			}
			else if (escape)
				toggle_chatfield(false);
		}
	}

	UIElement::Type UIChatBar::get_type() const
	{
		return TYPE;
	}

	Button::State UIChatBar::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case BT_OPENCHAT:
			set_chat_open(true);
			break;
		case BT_CLOSECHAT:
			set_chat_open(false);
			break;
		case BT_SCROLLUP:
			if (rowpos < rowmax)
				rowpos++;
			break;
		case BT_SCROLLDOWN:
			if (rowpos > 0)
				rowpos--;
			break;
		case BT_CHATTARGETS:
			cycle_chat_target();
			return Button::State::NORMAL;
		}

		return Button::State::NORMAL;
	}

	bool UIChatBar::is_in_range(Point<int16_t> cursorpos) const
	{
		Point<int16_t> absp(0, getchattop() - 16);
		Point<int16_t> dim(500, chatrows * CHATROWHEIGHT + CHATYOFFSET + 16);
		return Rectangle<int16_t>(absp, absp + dim).contains(cursorpos);
	}

	Cursor::State UIChatBar::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		// Custom button handling: break after a button press fires.
		// UIElement::send_cursor iterates ALL buttons in one loop, so when
		// BT_CLOSECHAT fires and activates BT_OPENCHAT, the loop immediately
		// processes BT_OPENCHAT in the same frame, causing toggle bounce.
		bool button_handled = false;

		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();
					btit.second->set_state(Button::State::MOUSEOVER);
					return Cursor::State::CANCLICK;
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicking)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(button_pressed(btit.first));
						button_handled = true;
						break;  // Stop processing buttons after a press fires
					}
					else
					{
						return Cursor::State::CANCLICK;
					}
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		if (button_handled)
			return Cursor::State::IDLE;

		if (slider.isenabled())
		{
			auto cursoroffset = cursorpos - Point<int16_t>(position.x(), getchattop() + 5);
			Cursor::State sstate = slider.send_cursor(cursoroffset, clicking);
			if (sstate != Cursor::State::IDLE)
				return sstate;
		}

		if (chatopen)
		{
			Cursor::State tstate = chatfield.send_cursor(cursorpos, clicking);

			if (tstate == Cursor::State::CLICKING || tstate == Cursor::State::CANCLICK)
				return tstate;

			// Don't let clicks outside the textfield defocus it —
			// only Enter key should close the chatfield
			if (clicking && chatfieldopen && chatfield.get_state() != Textfield::State::FOCUSED)
				chatfield.set_state(Textfield::State::FOCUSED);
		}
		else if (clicking)
		{
			// Clicking the chat input area when chat is closed opens and focuses it
			// Use the chatfield bounds relative to position to check
			Rectangle<int16_t> input_area(
				position + Point<int16_t>(-435, -58),
				position + Point<int16_t>(-40, -35)
			);

			if (input_area.contains(cursorpos))
			{
				toggle_chatfield(true);
				return Cursor::State::IDLE;
			}
		}

		auto chattop_rect = Rectangle<int16_t>(0, 502, getchattop(), getchattop() + 6);
		bool contains = chattop_rect.contains(cursorpos);

		if (dragchattop)
		{
			if (clicking)
			{
				int16_t ydelta = cursorpos.y() - getchattop();

				while (ydelta > 0 && chatrows > MINCHATROWS)
				{
					chatrows--;
					ydelta -= CHATROWHEIGHT;
				}

				while (ydelta < 0 && chatrows < MAXCHATROWS)
				{
					chatrows++;
					ydelta += CHATROWHEIGHT;
				}

				chatbox = ColorBox(502, 1 + chatrows * CHATROWHEIGHT, Color::Name::BLACK, 0.6f);
				slider.setrows(rowpos, chatrows, rowmax);
				slider.setvertical(Range<int16_t>(0, CHATROWHEIGHT * chatrows - 14));

				return Cursor::State::CLICKING;
			}
			else
			{
				dragchattop = false;
			}
		}
		else if (contains)
		{
			if (clicking)
			{
				dragchattop = true;
				return Cursor::State::CLICKING;
			}
			else
			{
				return Cursor::State::CANCLICK;
			}
		}

		return Cursor::State::IDLE;
	}

	void UIChatBar::send_line(const std::string& line, LineType type)
	{
		rowmax++;
		rowpos = rowmax;

		slider.setrows(rowpos, chatrows, rowmax);

		Color::Name color;

		switch (type)
		{
		case RED:
			color = Color::Name::RED;
			break;
		case BLUE:
			color = Color::Name::BLUE;
			break;
		case YELLOW:
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

	void UIChatBar::send_chatline(const std::string& line, LineType type)
	{
		send_line(line, type);
	}

	void UIChatBar::display_message(Messages::Type line, UIChatBar::LineType type)
	{
		if (message_cooldowns[line] > 0)
			return;

		std::string message = Messages::messages[line];
		send_line(message, type);

		message_cooldowns[line] = MESSAGE_COOLDOWN;
	}

	void UIChatBar::toggle_chat()
	{
		set_chat_open(!chatopen);
	}

	void UIChatBar::toggle_chatfield()
	{
		chatfieldopen = !chatfieldopen;
		toggle_chatfield(chatfieldopen);
	}

	void UIChatBar::toggle_chatfield(bool open)
	{
		static FILE* dbg = fopen("C:\\Users\\rdiol\\OpenStory2\\wz\\ui.txt", "a");
		if (dbg) { fprintf(dbg, "[toggle_chatfield] open=%d (was %d) chatopen=%d\n", open, chatfieldopen, chatopen); fflush(dbg); }
		chatfieldopen = open;

		if (chatfieldopen)
		{
			if (!chatopen)
				set_chat_open(true);

			// set_state(FOCUSED) internally calls UI::get().focus_textfield()
			// so we must NOT call focus_textfield again — the double call
			// would reset the state back to NORMAL.
			chatfield.set_state(Textfield::State::FOCUSED);
		}
		else
		{
			chatfield.set_state(chatopen ? Textfield::State::NORMAL : Textfield::State::DISABLED);
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

	void UIChatBar::set_chat_target(ChatTarget target)
	{
		if (target >= CHT_ALL && target < NUM_TARGETS)
			chattarget = target;
	}

	void UIChatBar::cycle_chat_target()
	{
		chattarget = static_cast<ChatTarget>((chattarget + 1) % NUM_TARGETS);
	}

	void UIChatBar::set_pending_party_invite(int32_t in_party_id, const std::string& inviter)
	{
		pending_party_invite_id = in_party_id;
		pending_party_inviter = inviter;
	}

	void UIChatBar::clear_pending_party_invite()
	{
		pending_party_invite_id = -1;
		pending_party_inviter.clear();
	}

	void UIChatBar::set_party_state(int32_t in_party_id, int32_t leader_id, const std::vector<PartyMember>& members)
	{
		party_id = in_party_id;
		party_leader_id = leader_id;
		party_members = members;

		if (pending_party_invite_id == in_party_id)
			clear_pending_party_invite();
	}

	void UIChatBar::clear_party_state()
	{
		party_id = -1;
		party_leader_id = -1;
		party_members.clear();
	}

	void UIChatBar::set_party_leader(int32_t leader_id)
	{
		party_leader_id = leader_id;
	}

	void UIChatBar::update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp)
	{
		for (PartyMember& member : party_members)
		{
			if (member.id == cid)
			{
				member.hp = hp;
				member.max_hp = max_hp;
				return;
			}
		}
	}

	int32_t UIChatBar::get_party_id() const
	{
		return party_id;
	}

	int32_t UIChatBar::get_party_leader_id() const
	{
		return party_leader_id;
	}

	int32_t UIChatBar::get_pending_party_invite_id() const
	{
		return pending_party_invite_id;
	}

	const std::string& UIChatBar::get_pending_party_inviter() const
	{
		return pending_party_inviter;
	}

	const std::vector<UIChatBar::PartyMember>& UIChatBar::get_party_members() const
	{
		return party_members;
	}

	int32_t UIChatBar::resolve_party_member_id(const std::string& token) const
	{
		int32_t member_id = 0;
		if (parse_int32(token, member_id))
			return member_id;

		std::string lowered = lowercase(token);
		for (const PartyMember& member : party_members)
		{
			if (lowercase(member.name) == lowered)
				return member.id;
		}

		return 0;
	}

	bool UIChatBar::handle_party_command(const std::string& message)
	{
		if (message.empty() || message[0] != '/')
			return false;

		std::istringstream whole(message);
		std::string command;
		whole >> command;
		command = lowercase(command);

		std::string argument_block;
		std::getline(whole, argument_block);
		argument_block = trim(argument_block);

		if (command == "/pt")
		{
			if (argument_block.empty())
			{
				send_line("[Party] Usage: /pt <message>", YELLOW);
				return true;
			}

			send_party_message(argument_block);
			return true;
		}

		if (command != "/party" && command != "/p")
			return false;

		if (argument_block.empty())
		{
			send_line("[Party] /party create | leave | invite <name> | join <partyId>", YELLOW);
			send_line("[Party] /party expel <name|id> | leader <name|id> | accept | deny | list", YELLOW);
			return true;
		}

		std::istringstream args(argument_block);
		std::string action;
		args >> action;
		action = lowercase(action);

		std::string argument;
		std::getline(args, argument);
		argument = trim(argument);

		if (action == "create")
		{
			CreatePartyPacket().dispatch();
			send_line("[Party] Sent create request.", YELLOW);
			return true;
		}

		if (action == "leave")
		{
			LeavePartyPacket().dispatch();
			send_line("[Party] Sent leave request.", YELLOW);
			return true;
		}

		if (action == "join")
		{
			int32_t join_party_id = 0;
			if (!parse_int32(argument, join_party_id))
			{
				send_line("[Party] Usage: /party join <partyId>", YELLOW);
				return true;
			}

			JoinPartyPacket(join_party_id).dispatch();
			send_line("[Party] Sent join request.", YELLOW);
			return true;
		}

		if (action == "invite")
		{
			if (argument.empty())
			{
				send_line("[Party] Usage: /party invite <name>", YELLOW);
				return true;
			}

			InviteToPartyPacket(argument).dispatch();
			send_line("[Party] Sent invite to " + argument + ".", YELLOW);
			return true;
		}

		if (action == "expel" || action == "kick")
		{
			int32_t member_id = resolve_party_member_id(argument);
			if (member_id <= 0)
			{
				send_line("[Party] Usage: /party expel <name|id>", YELLOW);
				return true;
			}

			ExpelFromPartyPacket(member_id).dispatch();
			send_line("[Party] Sent expel request.", YELLOW);
			return true;
		}

		if (action == "leader" || action == "lead")
		{
			int32_t member_id = resolve_party_member_id(argument);
			if (member_id <= 0)
			{
				send_line("[Party] Usage: /party leader <name|id>", YELLOW);
				return true;
			}

			ChangePartyLeaderPacket(member_id).dispatch();
			send_line("[Party] Sent leadership transfer request.", YELLOW);
			return true;
		}

		if (action == "accept")
		{
			if (pending_party_invite_id <= 0)
			{
				send_line("[Party] There is no pending invitation.", YELLOW);
				return true;
			}

			JoinPartyPacket(pending_party_invite_id).dispatch();
			send_line("[Party] Sent invitation accept request.", YELLOW);
			clear_pending_party_invite();
			return true;
		}

		if (action == "deny")
		{
			if (pending_party_inviter.empty())
			{
				send_line("[Party] There is no pending invitation.", YELLOW);
				return true;
			}

			DenyPartyInvitePacket(pending_party_inviter).dispatch();
			send_line("[Party] Declined invitation from " + pending_party_inviter + ".", YELLOW);
			clear_pending_party_invite();
			return true;
		}

		if (action == "list")
		{
			if (party_members.empty())
			{
				send_line("[Party] No cached party members.", YELLOW);
				return true;
			}

			send_line("[Party] Current members:", YELLOW);
			for (const PartyMember& member : party_members)
			{
				std::string leader_tag = member.id == party_leader_id ? " (leader)" : "";
				send_line(" - " + member.name + " Lv." + std::to_string(member.level) + leader_tag, WHITE);
			}
			return true;
		}

		send_line("[Party] Unknown command. Use /party for help.", YELLOW);
		return true;
	}

	void UIChatBar::send_targeted_message(const std::string& target, const std::string& message)
	{
		send_line("[To " + target + "] " + message, WHITE);
	}

	void UIChatBar::send_party_message(const std::string& message)
	{
		std::list<int32_t> recipients;
		MultiChatPacket(MultiChatPacket::PARTY, recipients, message).dispatch();
		send_targeted_message("Party", message);
	}

	void UIChatBar::send_chat_message(const std::string& message)
	{
		std::list<int32_t> recipients;

		switch (chattarget)
		{
		case CHT_ALL:
			GeneralChatPacket(message, true).dispatch();
			break;
		case CHT_BUDDY:
			MultiChatPacket(MultiChatPacket::BUDDY, recipients, message).dispatch();
			send_targeted_message("Buddy", message);
			break;
		case CHT_GUILD:
			MultiChatPacket(MultiChatPacket::GUILD, recipients, message).dispatch();
			send_targeted_message("Guild", message);
			break;
		case CHT_ALLIANCE:
			MultiChatPacket(MultiChatPacket::ALLIANCE, recipients, message).dispatch();
			send_targeted_message("Alliance", message);
			break;
		case CHT_SQUAD:
			send_party_message(message);
			break;
		case CHT_PARTY:
		default:
			send_party_message(message);
			break;
		}
	}

	int16_t UIChatBar::getchattop() const
	{
		return position.y() - chatrows * CHATROWHEIGHT - CHATYOFFSET;
	}
}
