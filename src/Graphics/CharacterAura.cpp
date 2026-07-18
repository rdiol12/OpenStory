//////////////////////////////////////////////////////////////////////////////////
//	See CharacterAura.h.														//
//////////////////////////////////////////////////////////////////////////////////
#include "CharacterAura.h"

#include "../Util/Misc.h"

#include <set>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	void CharacterAura::clear()
	{
		back = Animation();
		front = Animation();
		active = false;
		has_back = false;
	}

	void CharacterAura::load(nl::node effect)
	{
		clear();

#ifdef USE_NX
		if (!effect || effect.size() == 0)
			return;

		// Inspect the top level: are the children bitmaps (direct frames) or
		// containers (grouped by z-layer or by frame)?
		size_t topcount = 0;
		bool top_bitmap = false;
		nl::node firstchild;

		for (auto sub : effect)
		{
			if (topcount == 0)
			{
				firstchild = sub;
				top_bitmap = sub.data_type() == nl::node::type::bitmap;
			}

			topcount++;
		}

		if (top_bitmap)
		{
			// Direct frames — single flat animation, drawn in front.
			front = Animation(effect);
			active = true;
			return;
		}

		// Containers. Do the first container's children hold bitmaps?
		bool inner_bitmap = false;

		for (auto sub : firstchild)
		{
			inner_bitmap = sub.data_type() == nl::node::type::bitmap;
			break;
		}

		if (!inner_bitmap)
		{
			// Unexpected deeper nesting — fall back to the first group.
			front = Animation(firstchild);
			active = true;
			return;
		}

		if (topcount <= 2)
		{
			// Few top-level containers => they are z-layers: 0 behind, 1 front.
			if (effect["1"] && effect["1"].size() > 0)
			{
				back = Animation(effect["0"]);
				front = Animation(effect["1"]);
				has_back = true;
			}
			else
			{
				front = Animation(effect["0"]);
			}

			active = true;
			return;
		}

		// Many top-level containers => they are frames; each frame's sub-node
		// "0" is the back layer and "1" the front layer.
		std::set<int16_t> ids;

		for (auto sub : effect)
		{
			int16_t fid = string_conversion::or_default<int16_t>(sub.name(), -1);

			if (fid >= 0)
				ids.insert(fid);
		}

		std::vector<nl::node> backframes;
		std::vector<nl::node> frontframes;

		for (int16_t fid : ids)
		{
			nl::node frame = effect[std::to_string(fid)];

			if (frame["0"] && frame["0"].data_type() == nl::node::type::bitmap)
				backframes.push_back(frame["0"]);

			if (frame["1"] && frame["1"].data_type() == nl::node::type::bitmap)
				frontframes.push_back(frame["1"]);
		}

		if (!frontframes.empty())
		{
			front = Animation(frontframes);

			if (!backframes.empty())
			{
				back = Animation(backframes);
				has_back = true;
			}
		}
		else if (!backframes.empty())
		{
			// Only one layer present — draw it in front.
			front = Animation(backframes);
		}

		active = true;
#else
		(void)effect;
#endif
	}

	void CharacterAura::update()
	{
		if (!active)
			return;

		if (has_back)
			back.update();

		front.update();
	}

	void CharacterAura::draw_back(const DrawArgument& args, float alpha) const
	{
		if (active && has_back)
			back.draw(args, alpha);
	}

	void CharacterAura::draw_front(const DrawArgument& args, float alpha) const
	{
		if (active)
			front.draw(args, alpha);
	}

	void CharacterAura::draw_below(const DrawArgument& args, float alpha) const
	{
		if (!active)
			return;

		// A split effect draws its 0-layer behind and its 1-layer in front (see
		// draw_above). A flat effect (a ring/glow authored without a 0/1 split)
		// belongs ENTIRELY behind the character so it frames the body instead of
		// covering it — draw the whole thing here.
		if (has_back)
			back.draw(args, alpha);
		else
			front.draw(args, alpha);
	}

	void CharacterAura::draw_above(const DrawArgument& args, float alpha) const
	{
		// Only an explicit split effect has a front layer to lay over the body; a
		// flat effect already drew fully behind in draw_below.
		if (active && has_back)
			front.draw(args, alpha);
	}
}
