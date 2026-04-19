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
#include "PlayerHandlers.h"

#include "Helpers/LoginParser.h"

#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"

#include "../../Audio/Audio.h"
#include "../../Character/CharEffect.h"

#include "../../IO/UITypes/UIBuffList.h"
#include "../../IO/UITypes/UICashShop.h"
#include "../../IO/UITypes/UIMonsterBook.h"
#include "../../IO/UITypes/UINotice.h"
#include "../../IO/UITypes/UISkillBook.h"
#include "../../IO/UITypes/UISkillMacro.h"
#include "../../IO/UITypes/UIStatsInfo.h"

#include "../../Net/Packets/GameplayPackets.h"
#include "../../Net/Packets/LoginPackets.h"

namespace ms
{
	void ChangeChannelHandler::handle(InPacket& recv) const
	{
		LoginParser::parse_login(recv);

		auto cashshop = UI::get().get_element<UICashShop>();

		if (cashshop)
		{
			cashshop->exit_cashshop();
		}
		else
		{
			// Normal in-game channel change: after reconnecting to the new
			// channel server we must re-identify the character, otherwise the
			// server has no idea who connected and the client just stalls.
			Player& player = Stage::get().get_player();
			PlayerLoginPacket(player.get_oid()).dispatch();
		}
	}

	void ChangeStatsHandler::handle(InPacket& recv) const
	{
		recv.read_bool(); // 'itemreaction'
		int32_t updatemask = recv.read_int();

		bool recalculate = false;

		for (auto iter : MapleStat::codes)
			if (updatemask & iter.second)
				recalculate |= handle_stat(iter.first, recv);

		if (recalculate)
			Stage::get().get_player().recalc_stats(false);

		UI::get().enable();
	}

	bool ChangeStatsHandler::handle_stat(MapleStat::Id stat, InPacket& recv) const
	{
		Player& player = Stage::get().get_player();

		bool recalculate = false;

		switch (stat)
		{
		case MapleStat::Id::SKIN:
			player.change_look(stat, recv.read_short());
			break;
		case MapleStat::Id::FACE:
		case MapleStat::Id::HAIR:
			player.change_look(stat, recv.read_int());
			break;
		case MapleStat::Id::LEVEL:
			player.change_level(recv.read_byte());
			break;
		case MapleStat::Id::JOB:
			player.change_job(recv.read_short());
			break;
		case MapleStat::Id::EXP:
			player.get_stats().set_exp(recv.read_int());
			break;
		case MapleStat::Id::MESO:
			player.get_inventory().set_meso(recv.read_int());
			break;
		default:
		{
			int16_t value = recv.read_short();

			// Show heal number when HP increases from regen
			if (stat == MapleStat::Id::HP)
			{
				int16_t old_hp = player.get_stats().get_stat(MapleStat::Id::HP);
				if (value > old_hp)
					player.show_heal(value - old_hp);
			}

			player.get_stats().set_stat(stat, value);
			recalculate = true;

			// Trigger death state when HP reaches 0
			if (stat == MapleStat::Id::HP && value == 0)
			{
				player.set_state(Char::State::DIED);
				Sound(Sound::Name::TOMBSTONE).play();

				static FILE* ddbg = fopen("death_debug.txt", "a");
				if (ddbg) { fprintf(ddbg, "DIED: HP=0, showing dialog\n"); fflush(ddbg); }

				UI::get().emplace<UIOk>("You have died. Press OK to return to the nearest town.", [](bool ok) {
					static FILE* ddbg2 = fopen("death_debug.txt", "a");
					if (ddbg2) { fprintf(ddbg2, "OK pressed: ok=%d, dispatching ChangeMapPacket\n", ok); fflush(ddbg2); }
					ChangeMapPacket(true, -1, "", false).dispatch();
				});
			}

			break;
		}
		}

		bool update_statsinfo = need_statsinfo_update(stat);

		if (update_statsinfo && !recalculate)
			if (auto statsinfo = UI::get().get_element<UIStatsInfo>())
				statsinfo->update_stat(stat);

		bool update_skillbook = need_skillbook_update(stat);

		if (update_skillbook)
		{
			int16_t value = player.get_stats().get_stat(stat);

			if (auto skillbook = UI::get().get_element<UISkillBook>())
				skillbook->update_stat(stat, value);
		}

		return recalculate;
	}

	bool ChangeStatsHandler::need_statsinfo_update(MapleStat::Id stat) const
	{
		switch (stat)
		{
		case MapleStat::Id::JOB:
		case MapleStat::Id::STR:
		case MapleStat::Id::DEX:
		case MapleStat::Id::INT:
		case MapleStat::Id::LUK:
		case MapleStat::Id::HP:
		case MapleStat::Id::MAXHP:
		case MapleStat::Id::MP:
		case MapleStat::Id::MAXMP:
		case MapleStat::Id::AP:
			return true;
		default:
			return false;
		}
	}

	bool ChangeStatsHandler::need_skillbook_update(MapleStat::Id stat) const
	{
		switch (stat)
		{
		case MapleStat::Id::JOB:
		case MapleStat::Id::SP:
			return true;
		default:
			return false;
		}
	}

	void BuffHandler::handle(InPacket& recv) const
	{
		uint64_t firstmask = recv.read_long();
		uint64_t secondmask = recv.read_long();

		switch (secondmask)
		{
		case Buffstat::BATTLESHIP:
			handle_buff(recv, Buffstat::BATTLESHIP);
			return;
		}

		for (auto& iter : Buffstat::first_codes)
			if (firstmask & iter.second)
				handle_buff(recv, iter.first);

		for (auto& iter : Buffstat::second_codes)
			if (secondmask & iter.second)
				handle_buff(recv, iter.first);

		Stage::get().get_player().recalc_stats(false);
	}

	void ApplyBuffHandler::handle_buff(InPacket& recv, Buffstat::Id bs) const
	{
		int16_t value = recv.read_short();
		int32_t skillid = recv.read_int();
		int32_t duration = recv.read_int();

		Stage::get().get_player().give_buff({ bs, value, skillid, duration });

		if (auto bufflist = UI::get().get_element<UIBuffList>())
			bufflist->add_buff(skillid, duration);
	}

	void CancelBuffHandler::handle_buff(InPacket&, Buffstat::Id bs) const
	{
		Stage::get().get_player().cancel_buff(bs);
	}

	void RecalculateStatsHandler::handle(InPacket&) const
	{
		Stage::get().get_player().recalc_stats(false);
	}

	void UpdateSkillHandler::handle(InPacket& recv) const
	{
		recv.read_byte(); // enableActions
		int16_t count = recv.read_short();

		for (int16_t i = 0; i < count; i++)
		{
			int32_t skillid = recv.read_int();
			int32_t level = recv.read_int();
			int32_t masterlevel = recv.read_int();
			int64_t expire = recv.read_long();

			Stage::get().get_player().change_skill(skillid, level, masterlevel, expire);

			if (auto skillbook = UI::get().get_element<UISkillBook>())
				skillbook->update_skills(skillid);
		}

		if (recv.available())
			recv.read_byte(); // trailing byte (Cosmic sends writeByte(4))

		UI::get().enable();
	}

	void SkillMacrosHandler::handle(InPacket& recv) const
	{
		uint8_t size = recv.read_byte();

		auto skillbook = UI::get().get_element<UISkillBook>();

		for (uint8_t i = 0; i < size; i++)
		{
			std::string name = recv.read_string();
			bool shout = recv.read_byte() != 0;
			int32_t skill1 = recv.read_int();
			int32_t skill2 = recv.read_int();
			int32_t skill3 = recv.read_int();

			if (skillbook)
				skillbook->load_macro(i, name, shout, skill1, skill2, skill3);
		}
	}

	void AddCooldownHandler::handle(InPacket& recv) const
	{
		int32_t skill_id = recv.read_int();
		int16_t cooltime = recv.read_short();

		Stage::get().get_player().add_cooldown(skill_id, cooltime);
	}

	void KeymapHandler::handle(InPacket& recv) const
	{
		recv.skip(1);

		for (uint8_t i = 0; i < 90; i++)
		{
			uint8_t type = recv.read_byte();
			int32_t action = recv.read_int();

			UI::get().add_keymapping(i, type, action);
		}
	}

	void MonsterBookCardHandler::handle(InPacket& recv) const
	{
		int8_t full = recv.read_byte();      // 0 = book full, 1 = not full
		int32_t cardid = recv.read_int();    // full card item id (238xxxx)
		int32_t level = recv.read_int();     // card level (1-5)
		int8_t card_level = static_cast<int8_t>(level);

		Stage::get().get_player().get_monsterbook().add_card(cardid, card_level);

		// Play card acquisition effect
		Stage::get().get_player().show_effect_id(CharEffect::Id::MONSTER_CARD);

		if (auto monsterbook = UI::get().get_element<UIMonsterBook>())
			monsterbook->update_card(cardid, card_level);
	}

	void MonsterBookCoverHandler::handle(InPacket& recv) const
	{
		int32_t cardid = recv.read_int();

		Stage::get().get_player().get_monsterbook().set_cover(cardid);
	}
}