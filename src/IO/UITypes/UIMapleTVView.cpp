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
#include "UIMapleTVView.h"

#include "../../Constants.h"
#include "../../Gameplay/MapleTVBroadcast.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMapleTVView::UIMapleTVView()
	{
		tv_frame = nl::nx::ui["MapleTV.img"]["TVbasic"]["0"];
		tv_off = Animation(nl::nx::ui["MapleTV.img"]["TVoff"]);

		line_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, "", 160);
		name_text = Text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::MINESHAFT);

		int16_t vwidth = Constants::Constants::get().get_viewwidth();
		position = Point<int16_t>(vwidth - 252, 28);
		dimension = tv_frame.get_dimensions();
	}

	void UIMapleTVView::draw(float inter) const
	{
		auto& b = MapleTVBroadcast::get();

		// Off the air: the TV plays its static/off-air loop
		if (!b.active())
		{
			tv_off.draw(DrawArgument(position), inter);
			return;
		}

		tv_frame.draw(DrawArgument(position));

		if (has_look)
			sender_look.draw(DrawArgument(position + Point<int16_t>(34, 78)), inter);

		const auto& lines = b.lines();

		if (!lines.empty())
		{
			line_text.change_text(lines[line_index % lines.size()]);
			line_text.draw(position + Point<int16_t>(66, 24));
		}
	}

	void UIMapleTVView::update()
	{
		auto& b = MapleTVBroadcast::get();

		if (!b.active())
		{
			tv_off.update();

			int16_t vw = Constants::Constants::get().get_viewwidth();
			position = Point<int16_t>(vw - 252, 28);
			return;
		}

		b.update();

		// Fresh broadcast: rebuild the sender's look
		if (b.serial() != seen_serial)
		{
			seen_serial = b.serial();
			line_index = 0;
			line_tick = 0;
			has_look = b.has_look();

			if (has_look)
			{
				sender_look = CharLook(b.get_look());
				sender_look.set_stance(Stance::Id::STAND1);
			}
		}

		if (has_look)
			sender_look.update(Constants::TIMESTEP);

		// Cycle the message lines every ~4 seconds
		line_tick += Constants::TIMESTEP;

		if (line_tick >= 4000)
		{
			line_tick = 0;
			line_index++;
		}

		// Track window resizes
		int16_t vwidth = Constants::Constants::get().get_viewwidth();
		position = Point<int16_t>(vwidth - 252, 28);
	}

	bool UIMapleTVView::is_in_range(Point<int16_t>) const
	{
		return false;
	}

	UIElement::Type UIMapleTVView::get_type() const
	{
		return TYPE;
	}
}
