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
#include "ChatBalloon.h"

#include "../../Constants.h"
#include "../../Data/ItemData.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	ChatBalloon::ChatBalloon(int8_t type)
	{
		std::string typestr;

		if (type < 0)
		{
			switch (type)
			{
				case -1:
					typestr = "dead";
					break;
			}
		}
		else
		{
			typestr = std::to_string(type);
		}

		nl::node src = nl::nx::ui["ChatBalloon.img"][typestr];

		arrow = src["arrow"];
		frame = src;

		textlabel = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK, "", 80);

		duration = 0;
	}

	ChatBalloon::ChatBalloon() : ChatBalloon(0) {}

	void ChatBalloon::change_text(const std::string& text)
	{
		textlabel.change_text(text);

		duration = DURATION;
	}

	void ChatBalloon::draw(Point<int16_t> position) const
	{
		if (duration == 0)
			return;

		int16_t width = textlabel.width();
		int16_t height = textlabel.height();

		frame.draw(position, width, height);
		arrow.draw(position);

		Point<int16_t> text_origin = position - Point<int16_t>(0, height + 4);
		textlabel.draw(text_origin);

		// Overlay inline icons reserved by #v/#i (item), #q (quest), #s (skill),
		// #f (face) macros in the speak text. The Text layout already reserved a
		// size x size slot at img.pos (relative to text_origin); resolve the sprite
		// from the right NX source and stamp it centered in that slot. Mirrors the
		// NPC dialog window's inline-icon path.
		for (const auto& img : textlabel.images())
		{
			if (img.item_id <= 0)
				continue;

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
				int32_t job = img.item_id / 10000;
				std::string js = std::to_string(job);
				while (js.size() < 3) js.insert(0, 1, '0');
				nl::node snode = nl::nx::skill[js + ".img"]["skill"][std::to_string(img.item_id)]["icon"];
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
			case Text::Layout::ImageKind::EMOTE:
			{
				// Emote/expression icons authored in UI.wz/Emote.img/<id>.
				nl::node enode = nl::nx::ui["Emote.img"][std::to_string(img.item_id)];
				if (enode)
					tex = Texture(enode);
				break;
			}
			}

			if (!tex.is_valid())
				continue;

			Point<int16_t> icon_dims = tex.get_dimensions();
			Point<int16_t> slot_pos = text_origin + img.pos;
			Point<int16_t> centered(
				slot_pos.x() + (img.size - icon_dims.x()) / 2,
				slot_pos.y() + (img.size - icon_dims.y()) / 2);
			tex.draw(DrawArgument(centered));
		}
	}

	void ChatBalloon::update()
	{
		duration -= Constants::TIMESTEP;

		if (duration < 0)
			duration = 0;
	}

	void ChatBalloon::expire()
	{
		duration = 0;
	}

	ChatBalloonHorizontal::ChatBalloonHorizontal()
	{
		nl::node Balloon = nl::nx::ui["Login.img"]["WorldNotice"]["Balloon"];

		arrow = Balloon["arrow"];
		center = Balloon["c"];
		east = Balloon["e"];
		northeast = Balloon["ne"];
		north = Balloon["n"];
		northwest = Balloon["nw"];
		west = Balloon["w"];
		southwest = Balloon["sw"];
		south = Balloon["s"];
		southeast = Balloon["se"];

		xtile = std::max<int16_t>(north.width(), 1);
		ytile = std::max<int16_t>(west.height(), 1);

		textlabel = Text(Text::Font::A13B, Text::Alignment::LEFT, Color::Name::BLACK);
	}

	void ChatBalloonHorizontal::draw(Point<int16_t> position) const
	{
		int16_t width = textlabel.width() + 9;
		int16_t height = textlabel.height() - 2;

		int16_t left = position.x() - width / 2;
		int16_t top = position.y() - height;
		int16_t right = left + width;
		int16_t bottom = top + height;

		northwest.draw(DrawArgument(left, top));
		southwest.draw(DrawArgument(left, bottom));

		for (int16_t y = top; y < bottom; y += ytile)
		{
			west.draw(DrawArgument(left, y));
			east.draw(DrawArgument(right, y));
		}

		center.draw(DrawArgument(Point<int16_t>(left - 8, top), Point<int16_t>(width + 8, height)));

		for (int16_t x = left; x < right; x += xtile)
		{
			north.draw(DrawArgument(x, top));
			south.draw(DrawArgument(x, bottom));
		}

		northeast.draw(DrawArgument(right, top));
		southeast.draw(DrawArgument(right, bottom));

		arrow.draw(DrawArgument(right + 1, top));
		textlabel.draw(DrawArgument(left + 6, top - 5));
	}

	void ChatBalloonHorizontal::change_text(const std::string& text)
	{
		textlabel.change_text(text);
	}
}