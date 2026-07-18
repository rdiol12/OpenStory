//////////////////////////////////////////////////////////////////////////////////
//	Procedural hat renderer (parallel to ProceduralWeapon).
//
//	A "procedural" hat carries ONE canonical bitmap (crown up) plus a single brow
//	anchor. The client draws it at the head's brow every stance:
//
//		screen(brow) = head_brow (BodyDrawInfo::getfacepos)     (facing left mirrored)
//
//	Position is always right because the head just translates; head-tilt (prone/jump)
//	is an optional refinement. vslot/hair-layering is handled by the existing cap-type
//	logic (the empty Clothing still carries info/vslot).
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../../Graphics/Texture.h"
#include "../../Graphics/DrawArgument.h"
#include "../../Graphics/CharacterAura.h"
#include "../../Template/Point.h"
#include "BodyDrawInfo.h"
#include "Clothing.h"
#include "Stance.h"

#include <nlnx/node.hpp>

namespace ms
{
	class ProceduralHat
	{
	public:
		ProceduralHat() = default;
		// Build from the cap img node (expects default/cap + map/brow, no stand1).
		ProceduralHat(nl::node capnode, const BodyDrawInfo& drawinfo);

		bool is_valid() const;

		// Advance the (optional) looping cap effect layer.
		void update();

		// Draw for the given stance/frame, but only on the CAP layer.
		void draw(Stance::Id stance, Clothing::Layer layer, uint8_t frame, const DrawArgument& args) const;

	private:
		// Build the flip/scale/anchor transform shared by the hat and its aura.
		// useback selects the rear-view sprite's anchors (climbing).
		DrawArgument transform(Stance::Id stance, uint8_t frame, const DrawArgument& args, bool useback) const;

		Texture texture;
		CharacterAura effect;   // optional looping effect (default/cap/effect), rides the hat transform
		Point<int16_t> brow;
		Point<int16_t> top;
		Point<int16_t> dim;                       // canonical canvas (96 x 96)

		// Optional rear view (default/capBack): a full-head helmet shows this while
		// climbing a rope/ladder so it faces away like the original head.
		Texture back_texture;
		Point<int16_t> back_brow;
		Point<int16_t> back_top;
		Point<int16_t> back_dim;
		bool has_back_sprite = false;

		const BodyDrawInfo* drawinfo = nullptr;
		bool valid = false;
		bool full_head = false;                   // info/replaceHead: anchor on head centre
	};
}
