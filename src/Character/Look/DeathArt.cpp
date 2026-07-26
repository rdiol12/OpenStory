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
#include "DeathArt.h"

#include "CharLook.h"

#include "../../Graphics/Texture.h"

#include "../../Constants.h"

#include <cmath>
#include <vector>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		struct GhostFrame
		{
			Texture body;
			Point<int16_t> neck;
			uint16_t delay;
		};

		struct Art
		{
			std::vector<Texture> fall;
			Texture land;
			std::vector<GhostFrame> ghost;
			bool loaded = false;
		};

		Art& death_art()
		{
			static Art art;

			if (!art.loaded)
			{
				art.loaded = true;

				nl::node tomb = nl::nx::effect["Tomb.img"];

				for (nl::node f : tomb["fall"])
					art.fall.emplace_back(f);

				art.land = tomb["land"]["0"];

				// Ghost variant "1" is the little soul; the character's
				// head composites onto it via the neck map points
				nl::node stand = nl::nx::character["00002000.img"]["ghoststand"]["1"];

				for (nl::node frame : stand)
				{
					if (!frame["body"])
						continue;

					GhostFrame gf;
					gf.body = frame["body"];
					gf.neck = frame["body"]["map"]["neck"];
					gf.delay = frame["delay"].get_integer() > 0
						? static_cast<uint16_t>(frame["delay"].get_integer()) : 500;
					art.ghost.push_back(gf);
				}
			}

			return art;
		}
	}

	void DeathArt::reset()
	{
		tomb_yoff = -420;
		tomb_landed = false;
		tomb_frame = 0;
		tomb_elapsed = 0;
		ghost_frame = 0;
		ghost_elapsed = 0;
		ghost_bob = 0;
	}

	void DeathArt::update()
	{
		auto& art = death_art();

		if (!tomb_landed)
		{
			tomb_yoff += 9;
			tomb_elapsed += Constants::TIMESTEP;

			if (tomb_elapsed >= 30)
			{
				tomb_elapsed = 0;
				tomb_frame = static_cast<uint8_t>((tomb_frame + 1) % (art.fall.empty() ? 1 : art.fall.size()));
			}

			if (tomb_yoff >= 0)
			{
				tomb_yoff = 0;
				tomb_landed = true;
			}
		}

		if (!art.ghost.empty())
		{
			ghost_elapsed += Constants::TIMESTEP;
			ghost_bob++;

			if (ghost_elapsed >= art.ghost[ghost_frame % art.ghost.size()].delay)
			{
				ghost_elapsed = 0;
				ghost_frame = static_cast<uint8_t>((ghost_frame + 1) % art.ghost.size());
			}
		}
	}

	void DeathArt::draw(Point<int16_t> absp, bool flip, const CharLook& look, float alpha) const
	{
		auto& art = death_art();

		if (!tomb_landed && !art.fall.empty())
		{
			art.fall[tomb_frame % art.fall.size()].draw(
				DrawArgument(absp + Point<int16_t>(0, tomb_yoff)));
		}
		else if (art.land.is_valid())
		{
			art.land.draw(DrawArgument(absp));
		}

		// The soul hovers over the tomb with a slow bob
		if (!art.ghost.empty())
		{
			int16_t bob = static_cast<int16_t>(std::sinf(ghost_bob / 45.0f) * 5.0f);
			const GhostFrame& g = art.ghost[ghost_frame % art.ghost.size()];
			Point<int16_t> gpos = absp + Point<int16_t>(0, -30 + bob);

			g.body.draw(DrawArgument(gpos, flip));

			// The soul wears the character's own head: anchor a fake char
			// origin so the STAND1/0 head neck lands on the ghost's neck map
			const Body* body = look.get_body();
			const Hair* hair = look.get_hair();
			const Face* face = look.get_face();

			if (body && hair && face)
			{
				const BodyDrawInfo& di = CharLook::get_drawinfo();
				Point<int16_t> ns = g.neck - di.get_neck_position(Stance::Id::STAND1, 0);

				if (flip)
					ns = Point<int16_t>(-ns.x(), ns.y());

				DrawArgument hargs(gpos + ns, flip);
				DrawArgument faceargs = hargs + DrawArgument{ di.getfacepos(Stance::Id::STAND1, 0), false, Point<int16_t>{} };

				hair->draw(Stance::Id::STAND1, Hair::Layer::BELOWBODY, 0, hargs);
				body->draw(Stance::Id::STAND1, Body::Layer::HEAD, 0, hargs);
				hair->draw(Stance::Id::STAND1, Hair::Layer::SHADE, 0, hargs);
				hair->draw(Stance::Id::STAND1, Hair::Layer::DEFAULT, 0, hargs);
				face->draw(Expression::Id::DEFAULT, 0, faceargs);
				hair->draw(Stance::Id::STAND1, Hair::Layer::OVERHEAD, 0, hargs);
			}
		}
	}
}
