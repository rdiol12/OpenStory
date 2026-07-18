//////////////////////////////////////////////////////////////////////////////////
//	Procedural weapon renderer — see ProceduralWeapon.h.
//////////////////////////////////////////////////////////////////////////////////
#include "ProceduralWeapon.h"

#include "../../Data/WeaponData.h"

#include <cmath>

namespace ms
{
	ProceduralWeapon::ProceduralWeapon(nl::node weaponnode, int32_t itemid, const BodyDrawInfo& di)
	{
		nl::node wnode = weaponnode["default"]["weapon"];

		texture = Texture(wnode);
		grip = wnode["map"]["grip"];
		tip = wnode["map"]["tip"];
		dim = Point<int16_t>(texture.width(), texture.height());
		drawinfo = &di;
		type = WeaponData::get(itemid).get_type();
		valid = texture.is_valid();

		// Optional blade glow, authored in the weapon's own canvas space so it
		// rides the exact same transform. Absent -> inactive (no-op).
		glow.load(wnode["effect"]);
	}

	bool ProceduralWeapon::is_valid() const
	{
		return valid;
	}

	void ProceduralWeapon::update()
	{
		glow.update();
	}

	// First-pass rest angle per weapon category (degrees; 0 = blade straight up).
	// These are tuned in-client against the live character — placeholders that read
	// naturally for the initial scale/placement pass.
	static float rest_angle(Weapon::Type type)
	{
		switch (type)
		{
		case Weapon::Type::SWORD_1H:
		case Weapon::Type::AXE_1H:
		case Weapon::Type::MACE_1H: return 22.0f;   // up, slightly forward
		case Weapon::Type::DAGGER:  return 45.0f;   // forward, low
		case Weapon::Type::WAND:
		case Weapon::Type::STAFF:   return 8.0f;    // near-vertical
		case Weapon::Type::SWORD_2H:
		case Weapon::Type::AXE_2H:
		case Weapon::Type::MACE_2H: return 15.0f;   // resting on shoulder
		case Weapon::Type::SPEAR:
		case Weapon::Type::POLEARM: return 40.0f;   // diagonal
		case Weapon::Type::BOW:     return -3.0f;   // held vertical
		case Weapon::Type::CROSSBOW:return 55.0f;   // angled forward
		case Weapon::Type::CLAW:    return 50.0f;   // forward, low
		case Weapon::Type::KNUCKLE: return 62.0f;   // forward
		case Weapon::Type::GUN:     return -82.0f;  // pointed forward (negative =
		                                            // toward the facing direction, not
		                                            // resting back like a melee weapon)
		default:                    return 24.0f;
		}
	}

	// Per-type adjustment to the grip point (along the sprite). Moving the grip up
	// the shaft (negative y) makes a long weapon hang lower so it reads as a
	// two-handed hold with the butt below the hand.
	static Point<int16_t> grip_offset(Weapon::Type type)
	{
		switch (type)
		{
		case Weapon::Type::SPEAR:
		case Weapon::Type::POLEARM: return { 0, -20 };
		default:                    return { 0, 0 };
		}
	}

	ProceduralWeapon::Pose ProceduralWeapon::pose_for(Stance::Id stance, uint8_t frame) const
	{
		const Clothing::Layer L = Clothing::Layer::WEAPON_OVER_HAND;

		switch (stance)
		{
		// Attack stances: point the blade along the live forearm so it tracks the
		// body animation frame-by-frame instead of using a canned arc.
		case Stance::Id::SWINGO1:
		case Stance::Id::SWINGO2:
		case Stance::Id::SWINGO3:
		case Stance::Id::SWINGOF:
		case Stance::Id::SWINGT1:
		case Stance::Id::SWINGT2:
		case Stance::Id::SWINGT3:
		case Stance::Id::SWINGTF:
		case Stance::Id::SWINGP1:
		case Stance::Id::SWINGP2:
		case Stance::Id::SWINGPF:
			return { 0.0f, L, true, 0.0f };            // blade in line with the arm
		case Stance::Id::STABO1:
		case Stance::Id::STABO2:
		case Stance::Id::STABOF:
		case Stance::Id::STABT1:
		case Stance::Id::STABT2:
		case Stance::Id::STABTF:
		case Stance::Id::PRONESTAB:
			return { 0.0f, L, true, 0.0f };            // thrust: blade along the arm
		case Stance::Id::SHOOT1:
		case Stance::Id::SHOOT2:
		case Stance::Id::SHOOTF:
			return { 0.0f, L, true, (type == Weapon::Type::GUN) ? 0.0f : 0.0f };
		case Stance::Id::PRONE:
			return { 92.0f, L, false, 0.0f };
		case Stance::Id::LADDER:
		case Stance::Id::ROPE:
			// Climbing: the character is seen from behind, so slot the weapon
			// onto the BACKWEAPON layer and anchor it to the body (slung across
			// the back) instead of the hand.
			return { 28.0f, Clothing::Layer::BACKWEAPON, false, 0.0f, true };
		default: // STAND1/2, WALK1/2, ALERT, JUMP — held at rest
			return { rest_angle(type), L, false, 0.0f };
		}
	}

	void ProceduralWeapon::draw(Stance::Id stance, Clothing::Layer layer, uint8_t frame, const DrawArgument& args) const
	{
		if (!valid || drawinfo == nullptr)
			return;

		Pose pose = pose_for(stance, frame);

		if (layer != pose.layer)
			return; // emit only on this pose's z-layer

		// A held grip anchors to the arm's hand point (arm_position) — the same
		// anchor authored weapons use for their "hand" map point. hand_position
		// (HAND_BELOW_WEAPON/handMove) is only populated on some attack frames.
		// When slung on the back (climbing), anchor to the body/navel instead so
		// the weapon sits centered on the character's back.
		Point<int16_t> hand = pose.back
			? drawinfo->get_body_position(stance, frame)
			: drawinfo->get_arm_position(stance, frame);

		constexpr float DEG = 0.017453293f;
		float th;

		if (pose.track_arm)
		{
			// Forearm vector = ARM["hand"] - ARM["navel"] = arm_position - body_position.
			// Angle that makes the blade (canonical: up) point along it:
			//   blade dir at theta is (sin, -cos), so theta = atan2(dx, -dy).
			Point<int16_t> body = drawinfo->get_body_position(stance, frame);
			float dx = static_cast<float>(hand.x() - body.x());
			float dy = static_cast<float>(hand.y() - body.y());
			th = (dx == 0.0f && dy == 0.0f) ? pose.angle * DEG
			                                : std::atan2(dx, -dy) + pose.arm_offset * DEG;
		}
		else
		{
			th = pose.angle * DEG;
		}
		float c = std::cos(th);
		float s = std::sin(th);

		float hw = dim.x() / 2.0f;
		float hh = dim.y() / 2.0f;
		// Per-type hold adjustment (e.g. hold a pike lower for a two-handed look) —
		// only at rest; during attacks the grip stays at the true pivot so the
		// swing/thrust rotates cleanly.
		Point<int16_t> g = (pose.track_arm || pose.back) ? grip : grip + grip_offset(type);
		float gx = g.x() - hw; // grip relative to sprite centre
		float gy = g.y() - hh;

		bool flip = args.get_xscale() < 0.0f;

		// Place the sprite so the rotated grip lands on the hand anchor.
		// (Rotation is about the rect centre; this offset compensates. 0 px error.)
		// The engine mirrors the sprite for facing-left but still rotates by the
		// DrawArgument angle, so the blade direction is (sin, -cos) either way —
		// facing-left must NEGATE the angle to point the mirrored direction.
		float py = hand.y() - hh - (gx * s + gy * c);
		float px = flip ? (hw - hand.x() - (gx * c + gy * s))
		                : (hand.x() - hw - (gx * c - gy * s));

		// A gun is chiral (distinct top/bottom: sight up, grip down). Pointing it
		// forward needs the angle negation above, which REFLECTS the sprite — a
		// symmetric blade doesn't care, but the gun lands upside-down. Re-mirror
		// the gun sprite about its own Y axis to restore chirality: the barrel
		// direction is unchanged (it depends only on the draw angle), only the
		// sight side swaps. The mirror moves the grip, so add the compensating
		// shift that pins it back onto the hand.
		bool gun = (type == Weapon::Type::GUN);
		if (gun)
		{
			float fsign = flip ? -1.0f : 1.0f;
			px += 2.0f * fsign * (hw + gx * c);
			py += 2.0f * fsign * (gx * s);
		}

		// The placement above is in full sprite pixels. When the whole look is
		// drawn scaled (e.g. shrunk to ~0.28x on the minimap), the weapon texture
		// scales with `args`, but this positional shift must scale too — otherwise
		// the weapon lands a full-size offset away from the tiny character and
		// smears across the minimap. At world scale |scale| == 1, so this is a
		// no-op there.
		float sx = std::fabs(args.get_xscale());
		float sy = std::fabs(args.get_yscale());
		Point<int16_t> shift(static_cast<int16_t>(std::lround(px * sx)), static_cast<int16_t>(std::lround(py * sy)));

		float draw_angle = flip ? -th : th;

		// Compose onto the character args (keeps its flip + flip-centre), add
		// rotation. For a gun the extra flip=true mirrors the sprite (chirality fix).
		DrawArgument wargs = (args + shift) + DrawArgument(draw_angle, Point<int16_t>(0, 0), gun, 1.0f);

		// The glow is authored in the same canvas, so the identical transform makes
		// it wrap the blade — behind, then the weapon, then in front.
		glow.draw_back(wargs, 1.0f);
		texture.draw(wargs);
		glow.draw_front(wargs, 1.0f);
	}
}
