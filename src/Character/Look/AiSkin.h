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

#include "../../Graphics/Texture.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{
	// AI-skin synthesis, NX-driven: an equip .img whose info node carries an
	// aiSkin bitmap (a flat, seamless, opaque MATERIAL swatch) gets every frame
	// retextured at load time. Donor frames supply all garment form (cut, folds,
	// outline, silhouette); the material supplies surface color/texture.
	//
	// Data contract (all under the .img's info node):
	//   aiSkin              bitmap  REQUIRED. Seamless opaque material swatch.
	//   aiSkinScale         number  sprite px per material width (default 24)
	//   aiSkinShadeMid      number  donor luminance mapping to unchanged color (0.74)
	//   aiSkinShadeStrength number  0 flat .. 1 full donor shading (0.40)
	//   aiSkinGloss         number  highlight response: <1 matte, >1 glossy (1.0)
	//   aiSkinBands         int     shading quantization steps; 0/1 smooth (default 4)
	//   aiSkinTrim          int     0xRRGGBB silhouette-boundary recolor
	//   aiSkinDecal         bitmap  emblem over the material, 1x sprite scale
	//   aiSkinDecalPos      vector  body-space decal center relative to the anchor
	//   aiSkinArm           bitmap  optional second material for sleeve parts
	//                               ("leather coat, steel pauldrons")
	//   aiShellFullHead     int     1 on an AI hat = intentional closed helm
	//                               (hides head/hair/face; replaceHead is ignored
	//                               on AI hats)
	namespace AiSkin
	{
		// Whether this equip carries an aiSkin material. info = the .img's info node.
		bool available(int32_t itemid, nl::node info);

		// Build the retextured part frame (invalid Texture when not applicable)
		Texture synthesize(int32_t itemid, const std::string& stancename, uint8_t frame, const std::string& part, nl::node partnode);

		// Retexture an inventory icon bitmap with the item's material
		Texture retexture_icon(int32_t itemid, nl::node iconnode, const std::string& variant);

		// Retexture an aiShell view with the item's material (body-space relative
		// to the view's origin; rotate90 for the prone view)
		Texture retexture_shell(int32_t itemid, nl::node viewnode, const std::string& key, bool rotate90);

		// The upright view vertically squashed (anchored at the collar) — the
		// airborne stand-in when no aiShell/jump view is authored, so the hem
		// rides up over tucked legs instead of clipping or reverting to donor
		// art. Material applies when present.
		Texture squash_shell(int32_t itemid, nl::node info, nl::node viewnode, const std::string& key, float yscale);

		// The upright view rotated to lying (collar left) — the prone stand-in
		// when no aiShell/prone view is authored
		Texture prone_shell(int32_t itemid, nl::node info, nl::node viewnode, const std::string& key);

		// A shell view rotated by `rot` radians around its collar origin — used
		// to lean the shell with the body's per-frame spine angle during
		// attacks, so it follows the lunge instead of staying bolt-upright
		Texture lean_shell(int32_t itemid, nl::node info, nl::node viewnode, const std::string& key, float rot);

		// Icon rendered from the aiShell upright view (scaled to fit the icon
		// canvas), so shell items show their real shape in the inventory
		Texture icon_from_shell(int32_t itemid, nl::node info);

		// Icon rendered from any art bitmap (e.g. a procedural weapon's
		// canonical blade image) — retextured by the material when present,
		// fitted to the 32x32 icon canvas
		Texture icon_from_art(int32_t itemid, nl::node info, nl::node artnode, const std::string& variant);

		// The material's accent color (saturation-weighted, normalized to a
		// bright tint) — used to tint shared aura effects to match the armor
		// (info/effectTint=1). Returns false when the item has no material.
		bool accent_color(int32_t itemid, nl::node info, float& r, float& g, float& b);

		// Preview support (Custom/preview.txt): synthesized BGRA pixels of a
		// part frame / shell view, without touching the GL atlas. Return false
		// when no material is present or the node is not a bitmap.
		bool part_pixels(int32_t itemid, nl::node info, const std::string& stancename, const std::string& part, nl::node partnode, std::vector<uint8_t>& bgra, int16_t& width, int16_t& height);
		bool shell_pixels(int32_t itemid, nl::node info, nl::node viewnode, bool rotate90, std::vector<uint8_t>& bgra, int16_t& width, int16_t& height);
	}
}
