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

#include "../Data/WeaponData.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Char::Char(int32_t o, const CharLook& lk, const std::string& name) : MapObject(o), look(lk), look_preview(lk), namelabel(Text(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, Text::Background::NONE, name)), guildlabel(Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::MEDIUMBLUE))
	{
		// Default nametag color is plain white. Player/OtherChar call
		// apply_nametag_style(job_id) after construction to load the real
		// NameTag.img style (w/c/e sprite pieces + clr) based on job.
		// We drop Text::Background::NAMETAG (programmatic black rectangle)
		// and instead paint the real sprite pieces in Char::draw.
		name_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
	}

	void Char::draw(double viewx, double viewy, float alpha) const
	{
		Point<int16_t> absp = phobj.get_absolute(viewx, viewy, alpha);

		effects.drawbelow(absp, alpha);

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

		look.draw(DrawArgument(absp, color), alpha);

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

		// Looping cash/etc item aura (set by SHOW_ITEM_EFFECT or SPAWN_CHAR).
		if (item_effect_id != 0)
			item_effect_anim.draw(DrawArgument(absp), alpha);

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

		if (item_effect_id != 0)
			item_effect_anim.update();


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

		uint16_t stancespeed = 0;

		if (speed >= 1.0f / Constants::TIMESTEP)
			stancespeed = static_cast<uint16_t>(Constants::TIMESTEP * speed);

		afterimage.update(look.get_frame(), stancespeed);

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
			item_effect_anim = Animation();
			return;
		}

#ifdef USE_NX
		nl::node src = nl::nx::effect["ItemEff.img"][std::to_string(itemid)]["0"];
		if (src)
			item_effect_anim = Animation(src);
		else
			item_effect_id = 0;
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
		state = st;

		Stance::Id stance = Stance::by_state(state);
		look.set_stance(stance);
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

	void Char::apply_nametag_style(int32_t job_id)
	{
#ifdef USE_NX
		nl::node nt = nl::nx::ui["NameTag.img"];

		// Candidate styles, first match wins. GM/SuperGM get the yellow
		// style (11) so they're visually distinct; otherwise we prefer the
		// job-specific style if the NX dump happens to carry one, then
		// fall back through the generic "0","10","3","4","5" chain.
		std::vector<std::string> candidates;

		if (job_id == 900 || job_id == 910)
			candidates.push_back("11");

		if (job_id > 0)
			candidates.push_back(std::to_string(job_id));

		candidates.insert(candidates.end(), { "0", "10", "3", "4", "5" });

		nl::node style;
		for (const auto& k : candidates)
		{
			if (nt[k] && nt[k].size() > 0)
			{
				style = nt[k];
				break;
			}
		}

		if (!style) return;

		// Load the 9-slice sprite pieces (w = left edge, c = tiled center,
		// e = right edge). These are the actual in-game nametag background.
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
		(void)job_id;
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