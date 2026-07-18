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
#include "Reactor.h"

#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/audio.hpp>
#endif

namespace ms
{
	Reactor::Reactor(int32_t o, int32_t r, int8_t s, Point<int16_t> p) : MapObject(o, p), rid(r), state(s)
	{
		std::string strid = string_format::extend_id(rid, 7);
		src = nl::nx::reactor[strid + ".img"];

		normal = src[0];
		animation_ended = true;
		dead = false;
		hittable = false;

		// A reactor is hittable if ANY of its states defines an `event` block —
		// the WZ flag for "reacts to player input". The old code only checked the
		// SPAWN state, so a reactor the server spawned in a state whose node has
		// no event (yet the reactor is still interactive) read as un-hittable and
		// Combat skipped it. Scanning every state is a strict superset — it can
		// only make more reactors hittable, never fewer — and the server validates
		// the real hit anyway, so being permissive is safe.
		for (auto st : src)
		{
			bool is_number = !st.name().empty();
			for (char c : st.name())
				if (c < '0' || c > '9') { is_number = false; break; }
			if (!is_number)
				continue; // skip `info` and other non-state nodes

			for (auto sub : st)
				if (sub.name() == "event")
				{
					hittable = true;
					break;
				}

			if (hittable)
				break;
		}


		nl::node sndsrc = nl::nx::sound["Reactor.img"][strid];
		hitsound = sndsrc["hit"];
		diesound = sndsrc["break"];
	}

	void Reactor::draw(double viewx, double viewy, float alpha) const
	{
		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);
		Point<int16_t> shift = Point<int16_t>(0, normal.get_origin().y());

		if (animation_ended)
		{
			// Default/idle animation (e.g. horntail reactor floating)
			normal.draw(absp - shift, alpha);
		}
		else
		{
			// Guard: a server state of 0 (or beyond the reactor's frames)
			// would make animations.at(state - 1) throw and crash.
			auto it = animations.find(state - 1);
			if (it != animations.end())
				it->second.draw(DrawArgument(absp - shift), 1.0);
			else
				normal.draw(absp - shift, alpha);
		}
	}

	int8_t Reactor::update(const Physics& physics)
	{
		physics.move_object(phobj);

		if (!animation_ended)
		{
			auto it = animations.find(state - 1);
			animation_ended = (it != animations.end()) ? it->second.update() : true;
		}

		if (animation_ended && dead)
			deactivate();

		return phobj.fhlayer;
	}

	void Reactor::set_state(int8_t state)
	{
		hitsound.play();

		if (hittable)
		{
			animations[this->state] = src[this->state]["hit"];
			animation_ended = false;
		}

		this->state = state;
	}

	void Reactor::destroy(int8_t, Point<int16_t>)
	{
		diesound.play();
		animations[this->state] = src[this->state]["hit"];
		state++;
		dead = true;
		animation_ended = false;
	}

	bool Reactor::is_hittable() const
	{
		return hittable;
	}

	bool Reactor::is_in_range(const Rectangle<int16_t>& range) const
	{
		if (!active)
			return false;

		Rectangle<int16_t> bounds(Point<int16_t>(-30, -normal.get_dimensions().y()), Point<int16_t>(normal.get_dimensions().x() - 10, 0)); //normal.get_bounds(); //animations.at(stance).get_bounds();
		bounds.shift(get_position());

		return range.overlaps(bounds);
	}
}