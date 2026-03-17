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
#include "UINpcTalk.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/NpcInteractionPackets.h"

#include "../../Data/ItemData.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UINpcTalk::UINpcTalk() : offset(0), unitrows(0), rowmax(0), show_slider(false), draw_text(false), formatted_text(""), formatted_text_pos(0), timestep(0), selection_top(0), hovered_selection(-1)
	{
		nl::node UtilDlgEx = nl::nx::ui["UIWindow2.img"]["UtilDlgEx"];

		top = UtilDlgEx["t"];
		fill = UtilDlgEx["c"];
		bottom = UtilDlgEx["s"];
		nametag = UtilDlgEx["bar"];

		dot_normal = UtilDlgEx["dot0"];
		dot_hovered = UtilDlgEx["dot1"];
		list_normal = UtilDlgEx["list0"];
		list_hovered = UtilDlgEx["list1"];

		min_height = 8 * fill.height() + 14;

		buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(UtilDlgEx["BtClose"]);
		buttons[Buttons::NEXT] = std::make_unique<MapleButton>(UtilDlgEx["BtNext"]);
		buttons[Buttons::NO] = std::make_unique<MapleButton>(UtilDlgEx["BtNo"]);
		buttons[Buttons::OK] = std::make_unique<MapleButton>(UtilDlgEx["BtOK"]);
		buttons[Buttons::PREV] = std::make_unique<MapleButton>(UtilDlgEx["BtPrev"]);
		buttons[Buttons::QNO] = std::make_unique<MapleButton>(UtilDlgEx["BtQNo"]);
		buttons[Buttons::QYES] = std::make_unique<MapleButton>(UtilDlgEx["BtQYes"]);
		buttons[Buttons::YES] = std::make_unique<MapleButton>(UtilDlgEx["BtYes"]);
		buttons[Buttons::QSTART] = std::make_unique<MapleButton>(UtilDlgEx["BtQStart"]);
		buttons[Buttons::QAFTER] = std::make_unique<MapleButton>(UtilDlgEx["BtQAfter"]);
		buttons[Buttons::QCYES] = std::make_unique<MapleButton>(UtilDlgEx["BtQCYes"]);
		buttons[Buttons::QCNO] = std::make_unique<MapleButton>(UtilDlgEx["BtQCNo"]);
		buttons[Buttons::QGIVEUP] = std::make_unique<MapleButton>(UtilDlgEx["BtQGiveup"]);

		name = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);

		onmoved = [&](bool upwards)
		{
			int16_t shift = upwards ? -unitrows : unitrows;
			bool above = offset + shift >= 0;
			bool below = offset + shift <= rowmax - unitrows;

			if (above && below)
				offset += shift;
		};

		UI::get().remove_textfield();
	}

	void UINpcTalk::draw(float inter) const
	{
		Point<int16_t> drawpos = position;
		top.draw(drawpos);
		drawpos.shift_y(top.height());
		fill.draw(DrawArgument(drawpos, Point<int16_t>(0, height)));
		drawpos.shift_y(height);
		bottom.draw(drawpos);
		drawpos.shift_y(bottom.height());

		UIElement::draw(inter);

		int16_t speaker_y = (top.height() + height + bottom.height()) / 2;
		Point<int16_t> speaker_pos = position + Point<int16_t>(22, 11 + speaker_y);
		Point<int16_t> center_pos = speaker_pos + Point<int16_t>(nametag.width() / 2, 0);

		speaker.draw(DrawArgument(center_pos, true));
		nametag.draw(speaker_pos);
		name.draw(center_pos + Point<int16_t>(0, -4));

		if (show_slider)
		{
			int16_t text_min_height = position.y() + top.height() - 1;
			text.draw(position + Point<int16_t>(162, 19 - offset * 400), Range<int16_t>(text_min_height, text_min_height + height - 18));
			slider.draw(position);
		}
		else
		{
			int16_t y_adj = height - min_height;
			int16_t text_x = position.x() + 166;
			int16_t text_y = position.y() + 48 - y_adj;
			int16_t line_h = 16;

			// Draw selection list backgrounds and dots for SENDSIMPLE
			if (type == TalkType::SENDSIMPLE && !selections.empty())
			{
				for (size_t i = 0; i < selections.size(); i++)
				{
					auto& sel = selections[i];
					int16_t row_y = text_y + sel.line * line_h;
					Point<int16_t> row_pos(text_x - 36, row_y);
					Point<int16_t> dot_pos(text_x - 14, row_y + 3);

					bool is_hovered = (static_cast<int32_t>(i) == hovered_selection);

					// Draw list row background
					if (is_hovered)
						list_hovered.draw(DrawArgument(row_pos));
					else
						list_normal.draw(DrawArgument(row_pos));

					// Draw selection dot/bullet
					if (is_hovered)
						dot_hovered.draw(DrawArgument(dot_pos));
					else
						dot_normal.draw(DrawArgument(dot_pos));
				}
			}

			text.draw(position + Point<int16_t>(166, 48 - y_adj));
		}
	}

	void UINpcTalk::update()
	{
		UIElement::update();

		if (draw_text)
		{
			if (timestep > 4)
			{
				if (formatted_text_pos < formatted_text.size())
				{
					std::string t = text.get_text();
					char c = formatted_text[formatted_text_pos];

					text.change_text(t + c);

					formatted_text_pos++;
					timestep = 0;
				}
				else
				{
					draw_text = false;
				}
			}
			else
			{
				timestep++;
			}
		}
	}

	Button::State UINpcTalk::button_pressed(uint16_t buttonid)
	{
		deactivate();

		switch (type)
		{
			case TalkType::SENDSAY:
			{
				// msgtype 0 — OK/Next/Prev
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::OK:
					case Buttons::NEXT:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
					case Buttons::PREV:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDYESNO:
			{
				// msgtype 1
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::NO:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::YES:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDACCEPTDECLINE:
			{
				// msgtype 12 — quest accept/decline
				switch (buttonid)
				{
					case Buttons::CLOSE:
					case Buttons::QAFTER:
					case Buttons::QCNO:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::QNO:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::QYES:
					case Buttons::QSTART:
					case Buttons::QCYES:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
					case Buttons::QGIVEUP:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDGETTEXT:
			{
				// msgtype 2 — text input (not yet implemented)
				break;
			}
			case TalkType::SENDGETNUMBER:
			{
				// msgtype 3
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::OK:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDSIMPLE:
			{
				// msgtype 4 — close only; selections are handled in send_cursor
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
				}

				break;
			}
			default:
			{
				break;
			}
		}

		return Button::State::NORMAL;
	}

	Cursor::State UINpcTalk::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursor_relative = cursorpos - position;

		if (show_slider && slider.isenabled())
			if (Cursor::State sstate = slider.send_cursor(cursor_relative, clicked))
				return sstate;

		// Handle SENDSIMPLE text option clicks
		if (type == TalkType::SENDSIMPLE && !selections.empty())
		{
			// Text area starts at position + (166, 48 - y_adj) for non-slider
			int16_t y_adj = height - min_height;
			int16_t text_x = position.x() + 130;
			int16_t text_y = position.y() + 48 - y_adj;
			int16_t text_w = 320;
			int16_t line_h = 16; // approximate line height for A12M font

			int32_t prev_hovered = hovered_selection;
			hovered_selection = -1;

			for (size_t i = 0; i < selections.size(); i++)
			{
				auto& sel = selections[i];
				int16_t opt_y = text_y + sel.line * line_h;

				if (cursorpos.x() >= text_x && cursorpos.x() <= text_x + text_w &&
					cursorpos.y() >= opt_y && cursorpos.y() <= opt_y + line_h)
				{
					hovered_selection = static_cast<int32_t>(i);

					if (clicked)
					{
						hovered_selection = -1;
						deactivate();
						NpcTalkMorePacket(sel.index).dispatch();
						return Cursor::State::IDLE;
					}
					else
					{
						return Cursor::State::CANCLICK;
					}
				}
			}
		}

		Cursor::State estate = UIElement::send_cursor(clicked, cursorpos);

		if (estate == Cursor::State::CLICKING && clicked && draw_text)
		{
			draw_text = false;
			text.change_text(formatted_text);
		}

		return estate;
	}

	void UINpcTalk::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			deactivate();

			NpcTalkMorePacket(type, 0).dispatch();
		}
	}

	UIElement::Type UINpcTalk::get_type() const
	{
		return TYPE;
	}

	bool UINpcTalk::is_valid_type(int8_t value)
	{
		switch (value)
		{
		case TalkType::SENDSAY:
		case TalkType::SENDYESNO:
		case TalkType::SENDGETTEXT:
		case TalkType::SENDGETNUMBER:
		case TalkType::SENDSIMPLE:
		case TalkType::SENDSTYLE:
		case TalkType::SENDACCEPTDECLINE:
			return true;
		default:
			return false;
		}
	}

	// NPC text formatting — replaces all # macros with game data
	// Handles: #h# (player name), #p<id># (NPC name), #m<id># (map name),
	//          #o<id># (mob name), #t<id># / #z<id># (item name),
	//          #e...#n (bold on/off), #b #k #r #d #c (color codes — stripped)
	std::string UINpcTalk::format_text(const std::string& tx, const int32_t& npcid)
	{
		std::string result;
		result.reserve(tx.size());

		for (size_t i = 0; i < tx.size(); i++)
		{
			if (tx[i] != '#')
			{
				result += tx[i];
				continue;
			}

			if (i + 1 >= tx.size())
			{
				result += '#';
				continue;
			}

			char code = tx[i + 1];

			// Color/style codes: just skip the marker
			if (code == 'b' || code == 'k' || code == 'r' || code == 'd' ||
				code == 'c' || code == 'e' || code == 'n')
			{
				i++; // skip the code character
				continue;
			}

			// #h# — player name (no ID between markers)
			if (code == 'h')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					result += Stage::get().get_player().get_name();
					i = end; // skip past closing #
					continue;
				}
			}

			// #p<id># — NPC name
			if (code == 'p')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty())
					{
						std::string name = nl::nx::string["Npc.img"][id_str]["name"];
						result += name.empty() ? ("NPC " + id_str) : name;
					}
					i = end;
					continue;
				}
			}

			// #m<id># — map name
			if (code == 'm')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty())
					{
						// Map names: pad to 9 digits, look up in string.nx
						std::string padded = id_str;
						while (padded.size() < 9)
							padded.insert(0, 1, '0');

						std::string name = nl::nx::string["Map.img"][padded]["mapName"];
						result += name.empty() ? ("Map " + id_str) : name;
					}
					i = end;
					continue;
				}
			}

			// #o<id># — mob name
			if (code == 'o')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty())
					{
						std::string name = nl::nx::string["Mob.img"][id_str]["name"];
						result += name.empty() ? ("Mob " + id_str) : name;
					}
					i = end;
					continue;
				}
			}

			// #t<id># or #z<id># — item name
			if (code == 't' || code == 'z')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty())
					{
						try
						{
							int32_t itemid = std::stoi(id_str);
							const ItemData& idata = ItemData::get(itemid);
							result += idata.is_valid() ? idata.get_name() : ("Item " + id_str);
						}
						catch (...)
						{
							result += "Item " + id_str;
						}
					}
					i = end;
					continue;
				}
			}

			// #L, #l — selection markers, keep them for parse_simple_selections
			if (code == 'L' || code == 'l')
			{
				result += '#';
				continue;
			}

			// Unknown code — pass through
			result += '#';
		}

		return result;
	}

	std::string UINpcTalk::parse_simple_selections(const std::string& tx)
	{
		selections.clear();

		std::string result;
		int16_t line = 0;
		size_t pos = 0;

		// Count lines before the selections start (intro text)
		// by scanning for #L which marks the first selection
		size_t first_link = tx.find("#L");
		if (first_link != std::string::npos && first_link > 0)
		{
			std::string intro = tx.substr(0, first_link);

			// Strip color codes from intro
			std::string clean_intro;
			for (size_t i = 0; i < intro.size(); i++)
			{
				if (intro[i] == '#' && i + 1 < intro.size())
				{
					char next = intro[i + 1];
					if (next == 'b' || next == 'k' || next == 'r' || next == 'c')
					{
						i++; // skip the code
						continue;
					}
				}
				clean_intro += intro[i];
			}

			result = clean_intro;

			// Count newlines in intro to track line position
			for (char c : clean_intro)
			{
				if (c == '\n')
					line++;
			}
		}

		// Parse #L<n>#text#l entries
		pos = 0;
		while (pos < tx.size())
		{
			size_t link_start = tx.find("#L", pos);
			if (link_start == std::string::npos)
				break;

			// Read the selection index
			size_t idx_start = link_start + 2;
			size_t idx_end = tx.find('#', idx_start);
			if (idx_end == std::string::npos)
				break;

			int32_t sel_index = 0;
			try { sel_index = std::stoi(tx.substr(idx_start, idx_end - idx_start)); }
			catch (...) { break; }

			// Read the option text until #l
			size_t text_start = idx_end + 1;
			size_t text_end = tx.find("#l", text_start);
			if (text_end == std::string::npos)
				text_end = tx.size();

			std::string option_text = tx.substr(text_start, text_end - text_start);

			// Strip any color codes from option text
			std::string clean_option;
			for (size_t i = 0; i < option_text.size(); i++)
			{
				if (option_text[i] == '#' && i + 1 < option_text.size())
				{
					char next = option_text[i + 1];
					if (next == 'b' || next == 'k' || next == 'r' || next == 'c')
					{
						i++;
						continue;
					}
				}
				clean_option += option_text[i];
			}

			selections.push_back({ sel_index, clean_option, line });

			// Add to display text as blue clickable-looking text
			if (!result.empty() && result.back() != '\n')
				result += "\\r\\n";
			result += "#b" + clean_option + "#k";
			line++;

			pos = text_end + 2; // skip past #l
		}

		return result;
	}

	void UINpcTalk::change_text(int32_t npcid, int8_t msgtype, int16_t style, int8_t speakerbyte, const std::string& tx)
	{
		type = is_valid_type(msgtype) ? static_cast<TalkType>(msgtype) : TalkType::NONE;

		timestep = 0;
		draw_text = true;
		formatted_text_pos = 0;

		if (type == TalkType::SENDSIMPLE)
		{
			formatted_text = parse_simple_selections(format_text(tx, npcid));
			draw_text = false; // Show all text immediately for selection menus
		}
		else
		{
			formatted_text = format_text(tx, npcid);
		}

		text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::DARKGREY, formatted_text, 320);

		int16_t text_height = text.height();

		if (type != TalkType::SENDSIMPLE)
			text.change_text("");

		if (speakerbyte == 0)
		{
			std::string strid = std::to_string(npcid);
			if (strid.size() < 7)
				strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");

			speaker = nl::nx::npc[strid]["stand"]["0"];

			std::string namestr = nl::nx::string["Npc.img"][std::to_string(npcid)]["name"];
			name.change_text(namestr);
		}
		else
		{
			speaker = Texture();
			name.change_text("");
		}

		height = min_height;
		show_slider = false;

		if (text_height > height)
		{
			if (text_height > MAX_HEIGHT)
			{
				height = MAX_HEIGHT;
				show_slider = true;
				rowmax = text_height / 400 + 1;
				unitrows = 1;

				int16_t slider_y = top.height() - 7;
				slider = Slider(Slider::Type::DEFAULT_SILVER, Range<int16_t>(slider_y, slider_y + height - 20), top.width() - 26, unitrows, rowmax, onmoved);
			}
			else
			{
				height = text_height;
			}
		}

		for (auto& button : buttons)
		{
			button.second->set_active(false);
			button.second->set_state(Button::State::NORMAL);
		}

		int16_t y_cord = height + 48;

		buttons[Buttons::CLOSE]->set_position(Point<int16_t>(9, y_cord));
		buttons[Buttons::CLOSE]->set_active(true);

		switch (type)
		{
			case TalkType::SENDSAY:
			{
				// Style bytes: high byte = prev, low byte = next
				bool has_prev = (style >> 8) & 0xFF;
				bool has_next = style & 0xFF;

				if (has_prev && has_next)
				{
					buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
					buttons[Buttons::NEXT]->set_active(true);
					buttons[Buttons::PREV]->set_position(Point<int16_t>(389, y_cord));
					buttons[Buttons::PREV]->set_active(true);
				}
				else if (has_next)
				{
					buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
					buttons[Buttons::NEXT]->set_active(true);
				}
				else
				{
					buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
					buttons[Buttons::OK]->set_active(true);
				}
				break;
			}
			case TalkType::SENDYESNO:
			{
				Point<int16_t> yes_position = Point<int16_t>(389, y_cord);

				buttons[Buttons::YES]->set_position(yes_position);
				buttons[Buttons::YES]->set_active(true);

				buttons[Buttons::NO]->set_position(yes_position + Point<int16_t>(65, 0));
				buttons[Buttons::NO]->set_active(true);
				break;
			}
			case TalkType::SENDACCEPTDECLINE:
			{
				// Quest accept/decline — show quest-specific buttons
				buttons[Buttons::QSTART]->set_position(Point<int16_t>(389, y_cord));
				buttons[Buttons::QSTART]->set_active(true);

				buttons[Buttons::QAFTER]->set_position(Point<int16_t>(389 + 65, y_cord));
				buttons[Buttons::QAFTER]->set_active(true);
				break;
			}
			case TalkType::SENDGETNUMBER:
			{
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDSIMPLE:
			{
				// Only close button — options are clicked in the text area
				break;
			}
			default:
			{
				break;
			}
		}

		position = Point<int16_t>(400 - top.width() / 2, 240 - height / 2);
		dimension = Point<int16_t>(top.width(), height + 120);
	}
}