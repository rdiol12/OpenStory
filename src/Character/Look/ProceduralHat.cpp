//////////////////////////////////////////////////////////////////////////////////
//	Procedural hat renderer — see ProceduralHat.h.
//////////////////////////////////////////////////////////////////////////////////
#include "ProceduralHat.h"

#include <cmath>

namespace ms
{
	ProceduralHat::ProceduralHat(nl::node capnode, const BodyDrawInfo& di)
	{
		nl::node c = capnode["default"]["cap"];

		texture = Texture(c);
		brow = c["map"]["brow"];
		top = c["map"]["top"];
		dim = Point<int16_t>(texture.width(), texture.height());
		drawinfo = &di;
		valid = texture.is_valid();

		// A full-head helmet (info/replaceHead) hides the head/hair/face entirely,
		// so the hat IS the head. Such art fills the canvas from crown to chin and
		// is centred on the head (and scaled to fit it), while its aura draws behind
		// the whole character. A normal cap keeps the forehead-rim brow behaviour.
		full_head = static_cast<int32_t>(capnode["info"]["replaceHead"]) != 0;

		// Optional rear-view sprite (default/capBack): a full-head helmet shows this
		// while climbing a rope/ladder so it faces away like the original head. Its
		// own map/brow + map/top bracket the art, same convention as the front cap.
		nl::node cb = capnode["default"]["capBack"];

		if (cb && cb.data_type() == nl::node::type::bitmap)
		{
			back_texture = Texture(cb);
			back_brow = cb["map"]["brow"];
			back_top = cb["map"]["top"];
			back_dim = Point<int16_t>(back_texture.width(), back_texture.height());
			has_back_sprite = back_texture.is_valid();
		}

		// Optional looping effect (default/cap/effect/{0,1,2,...}, each a canvas +
		// int delay), authored in the cap's own canvas so it rides the exact same
		// transform. Absent -> inactive (no-op). Same mechanism as the weapon glow.
		effect.load(c["effect"]);
	}

	bool ProceduralHat::is_valid() const
	{
		return valid;
	}

	void ProceduralHat::update()
	{
		effect.update();
	}

	DrawArgument ProceduralHat::transform(Stance::Id stance, uint8_t frame, const DrawArgument& args, bool useback) const
	{
		// The head just translates per stance, so we anchor a fixed canvas point to
		// a fixed head-space point every frame (recomputed here so the hat tracks
		// the body animation). Flip-aware so facing-left mirrors the anchor.
		Point<int16_t> face = drawinfo->getfacepos(stance, frame);

		// Anchors come from whichever sprite we're drawing (front cap or rear capBack).
		const Point<int16_t>& sbrow = useback ? back_brow : brow;
		const Point<int16_t>& stop = useback ? back_top : top;
		const Point<int16_t>& sdim = useback ? back_dim : dim;

		float anchorx, anchory, targetx, targety, k;

		if (full_head)
		{
			// Full-head replacement. The authored 'top' and 'brow' bracket the art's
			// top and bottom edges (crown .. chin), so their midpoint is the skull's
			// centre. Pin it onto the head centre (forehead dropped by half a head)
			// and shrink the art to head size with FIT so it doesn't dwarf the body.
			// FWD/DOWN nudge the whole skull toward the face and down so it seats on
			// the head. Flip-aware: +FWD is always toward whichever way the character
			// faces (negative would push it toward the back of the head).
			constexpr float HEAD_HALF = 8.0f; // brow -> head centre; tune if high/low
			constexpr float FIT = 0.88f;      // art -> head size; tune if big/small
			constexpr float FWD = 4.0f;       // toward the face; negative = toward back
			constexpr float DOWN = 0.0f;      // downward seat; negative raises the hat
			anchorx = (stop.x() + sbrow.x()) / 2.0f;
			anchory = (stop.y() + sbrow.y()) / 2.0f;
			targetx = face.x() + FWD;
			targety = face.y() + HEAD_HALF + DOWN;
			k = FIT;
		}
		else
		{
			// Normal cap: the brow rim sits on the forehead, drawn at native size.
			anchorx = sbrow.x();
			anchory = sbrow.y();
			targetx = face.x();
			targety = face.y();
			k = 1.0f;
		}

		bool flip = args.get_xscale() < 0.0f;
		float oxs = args.get_xscale(); // signed world scale (± minimap magnitude)
		float oys = args.get_yscale();
		float mag = std::fabs(oxs);

		// k == 1 placement: pin the anchor to the head at world/minimap scale. This
		// is exactly the plain cap path.
		float px = flip ? (sdim.x() - targetx - anchorx) : (targetx - anchorx);
		float py = targety - anchory;
		Point<int16_t> shift0(
			static_cast<int16_t>(std::lround(px * mag)),
			static_cast<int16_t>(std::lround(py * mag)));
		DrawArgument base = args + shift0; // pos == center (char args carries both)

		if (k == 1.0f)
			return base;

		// Scale the sprite by k about the anchor: the anchor screen point stays put
		// while the art shrinks to head size (delta compensates the pivot).
		Point<int16_t> delta(
			static_cast<int16_t>(std::lround(oxs * (1.0f - k) * anchorx)),
			static_cast<int16_t>(std::lround(oys * (1.0f - k) * anchory)));
		Point<int16_t> np = base.getpos() + delta;
		return DrawArgument(np, np, Point<int16_t>(0, 0), oxs * k, oys * k, 1.0f, 0.0f);
	}

	void ProceduralHat::draw(Stance::Id stance, Clothing::Layer layer, uint8_t frame, const DrawArgument& args) const
	{
		if (!valid || drawinfo == nullptr)
			return;

		if (layer != Clothing::Layer::CAP)
			return; // emit once, on the main cap layer

		// While climbing, a full-head helmet shows its rear view (if authored) so it
		// faces away like the original head; otherwise the front cap is used.
		bool useback = has_back_sprite && Stance::is_climbing(stance);
		const Texture& tex = useback ? back_texture : texture;

		// The effect (if any) wraps the hat: behind the cap, then the static cap,
		// then in front — so a cap glow rides right on the brim. A ring/glow that
		// should sit behind the whole character is an equipped aura, not a cap
		// effect (see Char::draw / CharacterAura::draw_below).
		DrawArgument hatargs = transform(stance, frame, args, useback);
		effect.draw_back(hatargs, 1.0f);
		tex.draw(hatargs);
		effect.draw_front(hatargs, 1.0f);
	}
}
