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
#pragma once

#include "Stance.h"

#include "../../Graphics/CharacterAura.h"
#include "../../Graphics/Color.h"

#include <vector>

namespace ms
{
	class CharEquips;
	class EffectLayer;

	// All equip-driven auras on a character: the item aura (effect ring / cash
	// effect), the GM set effect and the data-driven info/effect equip auras.
	class CharAuras
	{
	public:
		// Per-frame facts of the wearer the aura passes depend on.
		struct Context
		{
			Point<int16_t> absp;
			bool prone;
			bool climbing;
			bool idle;
			bool facing_right;
			Stance::Id stance;
			uint8_t frame;
		};

		// Start/stop a persistent looping item-use aura (Effect.wz/ItemEff.img).
		// Pass itemid=0 to clear.
		void set_item_effect(int32_t itemid);
		// The currently active looping item aura (0 if none).
		int32_t get_item_effect_id() const { return item_effect_id; }
		// Play an item's ItemEff animation once (consumable use puff) on the
		// given effect layer, then let it expire. Unlike set_item_effect this
		// does not loop.
		void show_item_use_effect(int32_t itemid, EffectLayer& effects) const;
		// Refresh equip-driven effects: effect-ring auras, the GM-hat set
		// effect and the declared info/effect auras. Call on equip change.
		void refresh(const CharEquips& equips);

		void update(double hspeed, double vspeed);
		// Aura layers that sit behind / in front of the character. A flat
		// (unsplit) aura draws its whole self behind so it frames the body; a
		// split aura draws its 0-layer behind and its 1-layer in front.
		void draw_below(const Context& ctx, float alpha) const;
		void draw_above(const Context& ctx, float alpha) const;

	private:
		// Data-driven auras: any equipped item that declares info/effect adds one
		// here (name -> CharEff.img, "Folder/name" escape, or inline subtree).
		// ADDITIVE to item_aura/gm_effect (which stay untouched for network item
		// effects and the GM hat, so those can't regress). Rebuilt on equip
		// change, capped to AURA_CAP, drawn at the per-item pivot offset.
		struct AuraInstance
		{
			CharacterAura aura;
			int16_t pivot = 0;   // 0 center | 1 head | 2 feet
			int16_t blend = 0;   // 0 normal | 1 additive (rendered via setblend)
			int16_t prio = 0;    // higher wins the cap
			int16_t show = 0;    // 0 always | 1 hide climbing | 2 idle only
			float scale = 1.0f;  // effectScale: template drawn at this size
			float drag = 1.0f;   // effectDrag: motion-drag multiplier (0 = pinned)
			bool flip = false;   // effectFlip: mirror with the character's facing
			// Tint color * effectOpacity alpha. effectTintColor sets it
			// explicitly; effectTint=1 samples the aiSkin material accent, so
			// one shared white/gray template matches every armor theme.
			Color tint = Color(1.0f, 1.0f, 1.0f, 1.0f);
		};

		bool visible(const AuraInstance& a, const Context& ctx) const;
		DrawArgument args(const AuraInstance& a, const Context& ctx) const;

		// Persistent item aura (effect ring / cash effect). Active while
		// item_effect_id != 0.
		CharacterAura item_aura;
		int32_t item_effect_id = 0;

		// GM set effect (Effect.img/SetEff.img/37) shown on GM characters.
		CharacterAura gm_effect;

		std::vector<AuraInstance> equip_auras;

		// Eased offset opposing the character's velocity — auras trail when
		// moving and settle when standing (see update)
		float drag_x = 0.0f;
		float drag_y = 0.0f;
	};
}
