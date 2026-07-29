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
#include "Text.h"

#include "GraphicsGL.h"

#include <algorithm>

namespace ms
{
	namespace
	{
		// Bidi character classes. Only what levels 0-2 need: Arabic would
		// additionally require joining/shaping, which is deliberately out of scope.
		enum class Bidi
		{
			L,   // strong left-to-right
			R,   // strong right-to-left (Hebrew)
			EN,  // European number -- digits read left-to-right even inside Hebrew
			N    // neutral: spaces, punctuation, symbols
		};

		Bidi classify(uint32_t cp)
		{
			if (cp >= 0x0590 && cp <= 0x05FF) return Bidi::R;   // Hebrew
			if (cp >= 0xFB1D && cp <= 0xFB4F) return Bidi::R;   // Hebrew presentation forms
			if (cp >= '0' && cp <= '9') return Bidi::EN;
			if (cp >= 0x0660 && cp <= 0x0669) return Bidi::EN;  // Arabic-Indic digits
			if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return Bidi::L;
			if (cp >= 0x00C0 && cp <= 0x024F) return Bidi::L;   // Latin supplement/extended
			if (cp >= 0x0370 && cp <= 0x04FF) return Bidi::L;   // Greek, Cyrillic
			if (cp >= 0x1100 && cp <= 0xD7FF) return Bidi::L;   // Hangul, CJK
			if (cp >= 0x3000 && cp <= 0x9FFF) return Bidi::L;
			return Bidi::N;
		}

		// Paired punctuation reads mirrored inside a right-to-left run:
		// an opening paren must be drawn as a closing one.
		uint32_t mirror(uint32_t cp)
		{
			switch (cp)
			{
			case '(': return ')';
			case ')': return '(';
			case '[': return ']';
			case ']': return '[';
			case '{': return '}';
			case '}': return '{';
			case '<': return '>';
			case '>': return '<';
			default:  return cp;
			}
		}
	}

	namespace
	{
		// Order is the picker's display order, so it is part of the wire format:
		// a sent message carries #e<index>#, and changing the order would change
		// what an already-sent message means. Append only.
		const char* EMOTICONS[] = {
			"smile", "wink", "cheers", "love", "glitter", "chu",
			"blink", "hum", "shine", "despair", "cry", "troubled",
			"oops", "bewildered", "stunned", "pain", "hit", "dam",
			"angry", "blaze", "hot", "vomit", "bowing", "info",
			"default"
		};
	}

	const char* Text::emoticon_name(int32_t index)
	{
		if (index < 0 || index >= emoticon_count())
			return "default";

		return EMOTICONS[index];
	}

	int32_t Text::emoticon_count()
	{
		return static_cast<int32_t>(sizeof(EMOTICONS) / sizeof(EMOTICONS[0]));
	}

	uint32_t Text::utf8_decode(const char* text, size_t length, size_t i, size_t& consumed)
	{
		if (text == nullptr || i >= length)
		{
			consumed = 0;
			return 0;
		}

		uint8_t b0 = static_cast<uint8_t>(text[i]);

		if (b0 < 0x80)
		{
			consumed = 1;
			return b0;
		}

		auto cont = [&](size_t k) -> bool
		{
			return i + k < length && (static_cast<uint8_t>(text[i + k]) & 0xC0) == 0x80;
		};
		auto bits = [&](size_t k) -> uint32_t
		{
			return static_cast<uint8_t>(text[i + k]) & 0x3F;
		};

		if ((b0 & 0xE0) == 0xC0 && cont(1))
		{
			consumed = 2;
			return ((b0 & 0x1Fu) << 6) | bits(1);
		}

		if ((b0 & 0xF0) == 0xE0 && cont(1) && cont(2))
		{
			consumed = 3;
			return ((b0 & 0x0Fu) << 12) | (bits(1) << 6) | bits(2);
		}

		if ((b0 & 0xF8) == 0xF0 && cont(1) && cont(2) && cont(3))
		{
			consumed = 4;
			return ((b0 & 0x07u) << 18) | (bits(1) << 12) | (bits(2) << 6) | bits(3);
		}

		// Malformed: consume the byte so callers cannot loop forever.
		consumed = 1;
		return b0;
	}

	void Text::utf8_encode(uint32_t cp, std::string& out)
	{
		if (cp < 0x80)
		{
			out.push_back(static_cast<char>(cp));
		}
		else if (cp < 0x800)
		{
			out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else if (cp < 0x10000)
		{
			out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else
		{
			out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
	}

	bool Text::is_rtl(uint32_t cp)
	{
		return classify(cp) == Bidi::R;
	}

	bool Text::has_rtl(const std::string& text)
	{
		size_t i = 0;

		while (i < text.size())
		{
			size_t consumed = 1;
			uint32_t cp = utf8_decode(text.c_str(), text.size(), i, consumed);

			if (consumed == 0)
				break;

			if (is_rtl(cp))
				return true;

			i += consumed;
		}

		return false;
	}

	namespace
	{
		// One bidi pass, shared by visual_order and logical_to_visual so the
		// caret can never disagree with the glyphs it is drawn among.
		struct BidiRun
		{
			std::vector<uint32_t> cps;
			std::vector<size_t> starts;   // logical byte offset of each codepoint
			std::vector<size_t> lens;     // its byte length
			std::vector<uint8_t> level;
			std::vector<size_t> order;    // visual position -> codepoint index
		};
	}

	namespace
	{
		void analyze(const std::string& text, BidiRun& run);
	}

	std::string Text::visual_order(const std::string& text)
	{
		if (!has_rtl(text))
			return text;

		BidiRun run;
		analyze(text, run);

		std::string out;
		out.reserve(text.size());

		for (size_t idx : run.order)
		{
			uint32_t cp = run.cps[idx];

			if (run.level[idx] & 1)
				cp = mirror(cp);

			utf8_encode(cp, out);
		}

		return out;
	}

	size_t Text::logical_to_visual(const std::string& text, size_t pos)
	{
		if (pos == 0 || !has_rtl(text))
			return pos;

		BidiRun run;
		analyze(text, run);

		// The character immediately before the caret -- the one just typed.
		size_t owner = run.cps.size();

		for (size_t i = 0; i < run.cps.size(); ++i)
		{
			if (run.starts[i] + run.lens[i] <= pos)
				owner = i;
		}

		if (owner >= run.cps.size())
			return 0;

		size_t visual_bytes = 0;

		for (size_t v = 0; v < run.order.size(); ++v)
		{
			size_t idx = run.order[v];

			if (idx == owner)
			{
				// Right-to-left: the caret belongs on the glyph's left edge,
				// which is where the next character will be drawn.
				return (run.level[idx] & 1) ? visual_bytes
				                            : visual_bytes + run.lens[idx];
			}

			visual_bytes += run.lens[idx];
		}

		return visual_bytes;
	}

	namespace
	{
	void analyze(const std::string& text, BidiRun& run)
	{
		std::vector<uint32_t>& cps = run.cps;
		std::vector<Bidi> types;
		cps.reserve(text.size());
		types.reserve(text.size());

		for (size_t i = 0; i < text.size(); )
		{
			size_t consumed = 1;
			uint32_t cp = Text::utf8_decode(text.c_str(), text.size(), i, consumed);

			if (consumed == 0)
				break;

			cps.push_back(cp);
			types.push_back(classify(cp));
			run.starts.push_back(i);
			run.lens.push_back(consumed);
			i += consumed;
		}

		// Paragraph direction comes from the first strong character (UBA rule P2/P3).
		bool para_rtl = false;

		for (Bidi t : types)
		{
			if (t == Bidi::L) { para_rtl = false; break; }
			if (t == Bidi::R) { para_rtl = true;  break; }
		}

		const size_t n = cps.size();
		std::vector<uint8_t>& level = run.level;
		level.assign(n, para_rtl ? 1 : 0);

		// Strong characters take their own direction; digits sit one level above
		// the surrounding run so they read left-to-right inside Hebrew text.
		for (size_t i = 0; i < n; ++i)
		{
			if (types[i] == Bidi::L)
				level[i] = 0;
			else if (types[i] == Bidi::R)
				level[i] = 1;
		}

		// Neutrals between two same-direction strongs take that direction,
		// otherwise the paragraph direction (UBA rule N1/N2).
		for (size_t i = 0; i < n; ++i)
		{
			if (types[i] != Bidi::N && types[i] != Bidi::EN)
				continue;

			size_t j = i;
			while (j < n && (types[j] == Bidi::N || types[j] == Bidi::EN))
				++j;

			bool before_rtl = para_rtl;
			bool after_rtl = para_rtl;

			for (size_t k = i; k-- > 0; )
			{
				if (types[k] == Bidi::L || types[k] == Bidi::R)
				{
					before_rtl = (types[k] == Bidi::R);
					break;
				}
			}

			for (size_t k = j; k < n; ++k)
			{
				if (types[k] == Bidi::L || types[k] == Bidi::R)
				{
					after_rtl = (types[k] == Bidi::R);
					break;
				}
			}

			bool run_rtl = (before_rtl == after_rtl) ? before_rtl : para_rtl;

			for (size_t k = i; k < j; ++k)
			{
				// A digit inside a right-to-left run keeps its own left-to-right
				// order, which level 2 expresses: the reversal pass below undoes
				// the run reversal for exactly these.
				if (run_rtl)
					level[k] = (types[k] == Bidi::EN) ? 2 : 1;
				else
					level[k] = 0;
			}

			i = j - 1;
		}

		std::vector<size_t>& order = run.order;
		order.resize(n);

		for (size_t i = 0; i < n; ++i)
			order[i] = i;

		// UBA rule L2: from the highest level down to the lowest odd level,
		// reverse every contiguous run at or above that level.
		uint8_t highest = 0;
		uint8_t lowest_odd = 255;

		for (uint8_t lv : level)
		{
			highest = std::max(highest, lv);

			if ((lv & 1) && lv < lowest_odd)
				lowest_odd = lv;
		}

		if (lowest_odd != 255)
		{
			for (uint8_t lv = highest; lv >= lowest_odd; --lv)
			{
				size_t i = 0;

				while (i < n)
				{
					if (level[order[i]] < lv)
					{
						++i;
						continue;
					}

					size_t j = i;
					while (j < n && level[order[j]] >= lv)
						++j;

					std::reverse(order.begin() + i, order.begin() + j);
					i = j;
				}

				if (lv == 0)
					break;
			}
		}
	}
	}

	Text::Text(Font f, Alignment a, Color::Name c, Background b, const std::string& t, uint16_t mw, bool fm, int16_t la) : font(f), alignment(a), color(c), background(b), maxwidth(mw), formatted(fm), line_adj(la)
	{
		change_text(t);
	}

	Text::Text(Font f, Alignment a, Color::Name c, const std::string& t, uint16_t mw, bool fm, int16_t la) : Text(f, a, c, Background::NONE, t, mw, fm, la) {}
	Text::Text() : Text(Font::A11M, Alignment::LEFT, Color::BLACK) {}

	void Text::reset_layout()
	{
		if (text.empty())
		{
			display.clear();
			return;
		}

		// Layout and drawing both run strictly left-to-right, so Hebrew has to be
		// reordered into display order before either sees it. `text` stays logical
		// so get_text() and callers editing the string are unaffected.
		bool rtl = has_rtl(text);

		display = rtl ? visual_order(text) : text;

		layout = GraphicsGL::get().createlayout(display, font, alignment, maxwidth, formatted, line_adj, rtl);
	}

	void Text::change_text(const std::string& t)
	{
		if (text == t)
			return;

		text = t;

		reset_layout();
	}

	void Text::change_color(Color::Name c)
	{
		if (color == c)
			return;

		color = c;

		reset_layout();
	}

	void Text::set_background(Background b)
	{
		background = b;
	}

	void Text::draw(const DrawArgument& args) const
	{
		draw(args, Range<int16_t>(0, 0));
	}

	void Text::draw(const DrawArgument& args, const Range<int16_t>& vertical) const
	{
		GraphicsGL::get().drawtext(args, vertical, display, layout, font, color, background);
	}

	uint16_t Text::advance(size_t pos) const
	{
		return layout.advance(pos);
	}

	bool Text::empty() const
	{
		return text.empty();
	}

	size_t Text::length() const
	{
		return text.size();
	}

	int16_t Text::width() const
	{
		return layout.width();
	}

	int16_t Text::height() const
	{
		return layout.height();
	}

	Point<int16_t> Text::dimensions() const
	{
		return layout.get_dimensions();
	}

	Point<int16_t> Text::endoffset() const
	{
		return layout.get_endoffset();
	}

	const std::string& Text::get_text() const
	{
		return text;
	}

	const std::vector<Text::Layout::Image>& Text::images() const
	{
		return layout.get_images();
	}

	const std::vector<Text::Layout::Image>& Text::Layout::get_images() const
	{
		return images;
	}

	Text::Layout::Layout(const std::vector<Layout::Line>& l, const std::vector<int16_t>& a, const std::vector<Layout::Image>& im, int16_t w, int16_t h, int16_t ex, int16_t ey) : lines(l), advances(a), images(im), dimensions(w, h), endoffset(ex, ey) {}
	Text::Layout::Layout() : Layout(std::vector<Layout::Line>(), std::vector<int16_t>(), std::vector<Layout::Image>(), 0, 0, 0, 0) {}

	int16_t Text::Layout::width() const
	{
		return dimensions.x();
	}

	int16_t Text::Layout::height() const
	{
		return dimensions.y();
	}

	int16_t Text::Layout::advance(size_t index) const
	{
		return index < advances.size() ? advances[index] : 0;
	}

	Point<int16_t> Text::Layout::get_dimensions() const
	{
		return dimensions;
	}

	Point<int16_t> Text::Layout::get_endoffset() const
	{
		return endoffset;
	}

	Text::Layout::iterator Text::Layout::begin() const
	{
		return lines.begin();
	}

	Text::Layout::iterator Text::Layout::end() const
	{
		return lines.end();
	}
}