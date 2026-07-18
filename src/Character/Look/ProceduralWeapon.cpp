//////////////////////////////////////////////////////////////////////////////////
//	Procedural weapon renderer — see ProceduralWeapon.h.
//////////////////////////////////////////////////////////////////////////////////
#include "ProceduralWeapon.h"

#include "AiSkin.h"

#include "../../Data/WeaponData.h"

#include <cmath>
#include <map>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	ProceduralWeapon::ProceduralWeapon(nl::node weaponnode, int32_t itemid, const BodyDrawInfo& di)
	{
		nl::node wnode = weaponnode["default"]["weapon"];

		texture = Texture(wnode);

		// One canonical shape x any material: an info/aiSkin swatch retextures
		// the canonical bitmap (shaded by its own luminance), riding the same
		// motion profiles — the weapon equivalent of materials-on-shells.
		if (AiSkin::available(itemid, weaponnode["info"]))
		{
			std::string key = "aiweapon/" + std::to_string(itemid) + "/canonical";
			Texture themed = AiSkin::retexture_shell(itemid, wnode, key, false);

			if (themed.is_valid())
				texture = themed;
		}
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

	// Vanilla-measured blade angle for frames whose arm anchor is fused into
	// the body sprite: sample the TYPE's base vanilla weapon (id prefix*10000
	// + 2000, e.g. 1402000 for 2H swords) at the same stance/frame and measure
	// its blade axis from its opaque pixels (principal axis, signed away from
	// its map anchor). Cached per (type, stance, frame). Returns false when the
	// reference has no usable art for that frame.
	static bool reference_angle(Weapon::Type type, Stance::Id stance, uint8_t frame, float& out_rad)
	{
		static std::map<int64_t, float> cache;

		int64_t key = (int64_t(type) << 16) | (int64_t(stance) << 8) | frame;
		auto iter = cache.find(key);

		if (iter != cache.end())
		{
			out_rad = iter->second;

			return out_rad > -100.0f;
		}

		float result = -999.0f;
		int32_t refid = int32_t(type) * 10000 + 2000;
		nl::node part = nl::nx::character["Weapon"]["0" + std::to_string(refid) + ".img"][Stance::names[stance]][frame]["weapon"];

		if (part.data_type() == nl::node::type::bitmap)
		{
			nl::bitmap bmp = part;
			int w = bmp.width();
			int h = bmp.height();

			if (w > 0 && h > 0)
			{
				const uint8_t* px = static_cast<const uint8_t*>(bmp.data());

				double sx = 0.0, sy = 0.0;
				int count = 0;

				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x)
						if (px[(size_t(y) * w + x) * 4 + 3] >= 64)
						{
							sx += x;
							sy += y;
							count++;
						}

				if (count >= 10)
				{
					double cx = sx / count, cy = sy / count;
					double sxx = 0.0, syy = 0.0, sxy = 0.0;

					for (int y = 0; y < h; ++y)
						for (int x = 0; x < w; ++x)
							if (px[(size_t(y) * w + x) * 4 + 3] >= 64)
							{
								sxx += (x - cx) * (x - cx);
								syy += (y - cy) * (y - cy);
								sxy += (x - cx) * (y - cy);
							}

					float axis = 0.5f * std::atan2(float(2.0 * sxy), float(sxx - syy));
					float ax = std::cos(axis);
					float ay = std::sin(axis);

					// Blade points away from the anchor (grip-ish) point
					Point<int16_t> origin = part["origin"];
					Point<int16_t> anchor;

					for (nl::node mapnode : part["map"])
						if (mapnode.data_type() == nl::node::type::vector)
							anchor = mapnode;

					float to_blade_x = float(cx) - float(origin.x() + anchor.x());
					float to_blade_y = float(cy) - float(origin.y() + anchor.y());

					if (ax * to_blade_x + ay * to_blade_y < 0.0f)
					{
						ax = -ax;
						ay = -ay;
					}

					result = std::atan2(ax, -ay);
				}
			}
		}

		cache[key] = result;
		out_rad = result;

		return result > -100.0f;
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
		// anchor authored weapons use for their "hand" map point. When slung on
		// the back (climbing), anchor to the body/navel instead so the weapon
		// sits centered on the character's back.
		Point<int16_t> body = drawinfo->get_body_position(stance, frame);
		Point<int16_t> hand = pose.back
			? body
			: drawinfo->get_arm_position(stance, frame);

		// Some attack frames have NO separate arm part (the arm is fused into
		// the body sprite), which degenerates arm_position to the navel — the
		// weapon then lost its anchor and snapped to a vertical/idle pose
		// mid-swing. Fall back to the handMove anchor when it exists.
		bool anchorless = !pose.back &&
			hand.x() == body.x() && hand.y() == body.y();

		if (anchorless)
		{
			Point<int16_t> handmove = drawinfo->get_hand_position(stance, frame);

			if (handmove.x() != 0 || handmove.y() != 0)
			{
				hand = handmove;
				anchorless = false;
			}
			else
			{
				// Last resort: approximate a held position beside the torso
				hand = body + Point<int16_t>(6, -4);
			}
		}

		constexpr float DEG = 0.017453293f;
		float th;

		if (pose.track_arm)
		{
			// Forearm vector = ARM["hand"] - ARM["navel"] = arm_position - body_position.
			// Angle that makes the blade (canonical: up) point along it:
			//   blade dir at theta is (sin, -cos), so theta = atan2(dx, -dy).
			float dx = static_cast<float>(hand.x() - body.x());
			float dy = static_cast<float>(hand.y() - body.y());

			if (anchorless || (dx == 0.0f && dy == 0.0f))
			{
				// No usable anchor this frame: use the angle MEASURED from the
				// type's vanilla base weapon at this exact stance/frame — the
				// real arc Nexon drew. Canned constants only if even the
				// reference has no art here.
				float measured;

				if (reference_angle(type, stance, frame, measured))
				{
					th = measured;
				}
				else
				{
					bool stab = stance == Stance::Id::STABO1 || stance == Stance::Id::STABO2 ||
						stance == Stance::Id::STABOF || stance == Stance::Id::STABT1 ||
						stance == Stance::Id::STABT2 || stance == Stance::Id::STABTF ||
						stance == Stance::Id::PRONESTAB;

					float arc;

					if (stab)
						arc = frame == 0 ? 75.0f : 95.0f;
					else
						arc = frame == 0 ? -50.0f : frame == 1 ? 60.0f : 120.0f;

					th = arc * DEG;
				}
			}
			else
			{
				th = std::atan2(dx, -dy) + pose.arm_offset * DEG;
			}
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
