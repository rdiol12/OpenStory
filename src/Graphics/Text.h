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

#include "DrawArgument.h"

#include <map>
#include <vector>

namespace ms
{
	class Text
	{
	public:
		enum Font
		{
			A11M,
			A11B,
			A12M,
			A12B,
			A13M,
			A13B,
			A14B,
			A15B,
			A18M,
			NUM_FONTS
		};

		enum Alignment
		{
			LEFT,
			CENTER,
			RIGHT
		};

		enum Background
		{
			NONE,
			NAMETAG
		};

		class Layout
		{
		public:
			struct Word
			{
				size_t first;
				size_t last;
				Font font;
				Color::Name color;
			};

			struct Line
			{
				std::vector<Word> words;
				Point<int16_t> position;
			};

			// Inline image run — a sprite that replaces a `#X<id>#`
			// macro in formatted text. The layout reserves
			// `size` x `size` pixels at `pos` (top-left, in layout
			// coordinates relative to the Text's draw position) and
			// stores `item_id` plus a `kind` so the caller can pick
			// the correct NX source (item / quest / skill / face).
			enum class ImageKind : int8_t
			{
				ITEM = 0,    // #v<id># or #i<id># — Item.wz
				QUEST = 1,   // #q<id># — UI.wz/UIWindow.img/QuestIcon
				SKILL = 2,   // #s<id># — Skill.wz/<job>.img/skill/<id>/icon
				FACE = 3,    // #f<id># — Character.wz/Face/<id>.img
				EMOTE = 4    // #e<id># — UI.wz/Emote.img/<id> (emote/expression icons)
			};
			struct Image
			{
				Point<int16_t> pos;
				int32_t item_id;
				int16_t size;
				ImageKind kind = ImageKind::ITEM;
			};

			Layout(const std::vector<Line>& lines, const std::vector<int16_t>& advances, const std::vector<Image>& images, int16_t width, int16_t height, int16_t endx, int16_t endy);
			Layout();

			int16_t width() const;
			int16_t height() const;
			int16_t advance(size_t index) const;
			Point<int16_t> get_dimensions() const;
			Point<int16_t> get_endoffset() const;
			const std::vector<Image>& get_images() const;

			using iterator = std::vector<Line>::const_iterator;
			iterator begin() const;
			iterator end() const;

		private:
			std::vector<Line> lines;
			std::vector<int16_t> advances;
			std::vector<Image> images;
			Point<int16_t> dimensions;
			Point<int16_t> endoffset;
		};

		Text(Font font, Alignment alignment, Color::Name color, Background background, const std::string& text = "", uint16_t maxwidth = 0, bool formatted = true, int16_t line_adj = 0);
		Text(Font font, Alignment alignment, Color::Name color, const std::string& text = "", uint16_t maxwidth = 0, bool formatted = true, int16_t line_adj = 0);
		Text();

		void draw(const DrawArgument& args) const;
		void draw(const DrawArgument& args, const Range<int16_t>& vertical) const;

		void change_text(const std::string& text);
		void change_color(Color::Name color);
		void set_background(Background background);

		bool empty() const;
		size_t length() const;
		int16_t width() const;
		int16_t height() const;
		uint16_t advance(size_t pos) const;
		Point<int16_t> dimensions() const;
		Point<int16_t> endoffset() const;
		const std::string& get_text() const;

		// Inline images parsed from #v<id># / #i<id># macros, with
		// pixel positions resolved against the laid-out text.
		// Empty when the text contains no item-icon macros.
		const std::vector<Layout::Image>& images() const;

		// Decodes one UTF-8 codepoint at text[i]. Sets consumed to the byte
		// count (1-4), or 0 at the end. Falls back to the raw byte on
		// malformed input so a bad byte can never stall a caller's loop.
		static uint32_t utf8_decode(const char* text, size_t length, size_t i, size_t& consumed);
		// Appends one codepoint to out as UTF-8.
		static void utf8_encode(uint32_t codepoint, std::string& out);
		// True for codepoints in the Hebrew blocks.
		static bool is_rtl(uint32_t codepoint);
		// True if the string contains any right-to-left character. Cheap
		// guard so the pure-Latin case skips reordering entirely.
		static bool has_rtl(const std::string& text);

		// v83 ships 25 face expressions in Character.wz/Face/000<face>.img.
		// Chat emoticons address them by index through the #e<n># macro.
		// (That macro previously pointed at UI.wz/Emote.img, which does not
		// exist in v83, so it could never render anything.)
		static const char* emoticon_name(int32_t index);
		static int32_t emoticon_count();

		// Reorders logical-order text into visual (display) order per the
		// Unicode bidi algorithm, restricted to levels 0-2 (which is all
		// Hebrew needs -- no Arabic shaping).
		//
		// Everything downstream of layout draws strictly left-to-right and
		// accumulates advances forward, so reordering here is what makes
		// Hebrew display correctly without touching layout or drawing.
		// Returns text unchanged when it contains no RTL characters.
		static std::string visual_order(const std::string& text);

		// Maps a byte offset in the logical string to the matching offset in the
		// visual string. The caret is tracked logically (typing order) but drawn
		// against the visual layout, so without this it lands nowhere near the
		// character it belongs to in Hebrew.
		//
		// The caret sits on the trailing edge of the character before it, which
		// for a right-to-left character is its LEFT side -- that is where the
		// next keystroke will appear.
		static size_t logical_to_visual(const std::string& text, size_t pos);

	private:
		void reset_layout();

		Font font;
		Alignment alignment;
		Color::Name color;
		Background background;
		Layout layout;
		uint16_t maxwidth;
		bool formatted;
		std::string text;      // logical order, as supplied by the caller
		std::string display;   // visual order -- what layout and draw consume
		int16_t line_adj;
	};
}