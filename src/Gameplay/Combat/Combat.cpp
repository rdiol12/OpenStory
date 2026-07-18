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
#include "Combat.h"

#include "../../Character/SkillId.h"
#include "../../Data/SkillData.h"
#include "../../IO/Messages.h"
#include "../../IO/KeyAction.h"

#include "../../MapleStory.h"

#include <cstdlib>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include "../../Net/Packets/AttackAndSkillPackets.h"
#include "../../Net/Packets/GameplayPackets.h"

namespace ms
{
	Combat::Combat(Player& in_player, MapChars& in_chars, MapMobs& in_mobs, MapReactors& in_reactors, const Physics& in_physics) : player(in_player), chars(in_chars), mobs(in_mobs), reactors(in_reactors), physics(in_physics),
		attackresults([&](const AttackResult& attack) { apply_attack(attack); }),
		bulleteffects([&](const BulletEffect& effect) { apply_bullet_effect(effect); }),
		damageeffects([&](const DamageEffect& effect) { apply_damage_effect(effect); }) {}

	void Combat::draw(double viewx, double viewy, float alpha) const
	{
		for (auto& be : bullets)
			be.bullet.draw(viewx, viewy, alpha);

		for (auto& dn : damagenumbers)
			dn.draw(viewx, viewy, alpha);
	}

	void Combat::update()
	{
		if (teleport_cooldown > 0)
			teleport_cooldown--;

		attackresults.update();
		bulleteffects.update();
		damageeffects.update();

		bullets.remove_if(
			[&](BulletEffect& mb)
			{
				int32_t target_oid = mb.damageeffect.target_oid;

				if (mobs.contains(target_oid))
				{
					mb.target = mobs.get_mob_head_position(target_oid);
					bool apply = mb.bullet.update(mb.target);

					if (apply)
						apply_damage_effect(mb.damageeffect);

					return apply;
				}
				else
				{
					return mb.bullet.update(mb.target);
				}
			}
		);

		damagenumbers.remove_if(
			[](DamageNumber& dn)
			{
				return dn.update();
			}
		);
	}

	void Combat::use_move(int32_t move_id)
	{
		if (!player.can_attack())
			return;

		const SpecialMove& move = get_move(move_id);
		SpecialMove::ForbidReason reason = player.can_use(move);
		Weapon::Type weapontype = player.get_stats().get_weapontype();

		switch (reason)
		{
		case SpecialMove::ForbidReason::FBR_NONE:
			apply_move(move);
			break;
		default:
			ForbidSkillMessage(reason, weapontype).drop();
			break;
		}
	}

	void Combat::apply_move(const SpecialMove& move)
	{
		if (move.is_attack())
		{
			Attack attack = player.prepare_attack(move.is_skill());

			move.apply_useeffects(player);
			move.apply_actions(player, attack.type);

			player.set_afterimage(move.get_id());

			move.apply_stats(player, attack);

			Point<int16_t> origin = attack.origin;
			Rectangle<int16_t> range = attack.range;
			int16_t hrange = static_cast<int16_t>(range.left() * attack.hrange);

			if (attack.toleft)
			{
				range = {
					static_cast<int16_t>(origin.x() + hrange),
					static_cast<int16_t>(origin.x() + range.right()),
					static_cast<int16_t>(origin.y() + range.top()),
					static_cast<int16_t>(origin.y() + range.bottom())
				};
			}
			else
			{
				range = {
					static_cast<int16_t>(origin.x() - range.right()),
					static_cast<int16_t>(origin.x() - hrange),
					static_cast<int16_t>(origin.y() + range.top()),
					static_cast<int16_t>(origin.y() + range.bottom())
				};
			}

			// This approach should also make it easier to implement PvP
			uint8_t mobcount = attack.mobcount;
			AttackResult result = attack;

			MapObjects* mob_objs = mobs.get_mobs();
			MapObjects* reactor_objs = reactors.get_reactors();

			std::vector<int32_t> mob_targets = find_closest(mob_objs, range, origin, mobcount, true);
			std::vector<int32_t> reactor_targets = find_closest(reactor_objs, range, origin, mobcount, false);

			mobs.send_attack(result, attack, mob_targets, mobcount);
			result.attacker = player.get_oid();
			extract_effects(player, move, result);

			apply_use_movement(move);
			apply_result_movement(move, result);

			AttackPacket(result).dispatch();

			if (reactor_targets.size())
				if (Optional<Reactor> reactor = reactor_objs->get(reactor_targets.at(0)))
				{
					// Stance must NOT be 0 or 2 — Cosmic's hitReactor()
					// silently rejects those for reactor type 2 (the
					// most common attackable reactor type). Use the
					// attack's stance byte (player facing + attack
					// frame), which is always a non-zero value derived
					// from the actual swing animation.
					int16_t stance = static_cast<int16_t>(result.stance);
					if (stance == 0 || stance == 2)
						stance = attack.toleft ? 1 : 3;
					DamageReactorPacket(reactor->get_oid(), player.get_position(), stance, 0).dispatch();
				}
		}
		else
		{
			move.apply_useeffects(player);
			move.apply_actions(player, Attack::Type::MAGIC);

			// Non-attack movement skills (Teleport / Flash Jump) blink the player.
			apply_use_movement(move);

			int32_t moveid = move.get_id();
			int32_t level = player.get_skills().get_level(moveid);
			UseSkillPacket(moveid, level).dispatch();
		}
	}

	std::vector<int32_t> Combat::find_closest(MapObjects* objs, Rectangle<int16_t> range, Point<int16_t> origin, uint8_t objcount, bool use_mobs) const
	{
		std::multimap<uint16_t, int32_t> distances;

		for (auto& mmo : *objs)
		{
			if (use_mobs)
			{
				const Mob* mob = static_cast<const Mob*>(mmo.second.get());

				if (mob && mob->is_alive() && mob->is_in_range(range))
				{
					int32_t oid = mob->get_oid();
					uint16_t distance = mob->get_position().distance(origin);
					distances.emplace(distance, oid);
				}
			}
			else
			{
				// Assume Reactor
				const Reactor* reactor = static_cast<const Reactor*>(mmo.second.get());

				if (reactor && reactor->is_hittable() && reactor->is_in_range(range))
				{
					int32_t oid = reactor->get_oid();
					uint16_t distance = reactor->get_position().distance(origin);
					distances.emplace(distance, oid);
				}
			}
		}

		std::vector<int32_t> targets;

		for (auto& iter : distances)
		{
			if (targets.size() >= objcount)
				break;

			targets.push_back(iter.second);
		}

		return targets;
	}

	void Combat::apply_use_movement(const SpecialMove& move)
	{
		switch (move.get_id())
		{
		case SkillId::Id::TELEPORT_FP:
		case SkillId::Id::IL_TELEPORT:
		case SkillId::Id::PRIEST_TELEPORT:
		case SkillId::Id::GM_TELEPORT:
		case SkillId::Id::SUPERGM_TELEPORT:
			apply_teleport(move);
			break;
		case SkillId::Id::FLASH_JUMP:
			// TODO: Flash Jump is a mid-air dash, not a ground-snapping blink —
			// needs its own handling; left as a no-op for now.
		default:
			break;
		}
	}

	void Combat::apply_teleport(const SpecialMove& move)
	{
		// Client-side rate limit (~350ms) — the base skill carries no data
		// cooldown, so without this you could blink every frame.
		if (teleport_cooldown > 0)
			return;

		// Distance comes from the skill's level data (hrange * 100), matching the
		// classic ~130px blink; fall back if the data is missing.
		constexpr int16_t DEFAULT_RANGE = 130;
		int32_t skillid = move.get_id();
		int32_t level = player.get_skilllevel(skillid);
		int16_t range = static_cast<int16_t>(SkillData::get(skillid).get_stats(level).hrange * 100.0f);
		if (range <= 0)
			range = DEFAULT_RANGE;

		Point<int16_t> cur = player.get_position();

		bool up = player.is_key_down(KeyAction::Id::UP);
		bool down = player.is_key_down(KeyAction::Id::DOWN);
		bool left = player.is_key_down(KeyAction::Id::LEFT);
		bool right = player.is_key_down(KeyAction::Id::RIGHT);

		int16_t border = physics.get_fht().get_borders().second();
		Point<int16_t> target = cur;

		if (up || down)
		{
			// Vertical blink: snap onto the foothold `range` px above (or below,
			// +3 to clear the current contact) the player, if one exists there.
			int16_t probe_y = up
				? static_cast<int16_t>(cur.y() - range)
				: static_cast<int16_t>(cur.y() + 3 + range);

			int16_t ground = physics.get_y_below(Point<int16_t>(cur.x(), probe_y)).y();

			if (ground < border)
				target = Point<int16_t>(cur.x(), ground);
		}
		else
		{
			// Horizontal blink: step toward the destination in 8px increments,
			// stopping at a map wall or a >35px ground-height jump (a cliff/wall),
			// so the player can't blink straight through terrain.
			int16_t dir = left ? -1 : (right ? 1 : (player.is_facing_right() ? 1 : -1));
			Range<int16_t> walls = physics.get_fht().get_walls();
			int16_t base_ground = physics.get_y_below(Point<int16_t>(cur.x(), static_cast<int16_t>(cur.y() - 6))).y();

			int16_t bx = cur.x();
			int16_t by = base_ground;

			for (int16_t step = 8; step <= range; step += 8)
			{
				int16_t nx = static_cast<int16_t>(cur.x() + dir * step);

				if (nx <= walls.first() || nx >= walls.second())
					break;

				int16_t g = physics.get_y_below(Point<int16_t>(nx, static_cast<int16_t>(cur.y() - 6))).y();

				if (g >= border || std::abs(g - base_ground) > 35)
					break;

				bx = nx;
				by = g;
			}

			target = Point<int16_t>(bx, by);
		}

		if (target == cur)
			return;

		player.teleport(target.x(), target.y());

		// The teleport skills carry no own use-effect, so play the standard
		// BasicEff teleport puff on the player at the destination.
		static Animation tp_effect(nl::nx::effect["BasicEff.img"]["Teleport"]);
		player.show_attack_effect(tp_effect, 0);

		teleport_cooldown = 44; // ~350ms
	}

	void Combat::apply_result_movement(const SpecialMove& move, const AttackResult& result)
	{
		switch (move.get_id())
		{
		case SkillId::Id::RUSH_HERO:
		case SkillId::Id::RUSH_PALADIN:
		case SkillId::Id::RUSH_DK:
			apply_rush(result);
			break;
		default:
			break;
		}
	}

	void Combat::apply_rush(const AttackResult& result)
	{
		if (result.mobcount == 0)
			return;

		Point<int16_t> mob_position = mobs.get_mob_position(result.last_oid);
		int16_t targetx = mob_position.x();
		player.rush(targetx);
	}

	void Combat::apply_bullet_effect(const BulletEffect& effect)
	{
		bullets.push_back(effect);

		if (bullets.back().bullet.settarget(effect.target))
		{
			apply_damage_effect(effect.damageeffect);
			bullets.pop_back();
		}
	}

	void Combat::apply_damage_effect(const DamageEffect& effect)
	{
		Point<int16_t> head_position = mobs.get_mob_head_position(effect.target_oid);
		damagenumbers.push_back(effect.number);
		damagenumbers.back().set_x(head_position.x());

		const SpecialMove& move = get_move(effect.move_id);
		mobs.apply_damage(effect.target_oid, effect.damage, effect.toleft, effect.user, move);
	}

	void Combat::push_attack(const AttackResult& attack)
	{
		// Foreign attacks were delayed 400ms here, but the local player's
		// own attack is applied immediately (see apply_move → extract_effects).
		// On fast/one-shot kills the server's KILL_MONSTER removed the mob
		// before the 400ms elapsed, so the foreign damage numbers never
		// rendered and the mob just vanished. Apply promptly instead — the
		// per-hit attack delay inside extract_effects still syncs each
		// number to the attacker's swing.
		attackresults.push(0, attack);
	}

	void Combat::show_mob_damage(int32_t oid, int32_t damage)
	{
		if (damage <= 0 || !mobs.contains(oid))
			return;

		// Float a single number over the mob's head and play its hit
		// reaction, matching how attack damage is displayed.
		int16_t head = mobs.get_mob_head_position(oid).y();
		damagenumbers.emplace_back(DamageNumber::Type::NORMAL, damage, head);
		damagenumbers.back().set_x(mobs.get_mob_head_position(oid).x());

		mobs.apply_damage(oid, damage, false, {}, get_move(0));
	}

	void Combat::apply_attack(const AttackResult& attack)
	{
		if (Optional<OtherChar> ouser = chars.get_char(attack.attacker))
		{
			OtherChar& user = *ouser;
			user.update_skill(attack.skill, attack.level);
			user.update_speed(attack.speed);

			const SpecialMove& move = get_move(attack.skill);
			move.apply_useeffects(user);

			if (Stance::Id stance = Stance::by_id(attack.stance))
				user.attack(stance);
			else
				move.apply_actions(user, attack.type);

			user.set_afterimage(attack.skill);

			extract_effects(user, move, attack);
		}
	}

	void Combat::extract_effects(const Char& user, const SpecialMove& move, const AttackResult& result)
	{
		AttackUser attackuser = {
			user.get_skilllevel(move.get_id()),
			user.get_level(),
			user.is_twohanded(),
			!result.toleft
		};

		if (result.bullet)
		{
			Bullet bullet{
				move.get_bullet(user, result.bullet),
				user.get_position(),
				result.toleft
			};

			for (auto& line : result.damagelines)
			{
				int32_t oid = line.first;

				if (mobs.contains(oid))
				{
					std::vector<DamageNumber> numbers = place_numbers(oid, line.second);
					Point<int16_t> head = mobs.get_mob_head_position(oid);

					size_t i = 0;

					for (auto& number : numbers)
					{
						DamageEffect effect{
							attackuser,
							number,
							line.second[i].first,
							result.toleft,
							oid,
							move.get_id()
						};

						bulleteffects.emplace(user.get_attackdelay(i), std::move(effect), bullet, head);
						i++;
					}
				}
			}

			if (result.damagelines.empty())
			{
				int16_t xshift = result.toleft ? -400 : 400;
				Point<int16_t> target = user.get_position() + Point<int16_t>(xshift, -26);

				for (uint8_t i = 0; i < result.hitcount; i++)
				{
					DamageEffect effect{ attackuser, {}, 0, false, 0, 0 };
					bulleteffects.emplace(user.get_attackdelay(i), std::move(effect), bullet, target);
				}
			}
		}
		else
		{
			for (auto& line : result.damagelines)
			{
				int32_t oid = line.first;

				if (mobs.contains(oid))
				{
					std::vector<DamageNumber> numbers = place_numbers(oid, line.second);

					size_t i = 0;

					for (auto& number : numbers)
					{
						damageeffects.emplace(
							user.get_attackdelay(i),
							attackuser,
							number,
							line.second[i].first,
							result.toleft,
							oid,
							move.get_id()
							);

						i++;
					}
				}
			}
		}
	}

	std::vector<DamageNumber> Combat::place_numbers(int32_t oid, const std::vector<std::pair<int32_t, bool>>& damagelines)
	{
		std::vector<DamageNumber> numbers;
		int16_t head = mobs.get_mob_head_position(oid).y();

		for (auto& line : damagelines)
		{
			int32_t amount = line.first;
			bool critical = line.second;
			DamageNumber::Type type = critical ? DamageNumber::Type::CRITICAL : DamageNumber::Type::NORMAL;
			numbers.emplace_back(type, amount, head);

			head -= DamageNumber::rowheight(critical);
		}

		return numbers;
	}

	void Combat::show_buff(int32_t cid, int32_t skillid, int8_t level)
	{
		if (Optional<OtherChar> ouser = chars.get_char(cid))
		{
			OtherChar& user = *ouser;
			user.update_skill(skillid, level);

			const SpecialMove& move = get_move(skillid);
			move.apply_useeffects(user);
			move.apply_actions(user, Attack::Type::MAGIC);
		}
	}

	void Combat::show_player_buff(int32_t skillid)
	{
		get_move(skillid).apply_useeffects(player);
	}

	const SpecialMove& Combat::get_move(int32_t move_id)
	{
		if (move_id == 0)
			return regularattack;

		auto iter = skills.find(move_id);

		if (iter == skills.end())
			iter = skills.emplace(move_id, move_id).first;

		return iter->second;
	}
}