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
#include "UIBuffList.h"

#include "../../Data/ItemData.h"
#include "../../Data/SkillData.h"
#include "../../Util/Misc.h"
#include "../UI.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	BuffIcon::BuffIcon(int32_t buff, int32_t dur) : cover(IconCover::BUFF, dur - FLASH_TIME)
	{
		buffid = buff;
		duration = dur;
		opacity.set(1.0f);
		opcstep = -0.05f;

		if (buffid >= 0)
		{
			std::string strid;

			if (buffid < 10000000)
				strid = string_format::extend_id(buffid, 7);
			else
				strid = std::to_string(buffid);

			std::string jobid = std::to_string(buffid / 10000);
			nl::node src = nl::nx::skill[jobid + ".img"]["skill"][strid];
			icon = src["icon"];
		}
		else
		{
			icon = ItemData::get(-buffid).get_icon(true);
		}
	}

	void BuffIcon::draw(Point<int16_t> position, float alpha) const
	{
		icon.draw(DrawArgument(position, opacity.get(alpha)));
		cover.draw(position + Point<int16_t>(1, -31), alpha);
	}

	bool BuffIcon::update()
	{
		if (duration <= FLASH_TIME)
		{
			opacity += opcstep;

			bool fadedout = opcstep < 0.0f && opacity.last() <= 0.0f;
			bool fadedin = opcstep > 0.0f && opacity.last() >= 1.0f;

			if (fadedout || fadedin)
				opcstep = -opcstep;
		}

		cover.update();

		duration -= Constants::TIMESTEP;

		return duration < Constants::TIMESTEP;
	}

	int32_t BuffIcon::get_buffid() const
	{
		return buffid;
	}

	bool BuffIcon::contains(Point<int16_t> icon_pos, Point<int16_t> cursorpos) const
	{
		auto absx = cursorpos.x() - icon_pos.x();
		auto absy = cursorpos.y() - icon_pos.y();
		return absx >= -1 && absx <= 31 && absy >= -31 && absy <= 1;
	}

	UIBuffList::UIBuffList()
	{
		int16_t height = Constants::Constants::get().get_viewheight();
		int16_t width = Constants::Constants::get().get_viewwidth();

		update_screen(width, height);
	}

	void UIBuffList::draw(float alpha) const
	{
		Point<int16_t> icpos = position;

		for (auto& icon : icons)
		{
			icon.second.draw(icpos, alpha);
			icpos.shift_x(-32);
		}
	}

	void UIBuffList::update()
	{
		for (auto iter = icons.begin(); iter != icons.end();)
		{
			bool expired = iter->second.update();

			if (expired)
				iter = icons.erase(iter);
			else
				iter++;
		}
	}

	void UIBuffList::update_screen(int16_t new_width, int16_t)
	{
		position = Point<int16_t>(new_width - 35, 55);
		dimension = Point<int16_t>(position.x(), 32);
	}

	Cursor::State UIBuffList::send_cursor(bool pressed, Point<int16_t> cursorposition)
	{
		Point<int16_t> icpos = position;

		for (auto& icon : icons)
		{
			if (icon.second.contains(icpos, cursorposition))
			{
				int32_t bid = icon.second.get_buffid();
				std::string name;

				if (bid >= 0)
					name = SkillData::get(bid).get_name();
				else
					name = ItemData::get(-bid).get_name();

				if (!name.empty())
					UI::get().show_text(Tooltip::Parent::BUFFLIST, name);

				return Cursor::State::IDLE;
			}

			icpos.shift_x(-32);
		}

		UI::get().clear_tooltip(Tooltip::Parent::BUFFLIST);

		return UIElement::send_cursor(pressed, cursorposition);
	}

	UIElement::Type UIBuffList::get_type() const
	{
		return TYPE;
	}

	void UIBuffList::add_buff(int32_t buffid, int32_t duration)
	{
		icons.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(buffid),
			std::forward_as_tuple(buffid, duration)
		);
	}
}