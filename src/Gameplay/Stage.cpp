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
#include "Stage.h"

#include "../Configuration.h"

#include "../IO/UI.h"

#include "../IO/UITypes/UIStatusBar.h"
#include "../Net/Packets/AttackAndSkillPackets.h"
#include "../Net/Packets/GameplayPackets.h"
#include "../Graphics/GraphicsGL.h"
#include "../Character/MapleStat.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Stage::Stage() : combat(player, chars, mobs, reactors)
	{
		state = State::INACTIVE;
	}

	void Stage::init()
	{
		drops.init();
	}

	void Stage::load(int32_t mapid, int8_t portalid)
	{
		switch (state)
		{
			case State::INACTIVE:
				load_map(mapid);
				respawn(portalid);
				break;
			case State::TRANSITION:
				respawn(portalid);
				break;
		}

		state = State::ACTIVE;
	}

	void Stage::loadplayer(const CharEntry& entry)
	{
		player = entry;
		playable = player;

		start = ContinuousTimer::get().start();

		CharStats stats = player.get_stats();
		levelBefore = stats.get_stat(MapleStat::Id::LEVEL);
		expBefore = stats.get_exp();
	}

	void Stage::clear()
	{
		state = State::INACTIVE;

		chars.clear();
		npcs.clear();
		mobs.clear();
		summons.clear();
		dragons.clear();
		drops.clear();
		doors.clear();
		mists.clear();
		reactors.clear();
	}

	void Stage::load_map(int32_t mapid)
	{
		Stage::mapid = mapid;

		std::string strid = string_format::extend_id(mapid, 9);
		std::string prefix = std::to_string(mapid / 100000000);

		nl::node src = mapid == -1 ? nl::nx::ui["CashShopPreview.img"] : nl::nx::map["Map"]["Map" + prefix][strid + ".img"];

		tilesobjs = MapTilesObjs(src);
		backgrounds = MapBackgrounds(src["back"]);
		physics = Physics(src["foothold"]);
		mapinfo = MapInfo(src, physics.get_fht().get_walls(), physics.get_fht().get_borders());
		portals = MapPortals(src["portal"], mapid);
		environments = MapEnvironments(src["env"]);
	}

	void Stage::respawn(int8_t portalid)
	{
		Music(mapinfo.get_bgm()).play();

		Point<int16_t> spawnpoint = portals.get_portal_by_id(portalid);
		Point<int16_t> startpos = physics.get_y_below(spawnpoint);
		player.respawn(startpos, mapinfo.is_underwater());
		camera.set_position(startpos);
		camera.set_view(mapinfo.get_walls(), mapinfo.get_borders());
	}

	void Stage::draw(float alpha) const
	{
		if (state != State::ACTIVE)
			return;

		Point<int16_t> viewpos = camera.position(alpha);
		Point<double> viewrpos = camera.realposition(alpha);
		double viewx = viewrpos.x();
		double viewy = viewrpos.y();

		uint8_t gfx_quality = Setting<GraphicsQuality>::get().load();

		backgrounds.drawbackgrounds(viewx, viewy, alpha);

		for (auto id : Layer::IDs)
		{
			tilesobjs.draw(id, viewpos, alpha);
			reactors.draw(id, viewx, viewy, alpha);
			npcs.draw(id, viewx, viewy, alpha);
			mobs.draw(id, viewx, viewy, alpha);
			summons.draw(id, viewx, viewy, alpha);
			dragons.draw(id, viewx, viewy, alpha);
			chars.draw(id, viewx, viewy, alpha);
			player.draw(id, viewx, viewy, alpha);
			drops.draw(id, viewx, viewy, alpha);
			doors.draw(id, viewx, viewy, alpha);

			if (gfx_quality > 25)
				mists.draw(id, viewx, viewy, alpha);
		}

		combat.draw(viewx, viewy, alpha);
		portals.draw(viewpos, alpha);

		if (gfx_quality > 10)
			backgrounds.drawforegrounds(viewx, viewy, alpha);

		if (gfx_quality > 25)
			environments.draw(viewx, viewy, alpha);

		effect.draw();

		if (gfx_quality > 50)
			weather.draw(alpha);

		// HP/MP warning screen overlay
		uint8_t hp_threshold = Setting<HPWarning>::get().load();
		uint8_t mp_threshold = Setting<MPWarning>::get().load();

		if (hp_threshold > 0 || mp_threshold > 0)
		{
			const CharStats& stats = player.get_stats();
			int32_t cur_hp = stats.get_stat(MapleStat::Id::HP);
			int32_t max_hp = stats.get_total(EquipStat::Id::HP);
			int32_t cur_mp = stats.get_stat(MapleStat::Id::MP);
			int32_t max_mp = stats.get_total(EquipStat::Id::MP);

			if (max_hp > 0 && hp_threshold > 0)
			{
				int32_t hp_pct = cur_hp * 100 / max_hp;

				if (hp_pct <= hp_threshold)
				{
					float intensity = 1.0f - static_cast<float>(hp_pct) / hp_threshold;
					float pulse_alpha = 0.15f + 0.10f * intensity;
					GraphicsGL::get().drawscreenfill(1.0f, 0.0f, 0.0f, pulse_alpha);
				}
			}

			if (max_mp > 0 && mp_threshold > 0)
			{
				int32_t mp_pct = cur_mp * 100 / max_mp;

				if (mp_pct <= mp_threshold)
				{
					float intensity = 1.0f - static_cast<float>(mp_pct) / mp_threshold;
					float pulse_alpha = 0.12f + 0.08f * intensity;
					GraphicsGL::get().drawscreenfill(0.0f, 0.0f, 1.0f, pulse_alpha);
				}
			}
		}
	}

	void Stage::update()
	{
		if (state != State::ACTIVE)
			return;

		combat.update();
		backgrounds.update();
		environments.update();
		effect.update();
		weather.update();
		tilesobjs.update();

		reactors.update(physics);
		npcs.update(physics);
		mobs.update(physics);
		summons.update(physics);
		dragons.update(physics);
		chars.update(physics);
		drops.update(physics);
		doors.update(physics);
		mists.update(physics);
		player.update(physics);

		portals.update(player.get_position());
		camera.update(player.get_position());

		if (!player.is_climbing() && !player.is_sitting() && !player.is_attacking())
		{
			if (player.is_key_down(KeyAction::Id::UP) && !player.is_key_down(KeyAction::Id::DOWN))
				check_ladders(true);

			if (player.is_key_down(KeyAction::Id::UP))
				check_portals();

			if (player.is_key_down(KeyAction::Id::DOWN))
				check_ladders(false);

			if (player.is_key_down(KeyAction::Id::SIT))
				check_seats();

			if (player.is_key_down(KeyAction::Id::ATTACK))
				combat.use_move(0);

			if (player.is_key_down(KeyAction::Id::PICKUP))
				check_drops();
		}

		if (player.is_invincible())
			return;

		if (int32_t oid_id = mobs.find_colliding(player.get_phobj()))
		{
			if (MobAttack attack = mobs.create_attack(oid_id))
			{
				MobAttackResult result = player.damage(attack);
				TakeDamagePacket(result, TakeDamagePacket::From::TOUCH).dispatch();
			}
		}
	}

	void Stage::show_character_effect(int32_t cid, CharEffect::Id effect)
	{
		if (auto character = get_character(cid))
			character->show_effect_id(effect);
	}

	void Stage::check_portals()
	{
		if (player.is_attacking())
			return;

		Point<int16_t> playerpos = player.get_position();
		Portal::WarpInfo warpinfo = portals.find_warp_at(playerpos);

		if (warpinfo.intramap)
		{
			Point<int16_t> spawnpoint = portals.get_portal_by_name(warpinfo.toname);
			Point<int16_t> startpos = physics.get_y_below(spawnpoint);

			player.respawn(startpos, mapinfo.is_underwater());
		}
		else if (warpinfo.valid)
		{
			ChangeMapPacket(false, -1, warpinfo.name, false).dispatch();

			CharStats& stats = Stage::get().get_player().get_stats();

			stats.set_mapid(warpinfo.mapid);

			Sound(Sound::Name::PORTAL).play();
		}
	}

	void Stage::check_seats()
	{
		if (player.is_sitting() || player.is_attacking())
			return;

		Optional<const Seat> seat = mapinfo.findseat(player.get_position());
		player.set_seat(seat);
	}

	void Stage::check_ladders(bool up)
	{
		if (!player.can_climb() || player.is_climbing() || player.is_attacking())
			return;

		Optional<const Ladder> ladder = mapinfo.findladder(player.get_position(), up);
		player.set_ladder(ladder);
	}

	void Stage::check_drops()
	{
		Point<int16_t> playerpos = player.get_position();
		MapDrops::Loot loot = drops.find_loot_at(playerpos);

		if (loot.first)
			PickupItemPacket(loot.first, loot.second).dispatch();
	}

	void Stage::send_key(KeyType::Id type, int32_t action, bool down)
	{
		if (state != State::ACTIVE || !playable)
			return;

		switch (type)
		{
			case KeyType::Id::ACTION:
				playable->send_action(KeyAction::actionbyid(action), down);
				break;
			case KeyType::Id::SKILL:
				combat.use_move(action);
				break;
			case KeyType::Id::ITEM:
				if (down)
					player.use_item(action);
				break;
			case KeyType::Id::FACE:
				player.set_expression(action);
				break;
		}
	}

	Cursor::State Stage::send_cursor(bool pressed, Point<int16_t> position)
	{
		auto statusbar = UI::get().get_element<UIStatusBar>();

		if (statusbar && statusbar->is_menu_active())
		{
			if (pressed)
				statusbar->remove_menus();

			if (statusbar->is_in_range(position))
				return statusbar->send_cursor(pressed, position);
		}

		// Check characters first, then NPCs
		Cursor::State char_state = chars.send_cursor(pressed, position, camera.position());
		if (char_state != Cursor::State::IDLE)
			return char_state;

		return npcs.send_cursor(pressed, position, camera.position());
	}

	bool Stage::is_player(int32_t cid) const
	{
		return cid == player.get_oid();
	}

	MapNpcs& Stage::get_npcs()
	{
		return npcs;
	}

	MapChars& Stage::get_chars()
	{
		return chars;
	}

	MapMobs& Stage::get_mobs()
	{
		return mobs;
	}

	MapReactors& Stage::get_reactors()
	{
		return reactors;
	}

	MapDrops& Stage::get_drops()
	{
		return drops;
	}

	MapDoors& Stage::get_doors()
	{
		return doors;
	}

	MapMists& Stage::get_mists()
	{
		return mists;
	}

	MapSummons& Stage::get_summons()
	{
		return summons;
	}

	MapDragons& Stage::get_dragons()
	{
		return dragons;
	}

	Player& Stage::get_player()
	{
		return player;
	}

	Combat& Stage::get_combat()
	{
		return combat;
	}

	Optional<Char> Stage::get_character(int32_t cid)
	{
		if (is_player(cid))
			return player;
		else
			return chars.get_char(cid);
	}

	int Stage::get_mapid()
	{
		return mapid;
	}

	void Stage::add_effect(std::string path)
	{
		effect = MapEffect(path);
	}

	void Stage::set_weather(const std::string& path, const std::string& message)
	{
		nl::node src = nl::nx::map.resolve(path);

		weather.set(src, path, message);
	}

	void Stage::clear_weather()
	{
		weather.clear();
	}

	void Stage::toggle_environment(const std::string& name, int32_t mode)
	{
		environments.toggle(name, mode);
	}

	int64_t Stage::get_uptime()
	{
		return ContinuousTimer::get().stop(start);
	}

	uint16_t Stage::get_uplevel()
	{
		return levelBefore;
	}

	int64_t Stage::get_upexp()
	{
		return expBefore;
	}

	void Stage::transfer_player()
	{
		PlayerMapTransferPacket().dispatch();

		if (Configuration::get().get_admin())
			AdminEnterMapPacket(AdminEnterMapPacket::Operation::ALERT_ADMINS).dispatch();
	}

	void Stage::set_clock(int8_t hour, int8_t min, int8_t sec)
	{
		clock_hour = hour;
		clock_min = min;
		clock_sec = sec;
		clock_active = true;
		countdown_active = false;
	}

	void Stage::set_countdown(int32_t seconds)
	{
		countdown_seconds = seconds;
		countdown_active = true;
		clock_active = false;
	}

	void Stage::clear_clock()
	{
		clock_active = false;
		countdown_active = false;
	}

	bool Stage::is_clock_active() const
	{
		return clock_active;
	}

	bool Stage::is_countdown_active() const
	{
		return countdown_active;
	}

	int8_t Stage::get_clock_hour() const
	{
		return clock_hour;
	}

	int8_t Stage::get_clock_min() const
	{
		return clock_min;
	}

	int8_t Stage::get_clock_sec() const
	{
		return clock_sec;
	}

	int32_t Stage::get_countdown_seconds() const
	{
		return countdown_seconds;
	}

	void Stage::set_combo(int32_t count)
	{
		combo_count = count;
	}

	int32_t Stage::get_combo() const
	{
		return combo_count;
	}

	bool Stage::is_combo_active() const
	{
		return combo_count > 0;
	}

}