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

#include <cctype>
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

		// Overlay inline icons reserved by `#v<id>#`, `#i<id>#`,
		// `#q<id>#`, `#s<id>#`, `#f<id>#` macros. LayoutBuilder
		// records each macro's pixel position and kind; we resolve
		// the bitmap from the appropriate NX source and stamp it on
		// top of the reserved 32-px slot.
		auto draw_inline_icons = [&](Point<int16_t> text_origin)
		{
			for (const auto& img : text.images())
			{
				if (img.item_id <= 0) continue;

				Texture tex;
				switch (img.kind)
				{
				case Text::Layout::ImageKind::ITEM:
					tex = ItemData::get(img.item_id).get_icon(true);
					break;
				case Text::Layout::ImageKind::QUEST:
				{
					nl::node qnode = nl::nx::ui["UIWindow.img"]["QuestIcon"][std::to_string(img.item_id)];
					if (qnode)
						tex = Texture(qnode);
					break;
				}
				case Text::Layout::ImageKind::SKILL:
				{
					// Skill icons live under Skill.wz/<job>.img/skill/
					// /<skill_id>/icon. Job is derived from the high
					// digits of the skill id (skillid / 10000).
					int32_t job = img.item_id / 10000;
					std::string job_strid = std::to_string(job);
					while (job_strid.size() < 3) job_strid.insert(0, 1, '0');
					nl::node snode = nl::nx::skill[job_strid + ".img"]["skill"][std::to_string(img.item_id)]["icon"];
					if (snode)
						tex = Texture(snode);
					break;
				}
				case Text::Layout::ImageKind::FACE:
				{
					std::string fid = std::to_string(img.item_id);
					while (fid.size() < 5) fid.insert(0, 1, '0');
					nl::node fnode = nl::nx::character["Face"][fid + ".img"]["default"]["face"];
					if (fnode)
						tex = Texture(fnode);
					break;
				}
				}

				if (!tex.is_valid()) continue;

				Point<int16_t> icon_dims = tex.get_dimensions();
				Point<int16_t> slot_pos = text_origin + img.pos;
				Point<int16_t> centered(
					slot_pos.x() + (img.size - icon_dims.x()) / 2,
					slot_pos.y() + (img.size - icon_dims.y()) / 2);
				tex.draw(DrawArgument(centered));
			}
		};

		if (show_slider)
		{
			int16_t text_min_height = position.y() + top.height() - 1;
			Point<int16_t> text_origin = position + Point<int16_t>(162, 19 - offset * 400);
			text.draw(text_origin, Range<int16_t>(text_min_height, text_min_height + height - 18));
			draw_inline_icons(text_origin);
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

					// Selection bullet — the UtilDlgEx list / dot sprites
					// aren't a selection-row hover; they're used by the
					// quest-progress view (intro text + divider + reqs).
					// Plain grey bullet here keeps the simple-selection
					// list visually clean.
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

					// Translucent hover band so the player can see
					// which option the cursor is on.
					if (is_hovered)
					{
						constexpr int16_t HIGHLIGHT_W = 360;
						constexpr int16_t HIGHLIGHT_H = 16;
						ColorBox band(HIGHLIGHT_W, HIGHLIGHT_H,
							Color::Name::LIGHTBLUE, 0.30f);
						band.draw(DrawArgument(
							Point<int16_t>(text_x - 36, row_y)));
					}
				}
			}

			Point<int16_t> text_origin = position + Point<int16_t>(166, 48 - y_adj);

			// Always draw the NPC text first.
			text.draw(text_origin);
			draw_inline_icons(text_origin);

			quest_banner_hits.clear();

			// Quest-progress banner is disabled for now per user
			// request. Re-enable by flipping this guard to true; the
			// banner-rendering, hover/click hit-tests and the per-quest
			// expanded_quest_ids state are all still wired up below.
			constexpr bool QUEST_BANNER_ENABLED = false;
			if (QUEST_BANNER_ENABLED
				&& type != TalkType::SENDSIMPLE
				&& current_npcid > 0
				&& list_normal.is_valid())
			{
				const auto& quests = Stage::get().get_player().get_quests();
				const auto& started = quests.get_started();
				nl::node quest_check = nl::nx::quest["Check.img"];

				int16_t cur_y = text_origin.y() + text.height() + 6;
				const auto& inv = Stage::get().get_player().get_inventory();

				for (auto& it : started)
				{
					int16_t qid = it.first;
					const std::string& qdata = it.second;
					nl::node check = quest_check[std::to_string(qid)];
					if (!check) continue;
					nl::node sc = check["0"];
					nl::node ec = check["1"];
					int32_t sn = sc ? static_cast<int32_t>((int64_t)sc["npc"]) : 0;
					int32_t en = ec ? static_cast<int32_t>((int64_t)ec["npc"]) : 0;
					if (sn != current_npcid && en != current_npcid)
						continue;

					Point<int16_t> banner_pos(text_origin.x() - 8, cur_y);

					bool hovered = (quest_banner_hovered_qid == qid);
					const Texture& banner_tex =
						(hovered && list_hovered.is_valid())
							? list_hovered : list_normal;
					banner_tex.draw(DrawArgument(banner_pos));

					Point<int16_t> dims = banner_tex.get_dimensions();
					quest_banner_hits.push_back({
						qid,
						Rectangle<int16_t>(banner_pos, banner_pos + dims)
					});
					cur_y += dims.y();

					// If expanded, draw the requirements right under
					// this banner before moving on to the next quest.
					if (expanded_quest_ids.count(qid) && ec)
					{
						int16_t req_y = cur_y + 2;

						size_t qpos = 0;
						for (auto m : ec["mob"])
						{
							int32_t mobid = static_cast<int32_t>((int64_t)m["id"]);
							int32_t need = static_cast<int32_t>((int64_t)m["count"]);
							if (mobid <= 0) continue;
							int32_t have = 0;
							if (qpos + 3 <= qdata.size())
							{
								try { have = std::stoi(qdata.substr(qpos, 3)); }
								catch (...) { have = 0; }
							}
							qpos += 3;
							std::string mname = (std::string)nl::nx::string["Mob.img"][std::to_string(mobid)]["name"];
							if (mname.empty())
								mname = "Monster " + std::to_string(mobid);
							std::string line = mname + " " + std::to_string(have) + "/" + std::to_string(need);
							Color::Name col = (have >= need) ? Color::Name::EMPEROR : Color::Name::BLACK;
							Text req(Text::Font::A11M, Text::Alignment::LEFT, col, line, 320, false);
							req.draw(Point<int16_t>(text_origin.x(), req_y));
							req_y += 14;
						}

						for (auto i : ec["item"])
						{
							int32_t itemid = static_cast<int32_t>((int64_t)i["id"]);
							int32_t need = static_cast<int32_t>((int64_t)i["count"]);
							if (itemid <= 0) continue;
							int32_t have = inv.get_total_item_count(itemid);
							const ItemData& idata = ItemData::get(itemid);
							std::string iname = idata.is_valid() ? idata.get_name() : ("Item " + std::to_string(itemid));
							std::string line = iname + " " + std::to_string(have) + "/" + std::to_string(need);
							Color::Name col = (have >= need) ? Color::Name::EMPEROR : Color::Name::BLACK;
							Text req(Text::Font::A11M, Text::Alignment::LEFT, col, line, 320, false);
							req.draw(Point<int16_t>(text_origin.x(), req_y));
							req_y += 14;
						}

						cur_y = req_y;
					}

					// 5 px spacing before the next banner.
					cur_y += 5;
				}
			}
		}

		// Draw text input field for SENDGETTEXT / SENDGETNUMBER
		if (type == TalkType::SENDGETTEXT || type == TalkType::SENDGETNUMBER)
			input_field.draw(position);

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
			case TalkType::SENDOK:
			{
				// msgtype 0 — text + [OK]. Close and OK both report end.
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::OK:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}
				break;
			}
			case TalkType::SENDNEXT:
			{
				// msgtype 5 — text + [Next].
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
			case TalkType::SENDNEXTPREV:
			{
				// msgtype 6 — text + [Back] [Next].
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
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
				// msgtype 1 — text + [Yes] [No].
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
				// msgtype 7 — quest accept/decline (text + [Accept] [Decline]).
				// We render this with BtYes / BtNo, but keep the quest-
				// themed Q* button ids in the routing too so any NX
				// build that still uses the themed sprites keeps working.
				switch (buttonid)
				{
					case Buttons::CLOSE:
					case Buttons::NO:
					case Buttons::QNO:
					case Buttons::QAFTER:
					case Buttons::QCNO:
					case Buttons::QGIVEUP:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::YES:
					case Buttons::QYES:
					case Buttons::QSTART:
					case Buttons::QCYES:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}
				break;
			}
			case TalkType::SENDGETTEXT:
			{
				// msgtype 13 — text input.
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
				// msgtype 12 — number input.
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
				// msgtype 4 — close only; selections handled in send_cursor.
				if (buttonid == Buttons::CLOSE)
					NpcTalkMorePacket(type, 0).dispatch();
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

		// Quest-progress banner hit-test (one clickable list_normal
		// sprite per in-progress quest with this NPC). Hover swaps to
		// list_hovered; click toggles that quest's expanded state in
		// expanded_quest_ids so its requirements render below the
		// banner.
		quest_banner_hovered_qid = -1;
		for (const auto& hit : quest_banner_hits)
		{
			if (hit.rect.contains(cursorpos))
			{
				quest_banner_hovered_qid = hit.qid;
				if (clicked)
				{
					if (expanded_quest_ids.count(hit.qid))
						expanded_quest_ids.erase(hit.qid);
					else
						expanded_quest_ids.insert(hit.qid);
					return Cursor::State::CLICKING;
				}
				return Cursor::State::CANCLICK;
			}
		}

		// Handle SENDSIMPLE option clicks.
		// Uses embedded #L<i># codes in the text; reply via NpcTalkMorePacket(index).
		if (type == TalkType::SENDSIMPLE && !selections.empty())
		{
			// Mirror the DRAW path's geometry exactly — the previous
			// hit-test used text_x = +130 while the visible text
			// renders at +166, leaving a dead 36-px strip and pushing
			// every option's hot zone left of where the player sees
			// it. Y-band is widened by a few pixels above and below
			// to absorb word-wrap drift between prefix_text.height()
			// and the formatted_text's actual layout.
			int16_t y_adj = height - min_height;
			int16_t text_x = position.x() + 166;
			int16_t text_y = position.y() + 48 - y_adj;
			int16_t text_w = 320;
			int16_t line_h = 16; // approximate line height for A12M font
			int16_t band_pad = 4; // extra px above/below each option row

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
				int16_t row_top  = baseline - line_h - band_pad;
				int16_t row_bot  = baseline + band_pad;

				if (cursorpos.x() >= text_x && cursorpos.x() <= text_x + text_w &&
					cursorpos.y() >= row_top && cursorpos.y() <= row_bot)
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
		case TalkType::SENDOK:
		case TalkType::SENDYESNO:
		case TalkType::SENDSIMPLE:
		case TalkType::SENDNEXT:
		case TalkType::SENDNEXTPREV:
		case TalkType::SENDACCEPTDECLINE:
		case TalkType::SENDGETNUMBER:
		case TalkType::SENDGETTEXT:
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

		// Item IDs we've passed through via `#v<id>#`, `#i<id>#`,
		// `#t<id>#` or `#z<id>#` macros. After the main pass we run a
		// cleanup that strips those exact id strings if they appear
		// later in the text — many quest scripts hard-code the id as
		// literal digits next to the name (e.g. "#v4031161# #t4031161#
		// 4031161 / 1"), which surfaces as redundant numbers in the
		// dialog (Rusty Screw 4031161 / 1).
		std::vector<std::string> macro_ids;

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

			// #t<id># or #z<id># — item name.
			if (code == 't' || code == 'z')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty())
					{
						macro_ids.push_back(id_str);
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

			// Inline-icon macros (item / quest / skill / face). Pass
			// the whole `#X<id>#` through unchanged so the Text /
			// LayoutBuilder pipeline reserves a 32-px slot at the
			// macro position. UINpcTalk::draw_inline_icons paints
			// the bitmap from the right NX source per kind.
			if (code == 'v' || code == 'i' || code == 'q'
				|| code == 's' || code == 'f')
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos)
				{
					std::string id_str = tx.substr(i + 2, end - i - 2);
					if (!id_str.empty() && (code == 'v' || code == 'i'))
						macro_ids.push_back(id_str);
					result.append(tx, i, end - i + 1);
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

			// Catch-all for any other `#X<id>#` macro the script might
			// emit (e.g. `#q<id>#` quest icon, `#s<id>#` skill icon,
			// `#f<id>#` face icon). We don't render their bitmaps yet,
			// but stripping the macro is far better than leaking
			// "ui/UIWindow.img/QuestIcon/8" path-style text into the
			// dialog. Recognised by the shape: `#X<digits>#`.
			if (std::isalpha(static_cast<unsigned char>(code)))
			{
				size_t end = tx.find('#', i + 2);
				if (end != std::string::npos && end > i + 2)
				{
					bool all_digits = true;
					for (size_t k = i + 2; k < end; k++)
					{
						if (!std::isdigit(static_cast<unsigned char>(tx[k])))
						{
							all_digits = false;
							break;
						}
					}
					if (all_digits)
					{
						i = end;
						continue;
					}
				}
			}

			// Unknown code — pass through as-is.
			result += '#';
		}

		// Strip standalone occurrences of any item id we already
		// expanded via a macro. Avoids the "Rusty Screw 4031161 / 1"
		// look — when the script literally writes the id next to its
		// own name macro. We skip when the id is bounded by digits
		// (so legitimate larger numbers stay) AND when it's directly
		// inside a `#X<digits>#` inline-icon macro (e.g. `#v4031161#`),
		// because eating those breaks the inline-icon layout.
		for (const std::string& id : macro_ids)
		{
			if (id.size() < 4) continue; // skip short ids — too easily a real number
			size_t pos = 0;
			while ((pos = result.find(id, pos)) != std::string::npos)
			{
				bool left_ok = (pos == 0)
					|| !std::isdigit(static_cast<unsigned char>(result[pos - 1]));
				bool right_ok = (pos + id.size() >= result.size())
					|| !std::isdigit(static_cast<unsigned char>(result[pos + id.size()]));

				// Inside-macro guard: id sits immediately after `#X`
				// (any single letter), so `result[pos-2]` would be `#`.
				bool inside_macro = false;
				if (pos >= 2
					&& result[pos - 2] == '#'
					&& std::isalpha(static_cast<unsigned char>(result[pos - 1])))
				{
					inside_macro = true;
				}

				if (left_ok && right_ok && !inside_macro)
				{
					size_t start = pos;
					size_t end = pos + id.size();
					if (start > 0 && result[start - 1] == ' ')
						start--;
					else if (end < result.size() && result[end] == ' ')
						end++;
					result.erase(start, end - start);
					pos = start;
				}
				else
				{
					pos += id.size();
				}
			}
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

		current_npcid = npcid;
		expanded_quest_ids.clear();
		quest_banner_hovered_qid = -1;
		quest_banner_hits.clear();
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

		if (type != TalkType::SENDSIMPLE)
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

		// Pad the text area so its bottom edge clears the fill region's
		// own bottom border. Without this, multi-line dialogues whose
		// total height equals `height` exactly still bleed a few pixels
		// below the panel because text_y (= position.y + 48 - y_adj)
		// drifts as y_adj grows and the fill texture has internal padding
		// that isn't part of `text_height`.
		constexpr int16_t TEXT_BOTTOM_PAD = 16;
		int16_t needed = text_height + TEXT_BOTTOM_PAD;

		if (needed > height)
		{
			if (needed > MAX_HEIGHT)
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
				height = needed;
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
			case TalkType::SENDOK:
			{
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDNEXT:
			{
				buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::NEXT]->set_active(true);
				break;
			}
			case TalkType::SENDNEXTPREV:
			{
				buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::NEXT]->set_active(true);
				buttons[Buttons::PREV]->set_position(Point<int16_t>(389, y_cord));
				buttons[Buttons::PREV]->set_active(true);
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
				// Use BtYes / BtNo — the universal Yes/No sprites
				// shipped in every UtilDlgEx node. The quest-themed
				// BtQYes / BtQNo / BtQStart / BtQAfter variants
				// don't always have valid normal/mouseover/pressed
				// state subnodes, so they rendered as invisible 0×0
				// hit zones and the player saw no buttons.
				buttons[Buttons::YES]->set_position(Point<int16_t>(389, y_cord));
				buttons[Buttons::YES]->set_active(true);

				buttons[Buttons::NO]->set_position(Point<int16_t>(389 + 65, y_cord));
				buttons[Buttons::NO]->set_active(true);
				break;
			}
			case TalkType::SENDGETTEXT:
			{
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
				// Close-only; options are clicked in the text area.
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

}