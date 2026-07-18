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
#include "WeaponData.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Standard MapleStory afterimage (swing-trail) name for each weapon type.
	// Used when a weapon's info/afterImage is empty (e.g. procedural weapons),
	// so trails work with no NX change. An explicit info/afterImage still wins.
	static std::string default_afterimage(Weapon::Type type)
	{
		switch (type)
		{
		case Weapon::Type::SWORD_1H:
		case Weapon::Type::DAGGER:
		case Weapon::Type::CLAW:     return "swordOL";
		case Weapon::Type::SWORD_2H: return "swordTS";
		case Weapon::Type::AXE_1H:
		case Weapon::Type::AXE_2H:   return "axe";
		case Weapon::Type::MACE_1H:
		case Weapon::Type::MACE_2H:
		case Weapon::Type::WAND:
		case Weapon::Type::STAFF:    return "mace";
		case Weapon::Type::SPEAR:    return "spear";
		case Weapon::Type::POLEARM:  return "poleArm";
		case Weapon::Type::BOW:      return "bow";
		case Weapon::Type::CROSSBOW: return "crossBow";
		case Weapon::Type::KNUCKLE:  return "knuckle";
		case Weapon::Type::GUN:      return "gun";
		default:                     return "swordOL";
		}
	}

	// Same idea as default_afterimage: incomplete custom clones often ship
	// without info/sfx — fall back to the standard attack sound for the type
	// instead of swinging silently. An explicit info/sfx still wins.
	static std::string default_sfx(Weapon::Type type)
	{
		switch (type)
		{
		case Weapon::Type::SWORD_1H:
		case Weapon::Type::SWORD_2H:  return "swordL";
		case Weapon::Type::DAGGER:    return "swordS";
		case Weapon::Type::AXE_1H:
		case Weapon::Type::AXE_2H:
		case Weapon::Type::MACE_1H:
		case Weapon::Type::MACE_2H:
		case Weapon::Type::WAND:
		case Weapon::Type::STAFF:     return "mace";
		case Weapon::Type::SPEAR:     return "spear";
		case Weapon::Type::POLEARM:   return "poleArm";
		case Weapon::Type::BOW:       return "bow";
		case Weapon::Type::CROSSBOW:  return "cBow";
		case Weapon::Type::CLAW:      return "tGlove";
		case Weapon::Type::KNUCKLE:   return "knuckle";
		case Weapon::Type::GUN:       return "gun";
		default:                      return "swordL";
		}
	}

	WeaponData::WeaponData(int32_t equipid) : equipdata(EquipData::get(equipid))
	{
		int32_t prefix = equipid / 10000;
		type = Weapon::by_value(prefix);
		twohanded = prefix == Weapon::Type::STAFF || (prefix >= Weapon::Type::SWORD_2H && prefix <= Weapon::Type::POLEARM) || prefix == Weapon::Type::CROSSBOW;

		nl::node src = nl::nx::character["Weapon"]["0" + std::to_string(equipid) + ".img"]["info"];

		attackspeed = static_cast<uint8_t>(src["attackSpeed"]);
		attack = static_cast<uint8_t>(src["attack"]);

		// Missing attackSpeed on stub clones reads as 0, which would divide by
		// zero in get_attackdelay — use the normal(6)-ish default instead
		if (attackspeed == 0)
			attackspeed = 6;

		std::string sfx = (std::string)src["sfx"];

		if (sfx.empty())
			sfx = default_sfx(type);

		nl::node soundsrc = nl::nx::sound["Weapon.img"][sfx];

		bool twosounds = soundsrc["Attack2"].data_type() == nl::node::type::audio;

		if (twosounds)
		{
			usesounds[false] = soundsrc["Attack"];
			usesounds[true] = soundsrc["Attack2"];
		}
		else
		{
			usesounds[false] = soundsrc["Attack"];
			usesounds[true] = soundsrc["Attack"];
		}

		afterimage = (std::string)src["afterImage"];

		if (afterimage.empty())
			afterimage = default_afterimage(type);
	}

	bool WeaponData::is_valid() const
	{
		return equipdata.is_valid();
	}

	WeaponData::operator bool() const
	{
		return is_valid();
	}

	bool WeaponData::is_twohanded() const
	{
		return twohanded;
	}

	uint8_t WeaponData::get_speed() const
	{
		return attackspeed;
	}

	uint8_t WeaponData::get_attack() const
	{
		return attack;
	}

	std::string WeaponData::getspeedstring() const
	{
		switch (attackspeed)
		{
		case 1:
			return "FAST (1)";
		case 2:
			return "FAST (2)";
		case 3:
			return "FAST (3)";
		case 4:
			return "FAST (4)";
		case 5:
			return "NORMAL (5)";
		case 6:
			return "NORMAL (6)";
		case 7:
			return "SLOW (7)";
		case 8:
			return "SLOW (8)";
		case 9:
			return "SLOW (9)";
		default:
			return "";
		}
	}

	uint8_t WeaponData::get_attackdelay() const
	{
		if (type == Weapon::NONE)
			return 0;
		else
			return 50 - 25 / attackspeed;
	}

	Weapon::Type WeaponData::get_type() const
	{
		return type;
	}

	Sound WeaponData::get_usesound(bool degenerate) const
	{
		return usesounds[degenerate];
	}

	const std::string& WeaponData::get_afterimage() const
	{
		return afterimage;
	}

	const EquipData& WeaponData::get_equipdata() const
	{
		return equipdata;
	}
}