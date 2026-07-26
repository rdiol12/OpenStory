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
#include "CharAuras.h"

#include "AiSkin.h"
#include "CharLook.h"

#include "../../Data/EquipData.h"

#include "../../Graphics/EffectLayer.h"
#include "../../Graphics/GraphicsGL.h"

#include <algorithm>
#include <cmath>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
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

	void CharAuras::set_item_effect(int32_t itemid)
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

	void CharAuras::show_item_use_effect(int32_t itemid, EffectLayer& effects) const
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

	void CharAuras::refresh(const CharEquips& equips)
	{
#ifdef USE_NX
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

	void CharAuras::update(double hspeed, double vspeed)
	{
		item_aura.update();
		gm_effect.update();

		for (auto& a : equip_auras)
			a.aura.update();

		// Motion drag: auras trail against the velocity and ease back when
		// stopping — flames stream behind a runner, settle on a stander
		float drag_target_x = std::clamp(static_cast<float>(-hspeed) * 2.0f, -8.0f, 8.0f);
		float drag_target_y = std::clamp(static_cast<float>(-vspeed) * 1.2f, -6.0f, 6.0f);
		drag_x += (drag_target_x - drag_x) * 0.12f;
		drag_y += (drag_target_y - drag_y) * 0.12f;
	}

	bool CharAuras::visible(const AuraInstance& a, const Context& ctx) const
	{
		switch (a.show)
		{
		case 1:  return !ctx.climbing;
		case 2:  return ctx.idle;
		default: return true;
		}
	}

	DrawArgument CharAuras::args(const AuraInstance& a, const Context& ctx) const
	{
		Point<int16_t> pos = ctx.absp + aura_pivot_offset(a.pivot, ctx.prone) + Point<int16_t>(
			static_cast<int16_t>(std::round(drag_x * a.drag)),
			static_cast<int16_t>(std::round(drag_y * a.drag)));

		// Head pivot rides the per-frame head bob (delta from the stance's
		// first frame, so the calibrated base offset stays valid)
		if (a.pivot == 1 && !ctx.prone)
		{
			pos += CharLook::get_drawinfo().get_neck_position(ctx.stance, ctx.frame)
				- CharLook::get_drawinfo().get_neck_position(ctx.stance, 0);
		}

		float xscale = (a.flip && !ctx.facing_right) ? -a.scale : a.scale;

		return DrawArgument(pos, pos, Point<int16_t>(0, 0), xscale, a.scale, a.tint, 0.0f);
	}

	void CharAuras::draw_below(const Context& ctx, float alpha) const
	{
		item_aura.draw_below(DrawArgument(ctx.absp), alpha);
		gm_effect.draw_below(DrawArgument(ctx.absp), alpha);

		for (const auto& a : equip_auras)
		{
			if (!visible(a, ctx))
				continue;

			if (a.blend == 1)
				GraphicsGL::get().setblend(true);

			a.aura.draw_below(args(a, ctx), alpha);

			if (a.blend == 1)
				GraphicsGL::get().setblend(false);
		}
	}

	void CharAuras::draw_above(const Context& ctx, float alpha) const
	{
		item_aura.draw_above(DrawArgument(ctx.absp), alpha);
		gm_effect.draw_above(DrawArgument(ctx.absp), alpha);

		for (const auto& a : equip_auras)
		{
			if (!visible(a, ctx))
				continue;

			if (a.blend == 1)
				GraphicsGL::get().setblend(true);

			a.aura.draw_above(args(a, ctx), alpha);

			if (a.blend == 1)
				GraphicsGL::get().setblend(false);
		}
	}
}
