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
#include "../../Constants.h"
#include "../../Graphics/Geometry.h"

#include <cmath>

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

		// dot0/dot1/list0/list1 don't exist at UIWindow2.img/UtilDlgEx in v83
		// and the node lookup was picking up unrelated bitmaps ("QUESTS IN
		// PROGRESS" stickers) that rendered over each selection row. Try
		// the v83 path (UIWindow.img/UtilDlgEx) first and only accept nodes
		// that actually resolve to something.
		nl::node UtilDlgEx_v83 = nl::nx::ui["UIWindow.img"]["UtilDlgEx"];

		auto pick = [](nl::node a, nl::node b) -> nl::node
		{
			return a ? a : b;
		};

		dot_normal = pick(UtilDlgEx_v83["dot0"], UtilDlgEx["dot0"]);
		dot_hovered = pick(UtilDlgEx_v83["dot1"], UtilDlgEx["dot1"]);
		list_normal = pick(UtilDlgEx_v83["list0"], UtilDlgEx["list0"]);
		list_hovered = pick(UtilDlgEx_v83["list1"], UtilDlgEx["list1"]);

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

		// Quest-state icons painted next to SENDSIMPLE option rows:
		//   QuestIcon/0 = available (!), /1 = in-progress (?), /2 = complete
		if (nl::node qi = nl::nx::ui["UIWindow.img"]["QuestIcon"])
		{
			if (qi["0"] && qi["0"].size() > 0) mark_available  = Animation(qi["0"]);
			if (qi["1"] && qi["1"].size() > 0) mark_in_progress = Animation(qi["1"]);
			if (qi["2"] && qi["2"].size() > 0) mark_complete   = Animation(qi["2"]);
		}

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
			if ((type == TalkType::SENDSIMPLE || type == TalkType::SENDDIMENSIONALMIRROR) && !selections.empty())
			{
				for (size_t i = 0; i < selections.size(); i++)
				{
					auto& sel = selections[i];
					// Measure the rendered height of everything before this
					// option so word-wrapped intro text produces the right
					// Y position (sel.line counts source newlines, not the
					// visual lines the layout produces).
					std::string pref_escaped;
					pref_escaped.reserve(sel.prefix.size() + 8);
					for (char c : sel.prefix)
					{
						if (c == '\n') pref_escaped += "\\n";
						else pref_escaped += c;
					}
					Text prefix_text(Text::Font::A12M, Text::Alignment::LEFT,
						Color::Name::DARKGREY, Text::Background::NONE,
						pref_escaped, 320, true);
					// height() returns the baseline y of the next row after
					// the prefix; subtract linespace so row_y points at the
					// TOP of the option's row (where list_bg/dot sprites
					// would normally anchor).
					int16_t row_y = text_y + prefix_text.height() - line_h;
					Point<int16_t> row_pos(text_x - 36, row_y);
					Point<int16_t> dot_pos(text_x - 14, row_y + 3);

					bool is_hovered = (static_cast<int32_t>(i) == hovered_selection);

					// TEMPORARILY DISABLED — suspected to be drawing wrong
					// NX sprites ("QUEST IN PROGRESS" banner) over selection
					// rows. If the red banners vanish after this patch the
					// culprit is UtilDlgEx/list0/dot0 resolution; if they
					// remain, another UI element is responsible.
					//
					// if (is_hovered)
					//     list_hovered.draw(DrawArgument(row_pos));
					// else
					//     list_normal.draw(DrawArgument(row_pos));
					//
					// if (is_hovered)
					//     dot_hovered.draw(DrawArgument(dot_pos));
					// else
					//     dot_normal.draw(DrawArgument(dot_pos));

					// Small grey bullet as a stand-in for the selection dot
					ColorBox bullet(4, 4, Color::Name::DUSTYGRAY, 1.0f);
					bullet.draw(DrawArgument(Point<int16_t>(dot_pos.x() + 1, dot_pos.y() + 1)));

					// Quest-state icon (available/in-progress/complete) if
					// the server tagged this option. Drawn just to the
					// right of the bullet, before the option's text.
					const Animation* mark_anim = nullptr;
					switch (sel.mark)
					{
					case QuestMark::AVAILABLE:   mark_anim = &mark_available;  break;
					case QuestMark::IN_PROGRESS: mark_anim = &mark_in_progress; break;
					case QuestMark::COMPLETE:    mark_anim = &mark_complete;   break;
					default: break;
					}
					if (mark_anim && mark_anim->get_dimensions().x() > 0)
					{
						Point<int16_t> mark_pos(
							text_x - 6, row_y + line_h / 2);
						mark_anim->draw(DrawArgument(mark_pos), 1.0f);
					}

					// Hover indicator: a thin light-blue underline on the
					// hovered option (subtle enough not to clash with the
					// blue option text but visible enough to track).
					if (is_hovered)
					{
						ColorBox underline(
							static_cast<int16_t>(sel.text.empty() ? 120
								: static_cast<int16_t>(sel.text.size() * 7)),
							2,
							Color::Name::LIGHTBLUE, 0.75f);
						underline.draw(DrawArgument(
							Point<int16_t>(text_x, row_y + line_h - 2)));
					}
				}
			}

			text.draw(position + Point<int16_t>(166, 48 - y_adj));
		}

		// Draw text input field for SENDGETTEXT / SENDGETNUMBER
		if (type == TalkType::SENDGETTEXT || type == TalkType::SENDGETNUMBER)
			input_field.draw(position);

		// Draw SENDSTYLE row list
		if (type == TalkType::SENDSTYLE && !style_ids.empty())
		{
			int16_t y_adj = height - min_height;
			int16_t text_x = position.x() + 130;
			int16_t text_y = position.y() + 48 - y_adj;
			int16_t line_h = 16;

			for (size_t i = 0; i < style_ids.size(); i++)
			{
				Point<int16_t> row_pos(text_x - 36, text_y + static_cast<int16_t>(i) * line_h);
				bool is_hovered = (static_cast<int32_t>(i) == style_hovered);
				if (is_hovered)
					list_hovered.draw(DrawArgument(row_pos));
				else
					list_normal.draw(DrawArgument(row_pos));

				Point<int16_t> dot_pos(text_x - 14, text_y + static_cast<int16_t>(i) * line_h + 3);
				if (is_hovered)
					dot_hovered.draw(DrawArgument(dot_pos));
				else
					dot_normal.draw(DrawArgument(dot_pos));

				const std::string& label = (i < style_names.size())
					? style_names[i] : std::to_string(style_ids[i]);

				Text row_text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK,
					label, 320);
				row_text.draw(Point<int16_t>(text_x, text_y + static_cast<int16_t>(i) * line_h));
			}
		}

	}

	void UINpcTalk::update()
	{
		UIElement::update();

		++hover_pulse_tick;

		// Advance the quest-mark sprites so their animation loops play
		// while the NPC selection dialog is on screen.
		mark_available.update();
		mark_in_progress.update();
		mark_complete.update();

		if (type == TalkType::SENDGETTEXT || type == TalkType::SENDGETNUMBER)
			input_field.update(position);

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
			case TalkType::SENDNEXT:
			{
				// msgtype 5 — Next only; close cancels, Next advances.
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::NEXT:
						NpcTalkMorePacket(type, 1).dispatch();
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
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::OK:
						NpcTalkMorePacket(input_field.get_text()).dispatch();
						break;
				}
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
					{
						int32_t value = num_default;
						try
						{
							value = std::stoi(input_field.get_text());
						}
						catch (...) {}

						if (num_min != num_max)
						{
							if (value < num_min) value = num_min;
							if (value > num_max) value = num_max;
						}

						NpcTalkNumberPacket(value).dispatch();
						break;
					}
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
			case TalkType::SENDSTYLE:
			{
				// msgtype 7 — style picker; selection handled in send_cursor,
				// OK confirms the currently hovered/selected style.
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::OK:
					{
						int32_t idx = (style_hovered >= 0) ? style_hovered : 0;
						NpcTalkStylePacket(idx).dispatch();
						break;
					}
				}

				break;
			}
			case TalkType::SENDDIMENSIONALMIRROR:
			{
				// msgtype 14 — PQ list; close only, selection dispatched in send_cursor
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
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

		// Handle text input field cursor for SENDGETTEXT / SENDGETNUMBER
		if (type == TalkType::SENDGETTEXT || type == TalkType::SENDGETNUMBER)
		{
			Cursor::State tstate = input_field.send_cursor(cursor_relative, clicked);
			if (tstate != Cursor::State::IDLE)
				return tstate;
		}

		if (show_slider && slider.isenabled())
			if (Cursor::State sstate = slider.send_cursor(cursor_relative, clicked))
				return sstate;

		// Handle SENDSTYLE row clicks (just marks the hovered style).
		if (type == TalkType::SENDSTYLE && !style_ids.empty())
		{
			int16_t y_adj = height - min_height;
			int16_t text_x = position.x() + 130;
			int16_t text_y = position.y() + 48 - y_adj;
			int16_t text_w = 320;
			int16_t line_h = 16;

			style_hovered = -1;
			for (size_t i = 0; i < style_ids.size(); i++)
			{
				int16_t opt_y = text_y + static_cast<int16_t>(i) * line_h;
				if (cursorpos.x() >= text_x && cursorpos.x() <= text_x + text_w &&
					cursorpos.y() >= opt_y && cursorpos.y() <= opt_y + line_h)
				{
					style_hovered = static_cast<int32_t>(i);
					if (clicked)
					{
						deactivate();
						NpcTalkStylePacket(static_cast<int32_t>(i)).dispatch();
						return Cursor::State::IDLE;
					}
					return Cursor::State::CANCLICK;
				}
			}
		}

		// Handle SENDSIMPLE / SENDDIMENSIONALMIRROR text option clicks.
		// Both use embedded #L<i># codes in the text; only the reply packet differs.
		if ((type == TalkType::SENDSIMPLE || type == TalkType::SENDDIMENSIONALMIRROR) && !selections.empty())
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
				std::string pref_escaped;
				pref_escaped.reserve(sel.prefix.size() + 8);
				for (char c : sel.prefix)
				{
					if (c == '\n') pref_escaped += "\\n";
					else pref_escaped += c;
				}
				Text prefix_text(Text::Font::A12M, Text::Alignment::LEFT,
					Color::Name::DARKGREY, Text::Background::NONE,
					pref_escaped, 320, true);

				// prefix_text.height() is the baseline-y of where THIS
				// option's glyphs render. Glyphs extend roughly from
				// (baseline - 14) to baseline — define the hit-test row
				// around that range so clicking the visible text registers.
				int16_t baseline = text_y + prefix_text.height();
				int16_t row_top  = baseline - line_h;
				int16_t row_bot  = baseline + 2;

				if (cursorpos.x() >= text_x && cursorpos.x() <= text_x + text_w &&
					cursorpos.y() >= row_top && cursorpos.y() <= row_bot)
				{
					hovered_selection = static_cast<int32_t>(i);

					if (clicked)
					{
						hovered_selection = -1;
						deactivate();
						if (type == TalkType::SENDDIMENSIONALMIRROR)
							NpcTalkMirrorPacket(sel.index).dispatch();
						else
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

	void UINpcTalk::send_scroll(double yoffset)
	{
		if (show_slider)
			slider.send_scroll(yoffset);
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
		case TalkType::SENDNEXT:
		case TalkType::SENDSTYLE:
		case TalkType::SENDACCEPTDECLINE:
		case TalkType::SENDDIMENSIONALMIRROR:
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
			// Strip any ASCII control char below 0x20 except \n and \t —
			// otherwise the A12M font renders them as a box placeholder
			// (e.g. "X [] C [] Z [] Space"). This catches \r, \0, \x0b,
			// \x0c and any other sub-space bytes the server may emit.
			unsigned char uc = static_cast<unsigned char>(tx[i]);
			if (uc < 0x20 && uc != '\n' && uc != '\t')
				continue;

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
						std::string name = (std::string)nl::nx::string["Npc.img"][id_str]["name"];
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

						std::string name = (std::string)nl::nx::string["Map.img"][padded]["mapName"];
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
						std::string name = (std::string)nl::nx::string["Mob.img"][id_str]["name"];
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

			// Detect quest state tags the server embeds in the option
			// text ("(Start) …", "(In progress) …", "(Complete) …") and
			// map them to the QuestMark sprite we'll draw next to the
			// option. Tags are case-insensitive and can be bracketed by
			// parentheses or square brackets.
			QuestMark mark = QuestMark::NONE;
			{
				std::string lc;
				for (char c : clean_option) lc.push_back((char)std::tolower((unsigned char)c));

				auto consume_tag = [&](const std::string& tag, QuestMark m) -> bool
				{
					size_t pos = lc.find(tag);
					if (pos == std::string::npos) return false;
					mark = m;
					// Strip the tag from the visible text so the row just
					// shows "QuestName" next to the icon.
					clean_option.erase(pos, tag.size());
					lc.erase(pos, tag.size());
					// Also strip leading whitespace that remains.
					while (!clean_option.empty()
						&& (clean_option.front() == ' ' || clean_option.front() == ':'))
					{
						clean_option.erase(clean_option.begin());
						lc.erase(lc.begin());
					}
					return true;
				};

				// Order matters: try completes before starts so "(Complete
				// quest)" doesn't match a naive "start" scan first.
				consume_tag("(complete)", QuestMark::COMPLETE)
					|| consume_tag("[complete]", QuestMark::COMPLETE)
					|| consume_tag("(finish)",   QuestMark::COMPLETE)
					|| consume_tag("[finish]",   QuestMark::COMPLETE)
					|| consume_tag("(ready)",    QuestMark::COMPLETE)
					|| consume_tag("(in progress)", QuestMark::IN_PROGRESS)
					|| consume_tag("[in progress]", QuestMark::IN_PROGRESS)
					|| consume_tag("(start)",    QuestMark::AVAILABLE)
					|| consume_tag("[start]",    QuestMark::AVAILABLE)
					|| consume_tag("(available)", QuestMark::AVAILABLE);
			}

			// Ensure each option starts on its own rendered line.
			if (!result.empty() && result.back() != '\n')
				result += "\n";

			int16_t actual_line = 0;
			for (char c : result)
				if (c == '\n')
					actual_line++;

			// Snapshot the prefix now so draw() can measure visual-row Y
			// using the real Text layout (handles word-wrapped intro lines).
			Selection sel;
			sel.index = sel_index;
			sel.text = clean_option;
			sel.line = actual_line;
			sel.prefix = result;
			sel.mark = mark;
			selections.push_back(std::move(sel));

			// Render the option text in blue so it stands out against the
			// NPC's main dialogue body (DARKGREY). #b turns on blue, #k
			// turns it off; both are recognised by the Text formatter.
			result += "#b";
			result += clean_option;
			result += "#k";
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

		if (type == TalkType::SENDSIMPLE || type == TalkType::SENDDIMENSIONALMIRROR)
		{
			formatted_text = parse_simple_selections(format_text(tx, npcid));
			draw_text = false; // Show all text immediately for selection menus
		}
		else
		{
			formatted_text = format_text(tx, npcid);
		}

		// The Text/LayoutBuilder only treats the two-char escape "\\n" as a
		// line break; a raw '\n' byte (0x0A) renders as a missing-glyph box.
		// Convert real newlines to the escape and enable formatted=true so
		// multi-line NPC dialogue lays out correctly.
		{
			std::string escaped;
			escaped.reserve(formatted_text.size() + 8);
			for (char c : formatted_text)
			{
				if (c == '\n')
					escaped += "\\n";
				else
					escaped += c;
			}
			formatted_text = std::move(escaped);
		}

		text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::DARKGREY,
			Text::Background::NONE, formatted_text, 320, true);

		int16_t text_height = text.height();

		if (type != TalkType::SENDSIMPLE && type != TalkType::SENDDIMENSIONALMIRROR)
			text.change_text("");

		if (speakerbyte == 0)
		{
			std::string strid = std::to_string(npcid);
			if (strid.size() < 7)
				strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");

			speaker = nl::nx::npc[strid]["stand"]["0"];

			std::string namestr = (std::string)nl::nx::string["Npc.img"][std::to_string(npcid)]["name"];
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
				// endBytes on the wire are [hasPrev][hasNext]; read_short is
				// little-endian so prev is in the LOW byte and next in the HIGH byte.
				bool has_prev = style & 0xFF;
				bool has_next = (style >> 8) & 0xFF;

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
			case TalkType::SENDNEXT:
			{
				// Mode 5 — "Next only" flavour of SENDSAY. No Prev button;
				// server doesn't expect backwards navigation here.
				buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::NEXT]->set_active(true);
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
			case TalkType::SENDGETTEXT:
			{
				// Text input field below the NPC text
				int16_t field_y = y_cord - 25;
				input_field = Textfield(
					Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
					Rectangle<int16_t>(Point<int16_t>(166, field_y), Point<int16_t>(460, field_y + 18)),
					50
				);
				input_field.set_state(Textfield::State::FOCUSED);

				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDGETNUMBER:
			{
				// Number input field (same widget as GETTEXT, prefilled with default)
				int16_t field_y = y_cord - 25;
				input_field = Textfield(
					Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,
					Rectangle<int16_t>(Point<int16_t>(166, field_y), Point<int16_t>(460, field_y + 18)),
					16
				);
				input_field.change_text(std::to_string(num_default));
				input_field.set_state(Textfield::State::FOCUSED);

				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDSIMPLE:
			{
				// Only close button — options are clicked in the text area
				break;
			}
			case TalkType::SENDSTYLE:
			{
				// Style picker (hair/face/skin). Close button stays; style rows
				// are clicked in the text area. OK confirms current selection.
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDDIMENSIONALMIRROR:
			{
				// PQ entrance list — close only; rows are clicked in the text area.
				break;
			}
			default:
			{
				break;
			}
		}

		int16_t vw = Constants::Constants::get().get_viewwidth();
		int16_t vh = Constants::Constants::get().get_viewheight();
		position = Point<int16_t>(vw / 2 - top.width() / 2, vh / 2 - height / 2);
		dimension = Point<int16_t>(top.width(), height + 120);
	}

	void UINpcTalk::set_number_bounds(int32_t def, int32_t lo, int32_t hi)
	{
		num_default = def;
		num_min = lo;
		num_max = hi;

		if (type == TalkType::SENDGETNUMBER)
			input_field.change_text(std::to_string(def));
	}

	void UINpcTalk::set_style_ids(std::vector<int32_t> ids)
	{
		style_ids = std::move(ids);
		style_hovered = style_ids.empty() ? -1 : 0;

		// Resolve each style id to a human-readable name so the list shows
		// "Black Amour" instead of "30030". Categories by id prefix:
		//   20000-29999 → Face, 30000-39999 → Hair, otherwise → Skin/raw id.
#ifdef USE_NX
		style_names.clear();
		style_names.reserve(style_ids.size());

		for (int32_t id : style_ids)
		{
			std::string idstr = std::to_string(id);
			std::string nm;

			if (id >= 30000 && id < 40000)
				nm = (std::string)nl::nx::string["Eqp.img"]["Eqp"]["Hair"][idstr]["name"];
			else if (id >= 20000 && id < 30000)
				nm = (std::string)nl::nx::string["Eqp.img"]["Eqp"]["Face"][idstr]["name"];

			if (nm.empty())
				nm = idstr;

			style_names.push_back(std::move(nm));
		}
#else
		style_names.clear();
		style_names.reserve(style_ids.size());
		for (int32_t id : style_ids)
			style_names.push_back(std::to_string(id));
#endif
	}

}