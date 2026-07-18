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
#include "Clothing.h"

#include "AiSkin.h"

#include "../../Data/WeaponData.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Clothing::Clothing(int32_t id, const BodyDrawInfo& drawinfo) : itemid(id)
	{
		bodydrawinfo = &drawinfo;

		const EquipData& equipdata = EquipData::get(itemid);

		eqslot = equipdata.get_eqslot();

		if (eqslot == EquipSlot::Id::WEAPON)
			twohanded = WeaponData::get(itemid).is_twohanded();
		else
			twohanded = false;

		constexpr size_t NON_WEAPON_TYPES = 15;
		constexpr size_t WEAPON_OFFSET = NON_WEAPON_TYPES + 15;
		constexpr size_t WEAPON_TYPES = 20;

		constexpr Clothing::Layer layers[NON_WEAPON_TYPES] =
		{
			Clothing::Layer::CAP,
			Clothing::Layer::FACEACC,
			Clothing::Layer::EYEACC,
			Clothing::Layer::EARRINGS,
			Clothing::Layer::TOP,
			Clothing::Layer::MAIL,
			Clothing::Layer::PANTS,
			Clothing::Layer::SHOES,
			Clothing::Layer::GLOVE,
			Clothing::Layer::SHIELD,
			Clothing::Layer::CAPE,
			Clothing::Layer::RING,
			Clothing::Layer::PENDANT,
			Clothing::Layer::BELT,
			Clothing::Layer::MEDAL
		};

		Clothing::Layer chlayer;
		size_t index = (itemid / 10000) - 100;

		if (index < NON_WEAPON_TYPES)
			chlayer = layers[index];
		else if (index >= WEAPON_OFFSET && index < WEAPON_OFFSET + WEAPON_TYPES)
			chlayer = Clothing::Layer::WEAPON;
		else
			chlayer = Clothing::Layer::CAPE;

		std::string strid = "0" + std::to_string(itemid);
		std::string category = equipdata.get_itemdata().get_category();
		nl::node src = nl::nx::character[category][strid + ".img"];
		nl::node info = src["info"];

		// AI-skin equips carry an info/aiSkin material bitmap in their .img;
		// every frame is then retextured from it at load (see AiSkin.h)
		const bool has_aiskin = AiSkin::available(itemid, info);


		// Shell equips carry authored silhouette views in info/aiShell: the
		// 'mail' body part is replaced with a NEW silhouette image, pinned to
		// the body's per-frame navel position — the armor's shape is no longer
		// bound to any Nexon donor. Views: upright (required), prone, back,
		// sit (optional; stances without a matching view keep donor art).
		// The image's origin node marks its navel point. Sleeves stay donor
		// parts (arms genuinely need per-frame poses at this scale).
		nl::node shell = info["aiShell"];
		const bool has_shell = shell["upright"].data_type() == nl::node::type::bitmap;

		vslot = (std::string)info["vslot"];

		switch (int32_t standno = info["stand"])
		{
		case 1:
			stand = Stance::Id::STAND1;
			break;
		case 2:
			stand = Stance::Id::STAND2;
			break;
		default:
			stand = twohanded ? Stance::Id::STAND2 : Stance::Id::STAND1;
			break;
		}

		switch (int32_t walkno = info["walk"])
		{
		case 1:
			walk = Stance::Id::WALK1;
			break;
		case 2:
			walk = Stance::Id::WALK2;
			break;
		default:
			walk = twohanded ? Stance::Id::WALK2 : Stance::Id::WALK1;
			break;
		}

		for (auto iter : Stance::names)
		{
			Stance::Id stance = iter.first;
			const std::string& stancename = iter.second;

			nl::node stancenode = src[stancename];

			if (!stancenode)
				continue;

			for (uint8_t frame = 0; nl::node framenode = stancenode[frame]; ++frame)
			{
				// A shell view replaces this frame's parts at most once
				bool shell_used = false;

				for (nl::node partnode : framenode)
				{
					std::string part = partnode.name();

					if (!partnode || partnode.data_type() != nl::node::type::bitmap)
						continue;

					Clothing::Layer z = chlayer;
					std::string zs = (std::string)partnode["z"];

					if (part == "mailArm")
					{
						z = Clothing::Layer::MAILARM;
					}
					else
					{
						auto sublayer_iter = sublayernames.find(zs);

						if (sublayer_iter != sublayernames.end())
							z = sublayer_iter->second;
					}

					std::string parent;
					Point<int16_t> parentpos;

					for (auto mapnode : partnode["map"])
					{
						if (mapnode.data_type() == nl::node::type::vector)
						{
							parent = mapnode.name();
							parentpos = mapnode;
						}
					}

					nl::node mapnode = partnode["map"];
					Point<int16_t> shift;

					switch (eqslot)
					{
					case EquipSlot::Id::FACE:
						shift -= parentpos;
						break;
					case EquipSlot::Id::SHOES:
					case EquipSlot::Id::GLOVES:
					case EquipSlot::Id::TOP:
					case EquipSlot::Id::BOTTOM:
					case EquipSlot::Id::CAPE:
						shift = drawinfo.get_body_position(stance, frame) - parentpos;
						break;
					case EquipSlot::Id::HAT:
					case EquipSlot::Id::EARACC:
					case EquipSlot::Id::EYEACC:
						shift = drawinfo.getfacepos(stance, frame) - parentpos;
						break;
					case EquipSlot::Id::SHIELD:
					case EquipSlot::Id::WEAPON:
						if (parent == "handMove")
							shift += drawinfo.get_hand_position(stance, frame);
						else if (parent == "hand")
							shift += drawinfo.get_arm_position(stance, frame);
						else if (parent == "navel")
							shift += drawinfo.get_body_position(stance, frame);

						shift -= parentpos;
						break;
					}

					// Shell silhouettes replace donor parts with authored views:
					// - body armor: the 'mail' part, pinned to the body's NECK
					//   (the navel drifts laterally within breathing frames — the
					//   body art compensates, a rigid image can't, so shells
					//   floated in idle; the neck is rock-stable and tracks real
					//   torso motion). View origin = collar point.
					// - capes: ALL parts, neck-pinned like body armor (capes
					//   hang from the shoulders and barely animate in v83).
					// Hats DON'T use shells (the mannequin paint-on-head pipeline
					// produces plain cap data); weapons use the procedural
					// one-image system; gloves/shoes/sleeves stay donor art —
					// their frames encode per-frame poses a rigid image can't match.
					const bool cape_shell = eqslot == EquipSlot::Id::CAPE;

					if (has_shell && (part == "mail" || cape_shell))
					{
						// View families: prone, back (climbing), sit, attack
						// (swings/stabs/shoots), else upright
						const bool is_attack =
							stancename.rfind("swing", 0) == 0 ||
							stancename.rfind("stab", 0) == 0 ||
							stancename.rfind("shoot", 0) == 0;

						const char* family = "upright";
						nl::node view = shell["upright"];

						if (stance == Stance::Id::PRONE || stance == Stance::Id::PRONESTAB)
						{
							// No authored prone view -> upright rotated to
							// lying (continuity over donor reversion)
							if (shell["prone"])
							{
								view = shell["prone"];
								family = "prone";
							}
							else
							{
								family = "pronerot";
							}
						}
						else if (Stance::is_climbing(stance))
						{
							if (shell["back"])
							{
								view = shell["back"];
								family = "back";
							}
							else
							{
								// Upright stand-in (continuity over donor
								// reversion)
								family = "upright";
							}
						}
						else if (stance == Stance::Id::SIT && shell["sit"])
						{
							view = shell["sit"];
							family = "sit";
						}
						else if (is_attack && shell["attack"])
						{
							view = shell["attack"];
							family = "attack";
						}
						else if (stance == Stance::Id::JUMP)
						{
							// Airborne legs tuck up — a rigid upright hem clips
							// through them. Prefer the authored jump view; else
							// use the upright view vertically squashed so the
							// hem rides up over the tucked legs (keeps the
							// custom silhouette instead of reverting to donor).
							if (shell["jump"])
							{
								view = shell["jump"];
								family = "jump";
							}
							else
							{
								family = "jumpsq";
							}
						}

						// A view is either a single bitmap or a container of
						// numbered frame bitmaps (0,1,2,...) — an animated
						// shell. Animation rides the body's stance frames, so
						// billowing capes / flickering cloaks need no extra
						// timing data.
						nl::node viewbmp = view;
						uint8_t subframe = 0;

						if (view && view.data_type() != nl::node::type::bitmap)
						{
							uint8_t subcount = 0;

							while (view[std::to_string(subcount)].data_type() == nl::node::type::bitmap)
								subcount++;

							if (subcount > 0)
							{
								subframe = frame % subcount;
								viewbmp = view[std::to_string(subframe)];
							}
						}

						if (viewbmp.data_type() == nl::node::type::bitmap)
						{
							if (!shell_used)
							{
								Point<int16_t> pin = drawinfo.get_neck_position(stance, frame);

								// During attacks the torso leans into the
								// strike — rotate the shell to the body's
								// per-frame spine angle (neck vs navel,
								// relative to the standing baseline) so it
								// follows the lunge instead of staying rigid.
								// Quantized to 5-degree steps for atlas reuse.
								int16_t lean_deg = 0;

								if (is_attack)
								{
									auto spine_angle = [&](Stance::Id st, uint8_t fr)
									{
										Point<int16_t> neck = drawinfo.get_neck_position(st, fr);
										Point<int16_t> navel = drawinfo.get_body_position(st, fr);

										return std::atan2(
											float(neck.x() - navel.x()),
											float(navel.y() - neck.y()));
									};

									float lean = spine_angle(stance, frame) - spine_angle(Stance::Id::STAND1, 0);
									lean = std::clamp(lean, -0.9f, 0.9f);
									lean_deg = int16_t(std::lround(lean * 180.0f / 3.14159265f / 5.0f)) * 5;

									if (std::abs(lean_deg) < 10)
										lean_deg = 0;
								}

								// With a material present the shell is
								// retextured through the aiSkin engine (shaded
								// by its own luminance, trim/decal included) —
								// one shape, any theme. Keyed per family
								// subframe so all stances share atlas entries.
								Texture shelltex;
								std::string skey = "aishell/" + std::to_string(itemid) + "/" + family + "/" + std::to_string(subframe);

								if (lean_deg != 0)
								{
									shelltex = AiSkin::lean_shell(itemid, info, viewbmp,
										skey + "/r" + std::to_string(lean_deg),
										float(lean_deg) * 3.14159265f / 180.0f);
								}
								else if (family == std::string("jumpsq"))
								{
									// Squashed upright (airborne stand-in);
									// handles material and raw alike
									shelltex = AiSkin::squash_shell(itemid, info, viewbmp, skey, 0.82f);
								}
								else if (family == std::string("pronerot"))
								{
									shelltex = AiSkin::prone_shell(itemid, info, viewbmp, skey);
								}
								else if (has_aiskin)
								{
									shelltex = AiSkin::retexture_shell(itemid, viewbmp, skey, family == std::string("prone"));
								}

								if (!shelltex.is_valid())
									shelltex = Texture(viewbmp);

								auto shell_iter = stances[stance][z].emplace(frame, std::move(shelltex));
								shell_iter->second.shift(pin);

								// Optional back piece "<family>Behind" drawn
								// behind the body — open coats, backpacks,
								// wings. Uses the cape depth (below-body cap
								// depth for head slots).
								nl::node behindview = shell[std::string(family) + "Behind"];
								nl::node behindbmp = behindview;

								if (behindview && behindview.data_type() != nl::node::type::bitmap)
								{
									uint8_t bcount = 0;

									while (behindview[std::to_string(bcount)].data_type() == nl::node::type::bitmap)
										bcount++;

									if (bcount > 0)
										behindbmp = behindview[std::to_string(frame % bcount)];
								}

								if (behindbmp.data_type() == nl::node::type::bitmap)
								{
									Clothing::Layer behind_z = Clothing::Layer::CAPE;

									Texture behindtex;

									if (has_aiskin)
									{
										std::string bkey = "aishell/" + std::to_string(itemid) + "/" + family + "Behind/" + behindbmp.name();
										behindtex = AiSkin::retexture_shell(itemid, behindbmp, bkey, family == std::string("prone"));
									}

									if (!behindtex.is_valid())
										behindtex = Texture(behindbmp);

									auto behind_iter = stances[stance][behind_z].emplace(frame, std::move(behindtex));
									behind_iter->second.shift(pin);
								}

								shell_used = true;
							}

							continue;
						}
						// No authored view for this family → donor art below
					}

					Texture parttex(partnode);

					if (has_aiskin)
					{
						Texture synth = AiSkin::synthesize(itemid, stancename, frame, part, partnode);

						if (synth.is_valid())
							parttex = synth;
					}

					auto part_iter = stances[stance][z].emplace(frame, std::move(parttex));
					part_iter->second.shift(shift);
				}
			}
		}

		static const std::unordered_set<int32_t> transparents =
		{
			1002186
		};

		transparent = transparents.count(itemid) > 0;
	}

	void Clothing::draw(Stance::Id stance, Layer layer, uint8_t frame, const DrawArgument& args) const
	{
		auto range = stances[stance][layer].equal_range(frame);
		DrawArgument climbargs = args;

		// Cloned/custom equips (hats especially) frequently have the ladder/rope
		// climbing stances missing, blank, or written as a degenerate 1x1
		// placeholder bitmap — all of which made them disappear while on a rope
		// or ladder. When the climbing stance has no *real* frame for this layer
		// (nothing that is both valid and larger than a 1x1 stub), fall back to
		// the stand1 frames so the piece still shows (same frame first, then
		// frame 0). Back-view-only layers (BACKWEAPON/BACKSHIELD) have no stand1
		// frames, so this stays a no-op for them.
		if (Stance::is_climbing(stance))
		{
			bool has_real = false;
			for (auto it = range.first; it != range.second; ++it)
			{
				Point<int16_t> d = it->second.get_dimensions();
				if (it->second.is_valid() && d.x() > 1 && d.y() > 1) { has_real = true; break; }
			}

			if (!has_real)
			{
				const auto& s1 = stances[Stance::Id::STAND1][layer];
				range = s1.equal_range(frame);
				if (range.first == range.second)
					range = s1.equal_range(0);

				// The stand1 frame is baked for the stand head. For a cap, shift
				// it to the head position of THIS climbing frame so it stays
				// centered on the head and follows the up/down climb bob.
				// getfacepos is in unflipped sprite space; when the character
				// faces left (xscale < 0) the sprite is mirrored about its
				// centre, so the horizontal component of the shift must be
				// mirrored too or the cap lands off to one side.
				if (bodydrawinfo &&
					(layer == Layer::CAP || layer == Layer::CAP_OVER_HAIR || layer == Layer::CAP_BELOW_BODY))
				{
					Point<int16_t> shift = bodydrawinfo->getfacepos(stance, frame)
						- bodydrawinfo->getfacepos(Stance::Id::STAND1, 0);
					if (args.get_xscale() < 0.0f)
						shift = Point<int16_t>(static_cast<int16_t>(-shift.x()), shift.y());
					climbargs = args + shift;
				}
			}
		}

		for (auto iter = range.first; iter != range.second; ++iter)
			iter->second.draw(climbargs);
	}

	bool Clothing::contains_layer(Stance::Id stance, Layer layer) const
	{
		return !stances[stance][layer].empty();
	}

	bool Clothing::is_transparent() const
	{
		return transparent;
	}

	bool Clothing::is_twohanded() const
	{
		return twohanded;
	}

	int32_t Clothing::get_id() const
	{
		return itemid;
	}

	Stance::Id Clothing::get_stand() const
	{
		return stand;
	}

	Stance::Id Clothing::get_walk() const
	{
		return walk;
	}

	EquipSlot::Id Clothing::get_eqslot() const
	{
		return eqslot;
	}

	const std::string& Clothing::get_vslot() const
	{
		return vslot;
	}

	const std::unordered_map<std::string, Clothing::Layer> Clothing::sublayernames =
	{
		// WEAPON
		{ "weaponOverHand",			Clothing::Layer::WEAPON_OVER_HAND	},
		{ "weaponOverGlove",		Clothing::Layer::WEAPON_OVER_GLOVE	},
		{ "weaponOverBody",			Clothing::Layer::WEAPON_OVER_BODY	},
		{ "weaponBelowArm",			Clothing::Layer::WEAPON_BELOW_ARM	},
		{ "weaponBelowBody",		Clothing::Layer::WEAPON_BELOW_BODY	},
		{ "backWeaponOverShield",	Clothing::Layer::BACKWEAPON			},
		// SHIELD
		{ "shieldOverHair",			Clothing::Layer::SHIELD_OVER_HAIR	},
		{ "shieldBelowBody",		Clothing::Layer::SHIELD_BELOW_BODY	},
		{ "backShield",				Clothing::Layer::BACKSHIELD			},
		// GLOVE
		{ "gloveWrist",				Clothing::Layer::WRIST				},
		{ "gloveOverHair",			Clothing::Layer::GLOVE_OVER_HAIR	},
		{ "gloveOverBody",			Clothing::Layer::GLOVE_OVER_BODY	},
		{ "gloveWristOverHair",		Clothing::Layer::WRIST_OVER_HAIR	},
		{ "gloveWristOverBody",		Clothing::Layer::WRIST_OVER_BODY	},
		// CAP
		{ "capOverHair",			Clothing::Layer::CAP_OVER_HAIR		},
		{ "capBelowBody",			Clothing::Layer::CAP_BELOW_BODY		},
	};
}