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
#include "Char.h"

#include "Look/AiSkin.h"

#include "../Data/WeaponData.h"
#include "../Data/EquipData.h"

#include "../Graphics/GraphicsGL.h"

#include "../Constants.h"
#include "../Gameplay/MiniRooms.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#ifdef USE_NX
#include "../Graphics/Geometry.h"

#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace
	{
		// Shared death art: the falling/landed tomb (Effect/Tomb.img) and
		// the hovering ghost (the ghost stance baked into the body files)
		struct GhostFrame
		{
			Texture body;
			Point<int16_t> head_shift;
			uint16_t delay;
		};

		struct DeathArt
		{
			std::vector<Texture> fall;
			Texture land;
			std::vector<GhostFrame> ghost;
			Texture head;
			bool loaded = false;
		};

		DeathArt& death_art()
		{
			static DeathArt art;

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
				nl::node headnode = nl::nx::character["00012000.img"]["front"]["head"];

				art.head = headnode;
				Point<int16_t> head_neck = headnode["map"]["neck"];

				for (nl::node frame : stand)
				{
					if (!frame["body"])
						continue;

					GhostFrame gf;
					gf.body = frame["body"];
					Point<int16_t> body_neck = frame["body"]["map"]["neck"];
					gf.head_shift = body_neck - head_neck;
					gf.delay = frame["delay"].get_integer() > 0
						? static_cast<uint16_t>(frame["delay"].get_integer()) : 500;
					art.ghost.push_back(gf);
				}
			}

			return art;
		}
	}

	// Aura pivot offset from absp (= character feet). Seeds validated by an
	// offline render across stand/walk/jump; the neck barely moves so these are
	// single constants. On prone a lying body can't be described by a vertical
	// offset, so anchor to the base instead of floating a marker above it.
	static Point<int16_t> aura_pivot_offset(int16_t pivot, bool prone)
	{
		if (prone)
			return { 0, 0 };

		switch (pivot)
		{
		case 1:  return { 0, -44 }; // head (neck -31 + ~13 head radius)
		case 2:  return { 0, 0 };   // feet
		default: return { 0, -20 }; // center (navel)
		}
	}

	Char::Char(int32_t o, const CharLook& lk, const std::string& name) : MapObject(o), look(lk), look_preview(lk), namelabel(Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NAMETAG, name)), guildlabel(Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::MEDIUMBLUE))
	{
		// Default nametag color is plain white. Player/OtherChar call
		// apply_nametag_style(job_id) after construction to load the real
		// NameTag.img style (w/c/e sprite pieces + clr) based on job.
		// We drop Text::Background::NAMETAG (programmatic black rectangle)
		// and instead paint the real sprite pieces in Char::draw.
		name_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
		sit_offset = Point<int16_t>(0, 0);
	}

	void Char::draw(double viewx, double viewy, float alpha) const
	{
		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha) + sit_offset;

		effects.drawbelow(absp, alpha);

		// Aura layers that sit behind the character. A flat (unsplit) aura — e.g. a
		// ring or glow — draws its whole self here so it frames the body instead of
		// covering it; a split aura draws only its 0-layer here (1-layer below).
		item_aura.draw_below(DrawArgument(absp), alpha);
		gm_effect.draw_below(DrawArgument(absp), alpha);

		bool aura_prone = look.get_stance() == Stance::Id::PRONE;

		// Shared by the below/above aura passes: stance gating, and draw args
		// with pivot + motion drag + scale + facing flip + tint
		auto aura_visible = [&](const AuraInstance& a)
		{
			switch (a.show)
			{
			case 1:  return state != State::LADDER && state != State::ROPE;
			case 2:  return state == State::STAND || state == State::SIT;
			default: return true;
			}
		};

		auto aura_args = [&](const AuraInstance& a)
		{
			Point<int16_t> pos = absp + aura_pivot_offset(a.pivot, aura_prone) + Point<int16_t>(
				static_cast<int16_t>(std::round(aura_drag_x * a.drag)),
				static_cast<int16_t>(std::round(aura_drag_y * a.drag)));

			// Head pivot rides the per-frame head bob (delta from the stance's
			// first frame, so the calibrated base offset stays valid)
			if (a.pivot == 1 && !aura_prone)
			{
				Stance::Id st = look.get_stance();
				pos += CharLook::get_drawinfo().get_neck_position(st, look.get_frame())
					- CharLook::get_drawinfo().get_neck_position(st, 0);
			}

			float xscale = (a.flip && !facing_right) ? -a.scale : a.scale;

			return DrawArgument(pos, pos, Point<int16_t>(0, 0), xscale, a.scale, a.tint, 0.0f);
		};

		for (const auto& a : equip_auras)
		{
			if (!aura_visible(a))
				continue;

			if (a.blend == 1)
				GraphicsGL::get().setblend(true);

			a.aura.draw_below(aura_args(a), alpha);

			if (a.blend == 1)
				GraphicsGL::get().setblend(false);
		}

		Color color;

		if (invincible)
		{
			float phi = invincible.alpha() * 30;
			float rgb = 0.9f - 0.5f * std::abs(std::sinf(phi));

			color = Color(rgb, rgb, rgb, 1.0f);
		}
		else if (is_hidden())
		{
			color = Color(1.0f, 1.0f, 1.0f, 0.5f);
		}
		else
		{
			color = Color::Code::CWHITE;
		}

		if (riding_mount != 0 && state != State::DIED)
			current_mount_ani().draw(DrawArgument(absp, facing_right), alpha);

		if (state == State::DIED)
			draw_death(absp, alpha);
		else if (riding_mount != 0)
			look.draw(DrawArgument(absp + mount_navel + Point<int16_t>(0, 14), color), alpha);
		else
			look.draw(DrawArgument(absp, color), alpha);

		if (const auto* box = MiniRooms::get().find(get_oid()))
		{
			static Texture psskins[7];
			static bool skins_loaded = false;

			if (!skins_loaded)
			{
				skins_loaded = true;
				nl::node ps = nl::nx::ui["ChatBalloon.img"]["miniroom"]["PSSkin"];

				for (int i = 0; i < 7; i++)
					psskins[i] = ps[std::to_string(i)];
			}

			static Text shoptext(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);

			int8_t skin = (box->skin >= 0 && box->skin < 7 && psskins[box->skin].is_valid()) ? box->skin : 0;
			const Texture& sign = psskins[skin];
			int16_t w = sign.get_dimensions().x();
			int16_t h = sign.get_dimensions().y();
			Point<int16_t> tl(absp.x() - w / 2, absp.y() - 78 - h);

			sign.draw(DrawArgument(tl + sign.get_origin()));

			std::string d = box->desc.size() > 14 ? box->desc.substr(0, 14) : box->desc;
			shoptext.change_text(d);
			shoptext.draw(tl + Point<int16_t>(w / 2, skin == 0 ? 9 : 68));
		}

		// Only show the swing trail during an actual attack stance. Otherwise a
		// trail whose animation outlasts a short attack keeps drawing over the
		// character at idle (a stray stance frame can satisfy the frame gate),
		// which read as a stray glow on the face.
		if (Stance::is_attack(look.get_stance()))
			afterimage.draw(look.get_frame(), DrawArgument(absp, facing_right), alpha);

		if (ironbody)
		{
			float ibalpha = ironbody.alpha();
			float scale = 1.0f + ibalpha;
			float opacity = 1.0f - ibalpha;

			look.draw(DrawArgument(absp, scale, scale, opacity), alpha);
		}

		for (auto& pet : pets)
			if (pet.get_itemid())
				pet.draw(viewx, viewy, alpha);

		// Aura layers that sit in front of the character — only a split aura's
		// explicit 1-layer; a flat aura already drew fully behind (draw_below).
		item_aura.draw_above(DrawArgument(absp), alpha);
		gm_effect.draw_above(DrawArgument(absp), alpha);

		for (const auto& a : equip_auras)
		{
			if (!aura_visible(a))
				continue;

			if (a.blend == 1)
				GraphicsGL::get().setblend(true);

			a.aura.draw_above(aura_args(a), alpha);

			if (a.blend == 1)
				GraphicsGL::get().setblend(false);
		}

		// If ever changing code for namelabel confirm placements with map 10000
		// v83 nametag is a 9-slice sprite from NameTag.img/<style>:
		//   w (left edge)  — origin is its right corner  (pivot on the c join)
		//   c (center)     — origin is top-left; we stretch it to text width
		//   e (right edge) — origin is top-left          (pivot on the c join)
		// Drawing all three at the text anchor point lets each sprite's
		// origin offset do the vertical alignment automatically, so the bar
		// sits centered on the name glyphs. The name is drawn last, on top.
		Point<int16_t> name_center = absp + Point<int16_t>(0, -4);
		int16_t text_w = namelabel.width();

		if (tag_w.is_valid() || tag_c.is_valid() || tag_e.is_valid())
		{
			int16_t c_total = text_w > 0 ? text_w : 1;
			int16_t half = c_total / 2;
			// Nudge the sprite down a few px so the bar sits below the
			// glyph baseline rather than clipping the ascenders.
			int16_t tag_y = name_center.y() + 5;

			// Left edge (pivot on its right → draw at left-of-center so its
			// right edge meets the center piece).
			tag_w.draw(DrawArgument(Point<int16_t>(
				name_center.x() - half, tag_y)));
			// Stretched center, spanning the text width.
			tag_c.draw(DrawArgument(
				Point<int16_t>(name_center.x() - half, tag_y),
				Point<int16_t>(c_total, 0)));
			// Right edge (pivot on its left, draws from center to right).
			tag_e.draw(DrawArgument(Point<int16_t>(
				name_center.x() + half, tag_y)));
		}

		// Pass name_color via DrawArgument so drawtext multiplies the glyph
		// wordcolor (WHITE) by it, yielding the exact ARGB from NameTag.img.
		namelabel.draw(DrawArgument(name_center, name_color));

		// Draw guild name below character name
		if (!guildlabel.get_text().empty())
			guildlabel.draw(absp + Point<int16_t>(0, 8));

		// party overhead HP bar — same look and behaviour as the mob bar
		if (party_maxhp > 0)
		{
			static MobHpBar partybar;
			int16_t percent = static_cast<int16_t>(
				party_hp > 0 ? (100 * party_hp) / party_maxhp : 0);
			if (percent > 100)
				percent = 100;
			partybar.draw(absp + Point<int16_t>(0, -58), percent);
		}

		chatballoon.draw(absp - Point<int16_t>(0, 85));

		effects.drawabove(absp, alpha);

		for (auto& number : damagenumbers)
			number.draw(viewx, viewy, alpha);
	}

	void Char::draw_preview(Point<int16_t> position, float alpha) const
	{
		look_preview.draw(position, false, Stance::Id::STAND1, Expression::Id::DEFAULT);
	}

	bool Char::update(const Physics& physics, float speed)
	{
		if (state == State::DIED)
			update_death();

		damagenumbers.remove_if(
			[](DamageNumber& number)
			{
				return number.update();
			}
		);

		effects.update();
		chatballoon.update();
		invincible.update();
		ironbody.update();

		item_aura.update();
		gm_effect.update();

		for (auto& a : equip_auras)
			a.aura.update();

		// Motion drag: auras trail against the velocity and ease back when
		// stopping — flames stream behind a runner, settle on a stander
		float drag_target_x = std::clamp(static_cast<float>(-phobj.hspeed) * 2.0f, -8.0f, 8.0f);
		float drag_target_y = std::clamp(static_cast<float>(-phobj.vspeed) * 1.2f, -6.0f, 6.0f);
		aura_drag_x += (drag_target_x - aura_drag_x) * 0.12f;
		aura_drag_y += (drag_target_y - aura_drag_y) * 0.12f;


		for (auto& pet : pets)
		{
			if (pet.get_itemid())
			{
				switch (state)
				{
				case State::LADDER:
				case State::ROPE:
					pet.set_stance(PetLook::Stance::HANG);
					break;
				case State::SWIM:
					pet.set_stance(PetLook::Stance::FLY);
					break;
				default:
					if (pet.get_stance() == PetLook::Stance::HANG || pet.get_stance() == PetLook::Stance::FLY)
						pet.set_stance(PetLook::Stance::STAND);

					break;
				}

				pet.update(physics, get_position());
			}
		}

		if (riding_mount != 0)
		{
			const_cast<Animation&>(current_mount_ani()).update();
			look.set_stance(Stance::Id::SIT);
		}

		uint16_t stancespeed = 0;

		if (speed >= 1.0f / Constants::TIMESTEP)
			stancespeed = static_cast<uint16_t>(Constants::TIMESTEP * speed);

		afterimage.update(look.get_frame(), stancespeed);

		// Keep the look's audio anchor at the character's world position, so
		// its attack-swing sound plays attenuated by distance (other players
		// attacking across the map are quiet / silent).
		look.set_sound_position(get_position());

		return look.update(stancespeed);
	}

	float Char::get_stancespeed() const
	{
		if (attacking)
			return get_real_attackspeed();

		switch (state)
		{
		case State::WALK:
			return static_cast<float>(std::abs(phobj.hspeed));
		case State::LADDER:
		case State::ROPE:
			return static_cast<float>(std::abs(phobj.vspeed));
		default:
			return 1.0f;
		}
	}

	float Char::get_real_attackspeed() const
	{
		int8_t speed = get_integer_attackspeed();

		return 1.7f - static_cast<float>(speed) / 10;
	}

	uint16_t Char::get_attackdelay(size_t no) const
	{
		uint8_t first_frame = afterimage.get_first_frame();
		uint16_t delay = look.get_attackdelay(no, first_frame);
		float fspeed = get_real_attackspeed();

		return static_cast<uint16_t>(delay / fspeed);
	}

	int8_t Char::update(const Physics& physics)
	{
		update(physics, 1.0f);

		return get_layer();
	}

	int8_t Char::get_layer() const
	{
		return is_climbing() ? 7 : phobj.fhlayer;
	}

	void Char::show_attack_effect(Animation toshow, int8_t z)
	{
		float attackspeed = get_real_attackspeed();

		effects.add(toshow, DrawArgument(facing_right), z, attackspeed);
	}

	void Char::set_item_effect(int32_t itemid)
	{
		item_effect_id = itemid;

		if (itemid == 0)
		{
			item_aura.clear();
			return;
		}

#ifdef USE_NX
		item_aura.load(nl::nx::effect["ItemEff.img"][std::to_string(itemid)]);

		if (!item_aura.is_active())
			item_effect_id = 0;
#endif
	}

	void Char::show_item_use_effect(int32_t itemid)
	{
#ifdef USE_NX
		nl::node base = nl::nx::effect["ItemEff.img"][std::to_string(itemid)];
		nl::node frames = (base["0"].data_type() == nl::node::type::bitmap) ? base : base["0"];

		// Add to the effect layer, which plays it once and removes it when the
		// animation ends — a one-shot use puff, not a looping aura.
		if (frames && frames.size() > 0)
			effects.add(Animation(frames));
#endif
	}

	void Char::refresh_ring_effect()
	{
#ifdef USE_NX
		const CharEquips& equips = look.get_equips();

		// Effect rings drive the item aura.
		static const EquipSlot::Id ringslots[] = {
			EquipSlot::Id::RING1, EquipSlot::Id::RING2,
			EquipSlot::Id::RING3, EquipSlot::Id::RING4
		};

		int32_t ringeffect = 0;

		for (EquipSlot::Id slot : ringslots)
		{
			int32_t ring = equips.get_equip(slot);

			if (ring > 0 && nl::nx::effect["ItemEff.img"][std::to_string(ring)])
			{
				ringeffect = ring;
				break;
			}
		}

		if (ringeffect != 0)
		{
			if (item_effect_id != ringeffect)
				set_item_effect(ringeffect);
		}
		else if (item_effect_id != 0)
		{
			set_item_effect(0);
		}

		// A GM hat drives the GM set effect (SetEff.img). There is no set data
		// in the client NX, so the hat -> set-effect link is mapped explicitly.
		const char* seteff = nullptr;

		switch (equips.get_equip(EquipSlot::Id::HAT))
		{
		case 1002940: seteff = "37"; break;   // GMS hat        -> GM Effect
		case 1002959: seteff = "100"; break;  // Junior GM Cap  -> JR.GM effect
		default: break;
		}

		if (seteff)
			gm_effect.load(nl::nx::effect["SetEff.img"][seteff]["effect"]);
		else
			gm_effect.clear();

		// Data-driven auras: any equipped item may declare info/effect. This is
		// additive to the GM-hat/ring effects above (which are the always-shown
		// "reserved band"); these declared auras are the capped equip band.
		equip_auras.clear();

		for (auto slot : EquipSlot::values)
		{
			int32_t id = equips.get_equip(slot);
			if (id <= 0)
				continue;

			const std::string& category = EquipData::get(id).get_itemdata().get_category();
			nl::node info = nl::nx::character[category]["0" + std::to_string(id) + ".img"]["info"];
			nl::node eff = info["effect"];
			if (!eff)
				continue;

			// String -> resolve by name (bare = CharEff.img, "Folder/name" = escape
			// hatch to a vanilla bucket). Container -> use the subtree inline.
			nl::node effnode;
			if (eff.data_type() == nl::node::type::string)
			{
				std::string name = (std::string)eff;
				size_t slash = name.find('/');
				effnode = (slash != std::string::npos)
					? nl::nx::effect[name.substr(0, slash) + ".img"][name.substr(slash + 1)]
					: nl::nx::effect["CharEff.img"][name];
			}
			else
			{
				effnode = eff;
			}

			if (!effnode)
				continue;

			AuraInstance inst;
			inst.aura.load(effnode);
			if (!inst.aura.is_active())
				continue;

			inst.pivot = static_cast<int16_t>(info["effectPivot"]);
			inst.blend = static_cast<int16_t>(info["effectBlend"]);
			inst.prio = static_cast<int16_t>(info["effectPrio"]);
			inst.show = static_cast<int16_t>(info["effectShow"]);
			inst.flip = static_cast<int32_t>(info["effectFlip"]) != 0;

			if (double scale = info["effectScale"]; scale > 0.0)
				inst.scale = static_cast<float>(scale);

			if (info["effectDrag"].data_type() == nl::node::type::integer ||
				info["effectDrag"].data_type() == nl::node::type::real)
				inst.drag = static_cast<float>(static_cast<double>(info["effectDrag"]));

			float opacity = 1.0f;

			if (double o = info["effectOpacity"]; o > 0.0 && o <= 1.0)
				opacity = static_cast<float>(o);

			// Tint: explicit color wins; else effectTint=1 samples the aiSkin
			// material accent so one shared white/gray template effect matches
			// every armor theme
			if (info["effectTintColor"].data_type() == nl::node::type::integer)
			{
				int64_t rgb = info["effectTintColor"];
				inst.tint = Color(
					((rgb >> 16) & 0xFF) / 255.0f,
					((rgb >> 8) & 0xFF) / 255.0f,
					(rgb & 0xFF) / 255.0f,
					opacity);
			}
			else if (static_cast<int32_t>(info["effectTint"]) != 0)
			{
				float r, g, b;

				if (AiSkin::accent_color(id, info, r, g, b))
					inst.tint = Color(r, g, b, opacity);
				else
					inst.tint = Color(1.0f, 1.0f, 1.0f, opacity);
			}
			else
			{
				inst.tint = Color(1.0f, 1.0f, 1.0f, opacity);
			}

			equip_auras.push_back(std::move(inst));
		}

		// Cap the declared equip auras (GM/ring effects are separate and always
		// shown). Keep the highest-priority ones; ties keep insertion order.
		constexpr size_t AURA_CAP = 3;
		if (equip_auras.size() > AURA_CAP)
		{
			std::stable_sort(equip_auras.begin(), equip_auras.end(),
				[](const AuraInstance& a, const AuraInstance& b) { return a.prio > b.prio; });
			equip_auras.resize(AURA_CAP);
		}
#endif
	}

	void Char::show_effect_id(CharEffect::Id toshow)
	{
		effects.add(chareffects[toshow]);
	}

	void Char::show_iron_body()
	{
		ironbody.set_for(500);
	}

	void Char::show_damage(int32_t damage)
	{
		int16_t start_y = phobj.get_y() - 60;
		int16_t x = phobj.get_x() - 10;

		damagenumbers.emplace_back(DamageNumber::Type::TOPLAYER, damage, start_y, x);

		look.set_alerted(5000);
		invincible.set_for(2000);
	}

	void Char::show_heal(int32_t amount)
	{
		if (amount <= 0)
			return;

		int16_t start_y = phobj.get_y() - 60;
		int16_t x = phobj.get_x();

		damagenumbers.emplace_back(DamageNumber::Type::HEAL, amount, start_y, x);
	}


	void Char::speak(const std::string& line)
	{
		chatballoon.change_text(line);
	}

	void Char::draw_death(Point<int16_t> absp, float alpha) const
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
			bool flip = !facing_right;

			g.body.draw(DrawArgument(gpos, flip));

			// The soul wears the character's face, not the full head
			if (const Face* face = look.get_face())
			{
				Point<int16_t> hs = g.head_shift + Point<int16_t>(0, 6);

				if (flip)
					hs = Point<int16_t>(-hs.x(), hs.y());

				face->draw(Expression::Id::DEFAULT, 0, DrawArgument(gpos + hs, flip));
			}
		}
	}

	void Char::update_death()
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

	void Char::set_guild(const std::string& name)
	{
		guildlabel.change_text(name);
	}

	void Char::set_guild_mark(int16_t, int8_t, int16_t, int8_t)
	{
		// Guild mark rendering requires loading guild emblem sprites
		// from GuildMark.img — stored for future use
	}

	void Char::change_look(MapleStat::Id stat, int32_t id)
	{
		switch (stat)
		{
		case MapleStat::Id::SKIN:
			look.set_body(id);
			break;
		case MapleStat::Id::FACE:
			look.set_face(id);
			break;
		case MapleStat::Id::HAIR:
			look.set_hair(id);
			break;
		}
	}

	void Char::set_state(uint8_t statebyte)
	{
		if (statebyte % 2 == 1)
		{
			set_direction(false);

			statebyte -= 1;
		}
		else
		{
			set_direction(true);
		}

		Char::State newstate = by_value(statebyte);
		set_state(newstate);
	}

	void Char::set_expression(int32_t expid)
	{
		Expression::Id expression = Expression::byaction(expid);
		look.set_expression(expression);
	}

	void Char::attack(const std::string& action)
	{
		look.set_action(action);

		attacking = true;
		look.set_alerted(5000);
	}

	void Char::attack(Stance::Id stance)
	{
		look.attack(stance);

		attacking = true;
		look.set_alerted(5000);
	}

	void Char::attack(bool degenerate)
	{
		look.attack(degenerate);

		attacking = true;
		look.set_alerted(5000);
	}

	void Char::set_afterimage(int32_t skill_id)
	{
		int32_t weapon_id = look.get_equips().get_weapon();

		if (weapon_id <= 0)
			return;

		const WeaponData& weapon = WeaponData::get(weapon_id);

		std::string stance_name = Stance::names[look.get_stance()];
		int16_t weapon_level = weapon.get_equipdata().get_reqstat(MapleStat::Id::LEVEL);
		const std::string& ai_name = weapon.get_afterimage();

		afterimage = Afterimage(skill_id, ai_name, stance_name, weapon_level);
	}

	const Afterimage& Char::get_afterimage() const
	{
		return afterimage;
	}

	void Char::set_direction(bool f)
	{
		facing_right = f;
		look.set_direction(f);
	}

	void Char::set_state(State st)
	{
		bool was_dead = (state == State::DIED);
		state = st;

		Stance::Id stance = Stance::by_state(state);

		// Death must override any action that was playing (e.g. an attack
		// in progress when HP hit 0); otherwise the dead pose never shows.
		if (st == State::DIED)
		{
			// Clear the attack flag so the death animation plays at normal
			// speed (get_stancespeed returns attack speed while attacking).
			attacking = false;
			look.set_stance_forced(stance);

			// Kick off the tomb drop + ghost hover — only on the
			// transition into death (the server re-sends the dead state,
			// which must not restart the sequence)
			if (!was_dead)
			{
				tomb_yoff = -420;
				tomb_landed = false;
				tomb_frame = 0;
				tomb_elapsed = 0;
				ghost_frame = 0;
				ghost_elapsed = 0;
				ghost_bob = 0;
			}
		}
		else
		{
			look.set_stance(stance);
		}
	}

	void Char::add_pet(uint8_t index, int32_t iid, const std::string& name, int32_t uniqueid, Point<int16_t> pos, uint8_t stance, int32_t fhid)
	{
		if (index > 2)
			return;

		pets[index] = PetLook(iid, name, uniqueid, pos, stance, fhid);
	}

	PetLook& Char::get_pet(uint8_t index)
	{
		return pets[index < 3 ? index : 0];
	}

	void Char::set_party_hp(int32_t hp, int32_t maxhp)
	{
		party_hp = hp;
		party_maxhp = maxhp;
	}

	void Char::clear_party_hp()
	{
		party_hp = 0;
		party_maxhp = 0;
	}

	void Char::apply_nametag_style(bool is_gm)
	{
#ifdef USE_NX
		// The GM set effect is driven by the equipped GM hat (see
		// refresh_ring_effect), not by GM status. Only the name plate here.

		// Like the original game: the decorative name plate is not for everyone.
		// Regular players keep the plain default name (translucent background).
		// Only GMs get a distinct plate.
		if (!is_gm)
			return;

		// First existing style wins ("11" is the distinct GM plate if present,
		// otherwise fall back to whatever the NX actually carries).
		nl::node nt = nl::nx::ui["NameTag.img"];
		nl::node style;

		for (const char* k : { "11", "0", "1", "2", "3", "4", "5", "10" })
		{
			if (nt[k] && nt[k].size() > 0)
			{
				style = nt[k];
				break;
			}
		}

		if (!style)
			return;

		// The plate provides its own background — drop the default one so they
		// don't stack.
		namelabel = Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NONE, namelabel.get_text());

		// Load the 9-slice sprite pieces (w = left edge, c = tiled center,
		// e = right edge).
		tag_w = Texture(style["w"]);
		tag_c = Texture(style["c"]);
		tag_e = Texture(style["e"]);

		if (nl::node clr_node = style["clr"])
		{
			try
			{
				int64_t argb = clr_node.get_integer();
				uint32_t v = static_cast<uint32_t>(argb);
				uint8_t a = static_cast<uint8_t>((v >> 24) & 0xFF);
				uint8_t r = static_cast<uint8_t>((v >> 16) & 0xFF);
				uint8_t g = static_cast<uint8_t>((v >> 8) & 0xFF);
				uint8_t b = static_cast<uint8_t>(v & 0xFF);
				if (a == 0) a = 255;
				name_color = Color(r, g, b, a);
			}
			catch (...) {}
		}
#else
		(void)is_gm;
#endif
	}

	void Char::remove_pet(uint8_t index, bool hunger)
	{
		if (index > 2)
			return;

		pets[index] = PetLook();

		if (hunger)
		{
			// No action for this case
		}
	}

	bool Char::is_invincible() const
	{
		return invincible == true;
	}

	bool Char::is_hidden() const
	{
		return hidden;
	}

	void Char::set_hidden(bool h)
	{
		hidden = h;
	}

	bool Char::is_sitting() const
	{
		return state == State::SIT;
	}

	bool Char::is_climbing() const
	{
		return state == State::LADDER || state == State::ROPE;
	}

	bool Char::is_twohanded() const
	{
		return look.get_equips().is_twohanded();
	}

	Weapon::Type Char::get_weapontype() const
	{
		int32_t weapon_id = look.get_equips().get_weapon();

		if (weapon_id <= 0)
			return Weapon::Type::NONE;

		return WeaponData::get(weapon_id).get_type();
	}

	bool Char::has_pet() const
	{
		for (int i = 0; i < 3; i++)
			if (pets[i].get_itemid() != 0)
				return true;

		return false;
	}

	void Char::set_riding(int32_t mount_itemid)
	{
		if (mount_itemid == riding_mount)
			return;

		riding_mount = mount_itemid;

		if (mount_itemid == 0)
		{
			mount_ani = Animation();
			return;
		}

		std::string strid = std::to_string(mount_itemid);
		while (strid.size() < 8)
			strid.insert(0, 1, '0');

		nl::node base = nl::nx::character["TamingMob"][strid + ".img"];
		nl::node src = base["stand1"];

		if (!src)
			src = base["stand"];

		mount_ani = Animation(src);
		mount_walk = Animation(base["walk1"] ? base["walk1"] : base["walk"]);
		mount_jump = Animation(base["jump"]);

		nl::node navel = src["0"]["0"]["map"]["navel"];

		if (!navel)
			navel = src["0"]["map"]["navel"];

		mount_navel = navel ? Point<int16_t>(navel) : Point<int16_t>(0, -30);
	}

	bool Char::has_mount() const
	{
		return look.get_equips().get_equip(EquipSlot::TAMEDMOB) != 0;
	}

	bool Char::getflip() const
	{
		return facing_right;
	}

	std::string Char::get_name() const
	{
		return namelabel.get_text();
	}

	CharLook& Char::get_look()
	{
		return look;
	}

	const CharLook& Char::get_look() const
	{
		return look;
	}

	PhysicsObject& Char::get_phobj()
	{
		return phobj;
	}

	void Char::init()
	{
		CharLook::init();

		nl::node src = nl::nx::effect["BasicEff.img"];

		for (auto iter : CharEffect::PATHS)
			chareffects.emplace(iter.first, src.resolve(iter.second));
	}

	EnumMap<CharEffect::Id, Animation> Char::chareffects;
}