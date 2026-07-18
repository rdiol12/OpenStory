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
#include "CharEquips.h"

#include "../../Data/EquipData.h"

#include <nlnx/nx.hpp>

namespace ms
{
	CharEquips::CharEquips()
	{
		for (auto iter : clothes)
			iter.second = nullptr;
	}

	void CharEquips::draw(EquipSlot::Id slot, Stance::Id stance, Clothing::Layer layer, uint8_t frame, const DrawArgument& args) const
	{
		// Procedural weapons/hats render themselves (anchored + posed).
		if (slot == EquipSlot::Id::WEAPON && procweapon.is_valid())
		{
			procweapon.draw(stance, layer, frame, args);
			return;
		}

		if (slot == EquipSlot::Id::HAT && prochat.is_valid())
		{
			prochat.draw(stance, layer, frame, args);
			return;
		}

		if (const Clothing* cloth = clothes[slot])
			cloth->draw(stance, layer, frame, args);
	}

	void CharEquips::update()
	{
		procweapon.update();
		prochat.update();
	}

	void CharEquips::add_equip(int32_t itemid, const BodyDrawInfo& drawinfo)
	{
		if (itemid <= 0)
			return;

		auto iter = cloth_cache.find(itemid);

		if (iter == cloth_cache.end())
		{
			iter = cloth_cache.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(itemid),
				std::forward_as_tuple(itemid, drawinfo)
				).first;
		}

		const Clothing& cloth = iter->second;

		EquipSlot::Id slot = cloth.get_eqslot();
		clothes[slot] = &cloth;

		// Detect the new procedural weapon format: a weapon img with a
		// default/weapon bitmap and no authored stance groups (no stand1).
		if (slot == EquipSlot::Id::WEAPON)
		{
			nl::node src = nl::nx::character["Weapon"]["0" + std::to_string(itemid) + ".img"];

			if (src["stand1"].size() == 0 &&
				src["default"]["weapon"].data_type() == nl::node::type::bitmap)
				procweapon = ProceduralWeapon(src, itemid, drawinfo);
			else
				procweapon = ProceduralWeapon(); // authored — clear any prior procedural
		}
		else if (slot == EquipSlot::Id::HAT)
		{
			nl::node src = nl::nx::character["Cap"]["0" + std::to_string(itemid) + ".img"];

			if (src["stand1"].size() == 0 &&
				src["default"]["cap"].data_type() == nl::node::type::bitmap)
				prochat = ProceduralHat(src, drawinfo);
			else
				prochat = ProceduralHat(); // authored — clear any prior procedural

			// Head hiding (info/replaceHead) is retired — no hat ever hides the
			// head/hair/face. Hair coverage stays data-driven via vslot
			// (CpH1H5 = half cover, CpH1H5AyAs = full cover hides the hair),
			// which is how helms/masks avoid hair poking through them.
		}
	}

	void CharEquips::remove_equip(EquipSlot::Id slot)
	{
		clothes[slot] = nullptr;

		if (slot == EquipSlot::Id::WEAPON)
			procweapon = ProceduralWeapon();
		else if (slot == EquipSlot::Id::HAT)
		{
			prochat = ProceduralHat();
		}
	}

	bool CharEquips::is_visible(EquipSlot::Id slot) const
	{
		if (const Clothing* cloth = clothes[slot])
			return cloth->is_transparent() == false;
		else
			return false;
	}

	bool CharEquips::comparelayer(EquipSlot::Id slot, Stance::Id stance, Clothing::Layer layer) const
	{
		if (const Clothing* cloth = clothes[slot])
			return cloth->contains_layer(stance, layer);
		else
			return false;
	}

	bool CharEquips::has_overall() const
	{
		return get_equip(EquipSlot::Id::TOP) / 10000 == 105;
	}

	bool CharEquips::has_weapon() const
	{
		return get_weapon() != 0;
	}

	bool CharEquips::is_twohanded() const
	{
		if (const Clothing* weapon = clothes[EquipSlot::Id::WEAPON])
			return weapon->is_twohanded();
		else
			return false;
	}

	CharEquips::CapType CharEquips::getcaptype() const
	{
		if (const Clothing* cap = clothes[EquipSlot::Id::HAT])
		{
			// Parse the vslot cover codes instead of matching exact strings —
			// unknown combinations (e.g. "CpH1H2H3H5HfHsAfAyAsAeHbH4H6" on AI
			// helms) previously fell to NONE, whose draw branch renders no hat
			// at all. A/face codes = full cover (hair hidden), H1 = half cover,
			// H5 alone = headband; any equipped hat must always draw.
			const std::string& vslot = cap->get_vslot();

			if (vslot.find("As") != std::string::npos || vslot.find("Ay") != std::string::npos)
				return CharEquips::CapType::FULLCOVER;
			if (vslot.find("H1") != std::string::npos)
				return CharEquips::CapType::HALFCOVER;
			if (vslot.find("H5") != std::string::npos)
				return CharEquips::CapType::HEADBAND;

			return CharEquips::CapType::HALFCOVER;
		}
		else
		{
			return CharEquips::CapType::NONE;
		}
	}

	bool CharEquips::covers_whole_head() const
	{
		// Head hiding retired: no hat suppresses the head/hair/face anymore
		return false;
	}

	Stance::Id CharEquips::adjust_stance(Stance::Id stance) const
	{
		if (const Clothing* weapon = clothes[EquipSlot::Id::WEAPON])
		{
			switch (stance)
			{
			case Stance::Id::STAND1:
			case Stance::Id::STAND2:
				return weapon->get_stand();
			case Stance::Id::WALK1:
			case Stance::Id::WALK2:
				return weapon->get_walk();
			default:
				return stance;
			}
		}
		else
		{
			return stance;
		}
	}

	int32_t CharEquips::get_equip(EquipSlot::Id slot) const
	{
		if (const Clothing* cloth = clothes[slot])
			return cloth->get_id();
		else
			return 0;
	}

	int32_t CharEquips::get_weapon() const
	{
		return get_equip(EquipSlot::Id::WEAPON);
	}

	std::unordered_map<int32_t, Clothing> CharEquips::cloth_cache;
}