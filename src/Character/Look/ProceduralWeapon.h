//////////////////////////////////////////////////////////////////////////////////
//	Procedural weapon renderer.
//
//	A "procedural" weapon carries ONE canonical bitmap (blade up, handle down) plus
//	a single grip pivot, instead of authored per-stance frames. The client poses it
//	for every stance by placing the grip on the character's hand anchor and rotating
//	about the grip by a per-Weapon::Type motion profile:
//
//		screen(p) = hand + R(theta) * (p - grip)      (facing left: mirror about hand.x)
//
//	The engine rotates a sprite about its rect centre, so the placement is derived so
//	the rotated grip lands exactly on the hand (validated to 0 px). No pipeline change.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../../Graphics/Texture.h"
#include "../../Graphics/DrawArgument.h"
#include "../../Graphics/CharacterAura.h"
#include "../../Template/Point.h"
#include "../Inventory/Weapon.h"
#include "BodyDrawInfo.h"
#include "Clothing.h"
#include "Stance.h"

#include <nlnx/node.hpp>

namespace ms
{
	class ProceduralWeapon
	{
	public:
		ProceduralWeapon() = default;
		// Build from the weapon img node (expects default/weapon + map/grip, no stand1).
		ProceduralWeapon(nl::node weaponnode, int32_t itemid, const BodyDrawInfo& drawinfo);

		bool is_valid() const;

		// Advance the (optional) blade-glow animation.
		void update();

		// Draw for the given stance/frame, but only when 'layer' matches the pose's
		// z-layer (so it slots into CharLook's existing weapon draw order).
		void draw(Stance::Id stance, Clothing::Layer layer, uint8_t frame, const DrawArgument& args) const;

	private:
		struct Pose
		{
			float angle;              // degrees; 0 = blade straight up (canonical)
			Clothing::Layer layer;    // which weapon z-layer to emit on
			bool track_arm;           // true = derive angle from the live forearm vector
			float arm_offset;         // degrees added to the forearm angle when tracking
			bool back = false;        // true = slung on the back (climbing), anchored to
			                          // the body instead of the hand
		};

		Pose pose_for(Stance::Id stance, uint8_t frame) const;

		Texture texture;
		CharacterAura glow;   // optional blade effect (default/weapon/effect), rides the transform
		Point<int16_t> grip;
		Point<int16_t> tip;
		Point<int16_t> dim;                       // canonical canvas (96 x 96)
		const BodyDrawInfo* drawinfo = nullptr;   // points at the process-wide static
		Weapon::Type type = Weapon::Type::NONE;
		bool valid = false;
	};
}
