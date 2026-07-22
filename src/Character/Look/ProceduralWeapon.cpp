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
	ProceduralWeapon::ProceduralWeapon(nl::node weaponnode, int32_t id, const BodyDrawInfo& di)
	{
		itemid = id;
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

		// Canonical pixels for pose baking (retextured when material present)
		{
			int16_t cw = 0, ch = 0;
			AiSkin::shell_pixels(itemid, weaponnode["info"], wnode, false, canon, cw, ch);
		}
		grip = wnode["map"]["grip"];
		tip = wnode["map"]["tip"];
		dim = Point<int16_t>(texture.width(), texture.height());

		// Opaque extent of the canonical sprite (blade up): top = tip end,
		// bottom = butt end, cx = the shaft's horizontal mass center. Used to
		// map the vanilla donor's fractional hold point onto this sprite.
		{
			int w = dim.x(), h = dim.y();
			double sx = 0.0;
			int n = 0, top = h, bot = -1;

			if (!canon.empty() && w > 0 && h > 0)
				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x)
						if (canon[(size_t(y) * w + x) * 4 + 3] >= 64)
						{
							sx += x;
							n++;

							if (y < top) top = y;
							if (y > bot) bot = y;
						}

			if (n > 0)
			{
				canon_cx = static_cast<float>(sx / n);
				canon_top = static_cast<int16_t>(top);
				canon_bot = static_cast<int16_t>(bot);
			}
		}
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

	// Full pose measured from the TYPE's base vanilla weapon (id prefix*10000
	// + 2000, e.g. 1402000 for 2H swords) at a given stance/frame: blade angle
	// (principal axis of the opaque pixels, signed away from the hand anchor),
	// the hand's fractional position along the sprite's axial extent, the
	// sideways offset of the sprite's mass from the hand, and the authored
	// z-layer. angle stays sentinel when the donor has no usable art.
	struct RefPose
	{
		float angle = -999.0f;    // radians; <= -100 means no usable art
		float frac = 0.35f;       // hand position along the axis, 0 = butt, 1 = tip
		float perp = 0.0f;        // mass offset perpendicular to the axis, px
		Clothing::Layer layer = Clothing::Layer::WEAPON;

		bool valid() const { return angle > -100.0f; }
	};

	// hold_first: when the donor stance has fewer frames than the body
	// animation, hold its first frame instead of reporting no art. Right for
	// rest stances; wrong for attacks, where freezing frame 0 would stall the
	// swing arc mid-animation.
	static const RefPose& reference_pose(Weapon::Type type, Stance::Id stance, uint8_t frame, bool hold_first = false)
	{
		static std::map<int64_t, RefPose> cache;

		int64_t key = (int64_t(hold_first) << 32) | (int64_t(type) << 16) | (int64_t(stance) << 8) | frame;
		auto iter = cache.find(key);

		if (iter != cache.end())
			return iter->second;

		RefPose result;
		int32_t refid = int32_t(type) * 10000 + 2000;
		nl::node stnode = nl::nx::character["Weapon"]["0" + std::to_string(refid) + ".img"][Stance::names[stance]];
		nl::node part = stnode[frame]["weapon"];

		if (hold_first && part.data_type() != nl::node::type::bitmap)
			part = stnode["0"]["weapon"];

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

					// Anchor = where the hand lands when the part is drawn;
					// prefer the "hand" map point over navel/handMove
					Point<int16_t> origin = part["origin"];
					Point<int16_t> anchor;
					bool hashand = false;

					for (nl::node mapnode : part["map"])
						if (mapnode.data_type() == nl::node::type::vector)
						{
							if (mapnode.name() == "hand")
							{
								anchor = mapnode;
								hashand = true;
							}
							else if (!hashand)
							{
								anchor = mapnode;
							}
						}

					float anchor_x = float(origin.x() + anchor.x());
					float anchor_y = float(origin.y() + anchor.y());

					// Blade points away from the anchor (grip-ish) point
					float to_blade_x = float(cx) - anchor_x;
					float to_blade_y = float(cy) - anchor_y;

					if (ax * to_blade_x + ay * to_blade_y < 0.0f)
					{
						ax = -ax;
						ay = -ay;
					}

					result.angle = std::atan2(ax, -ay);

					// Project the opaque pixels onto the blade axis relative to
					// the anchor: [smin, smax] spans butt -> tip and the anchor
					// sits at s = 0, so its fraction along that span is where
					// the donor's hand actually holds the weapon
					double smin = 1e9, smax = -1e9;

					for (int y = 0; y < h; ++y)
						for (int x = 0; x < w; ++x)
							if (px[(size_t(y) * w + x) * 4 + 3] >= 64)
							{
								double s = (x - anchor_x) * ax + (y - anchor_y) * ay;

								if (s < smin) smin = s;
								if (s > smax) smax = s;
							}

					if (smax - smin > 1.0)
					{
						double f = (0.0 - smin) / (smax - smin);
						result.frac = static_cast<float>(f < 0.0 ? 0.0 : f > 1.0 ? 1.0 : f);
					}

					// Sideways offset of the mass from the hand (e.g. a gun's
					// barrel riding above the trigger grip)
					result.perp = static_cast<float>(
						(cx - anchor_x) * -ay + (cy - anchor_y) * ax);

					std::string zs = part["z"].get_string();
					auto zit = Clothing::sublayernames.find(zs);

					if (zit != Clothing::sublayernames.end())
						result.layer = zit->second;
				}
			}
		}

		return cache.emplace(key, result).first->second;
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
		{
			// Rest pose measured from the type's vanilla base weapon at this
			// stance/frame, so each category rests the way Nexon drew it (2H
			// on the shoulder behind the head, spear across the body, bow
			// vertical) — angle, hold point and z-layer all derived, no
			// per-type tables. Canned angle only if the donor has no art.
			const RefPose& ref = reference_pose(type, stance, frame, true);

			if (ref.valid())
			{
				Pose pose{ ref.angle / 0.017453293f, ref.layer, false, 0.0f };
				pose.ref_rest = true;
				pose.hold_frac = ref.frac;
				pose.perp = ref.perp;

				return pose;
			}

			return { rest_angle(type), L, false, 0.0f };
		}
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
				const RefPose& ref = reference_pose(type, stance, frame);

				if (ref.valid())
				{
					th = ref.angle;
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
		// Thrust foreshortening: an extending blade points partly toward the
		// viewer, so shorten it along its axis for the depth read
		float axial = 1.0f;

		if (pose.track_arm)
		{
			bool is_stab = stance == Stance::Id::STABO1 || stance == Stance::Id::STABO2 ||
				stance == Stance::Id::STABOF || stance == Stance::Id::STABT1 ||
				stance == Stance::Id::STABT2 || stance == Stance::Id::STABTF ||
				stance == Stance::Id::PRONESTAB;

			if (is_stab)
				axial = frame == 0 ? 0.9f : 0.78f;
		}

		// Pose pivot (the point placed on the hand). Donor-measured rest poses
		// map the vanilla hand fraction onto this sprite's own opaque extent,
		// so a longer custom blade is still held at the same relative point;
		// otherwise the authored grip (plus the per-type rest adjustment).
		Point<int16_t> g;

		if (pose.ref_rest && canon_bot > canon_top)
		{
			float py = canon_bot - pose.hold_frac * float(canon_bot - canon_top);
			g = Point<int16_t>(
				static_cast<int16_t>(std::lround(canon_cx - pose.perp)),
				static_cast<int16_t>(std::lround(py)));
		}
		else
		{
			g = (pose.track_arm || pose.back) ? grip : grip + grip_offset(type);
		}

		// Baked pose sprite: supersampled rotation + outline re-ink, cached per
		// (stance, frame). Its origin is the rotated grip, so it draws like any
		// authored equip part — the engine's mirror handles facing.
		const Texture& posedtex = posed(stance, frame, th, axial, g);

		if (!posedtex.is_valid())
			return;

		bool flip = args.get_xscale() < 0.0f;

		// Positional shift scales with the args (minimap) and mirrors with facing
		float sx = std::fabs(args.get_xscale());
		float sy = std::fabs(args.get_yscale());
		Point<int16_t> shift(
			static_cast<int16_t>(std::lround((flip ? -hand.x() : hand.x()) * sx)),
			static_cast<int16_t>(std::lround(hand.y() * sy)));

		DrawArgument wargs = args + shift;

		// The glow keeps the live-rotation path (soft light — sampling artifacts
		// are invisible there) so it still wraps the blade angle
		DrawArgument gargs = wargs + DrawArgument(flip ? -th : th, Point<int16_t>(0, 0), false, 1.0f);

		glow.draw_back(gargs, 1.0f);
		posedtex.draw(wargs);
		glow.draw_front(gargs, 1.0f);
	}

	const Texture& ProceduralWeapon::posed(Stance::Id stance, uint8_t frame, float th, float axial, Point<int16_t> pivot) const
	{
		int32_t key = int32_t(stance) * 256 + frame;
		auto iter = posed_cache.find(key);

		if (iter != posed_cache.end())
			return iter->second;

		Texture baked;
		std::vector<uint8_t> out;
		int16_t out_w = 0, out_h = 0;
		Point<int16_t> out_pivot;

		if (!canon.empty() &&
			AiSkin::transform_pixels(canon, dim.x(), dim.y(), pivot, th, axial, out, out_w, out_h, out_pivot))
		{
			baked = Texture::from_pixels(
				"procwpn/" + std::to_string(itemid) + "/" + std::to_string(key),
				out_w, out_h, std::move(out), out_pivot);
		}

		return posed_cache.emplace(key, std::move(baked)).first->second;
	}
}
