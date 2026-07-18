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
#include "AiSkin.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace AiSkin
	{
		namespace
		{
			constexpr float DEFAULT_SCALE_SPAN = 24.0f;
			constexpr float DEFAULT_SHADE_MID = 0.74f;
			constexpr float DEFAULT_SHADE_STRENGTH = 0.40f;
			constexpr float DEFAULT_GLOSS = 1.0f;
			constexpr int DEFAULT_BANDS = 4;
			constexpr float SHADE_MIN = 0.58f;
			constexpr float SHADE_MAX = 1.08f;

			// Donor pixels darker than this stay near-black (the drawn outline)
			constexpr float OUTLINE_LUM = 0.10f;
			constexpr float OUTLINE_DARK = 0.35f;

			struct Material
			{
				bool valid = false;
				int width = 0;
				int height = 0;
				std::vector<uint8_t> bgra;

				float scale_span = DEFAULT_SCALE_SPAN;
				float shade_mid = DEFAULT_SHADE_MID;
				float shade_strength = DEFAULT_SHADE_STRENGTH;
				float gloss = DEFAULT_GLOSS;
				int bands = DEFAULT_BANDS;

				bool has_trim = false;
				uint8_t trim_r = 0, trim_g = 0, trim_b = 0;

				int decal_w = 0, decal_h = 0;
				std::vector<uint8_t> decal_bgra;
				Point<int16_t> decal_pos;

				// Optional second material for sleeve parts (info/aiSkinArm) ג€”
				// "leather coat, steel pauldrons" as two swatches
				std::shared_ptr<Material> arm;
			};

			std::unordered_map<int32_t, Material>& material_cache()
			{
				static std::unordered_map<int32_t, Material> cache;

				return cache;
			}

			// Contract linting: warn once per item about data that will render
			// wrong, instead of failing silently
			void lint_material(int32_t itemid, const Material& material)
			{
				if (material.width > 1024 || material.height > 1024)
					std::cout << "[AiSkin] WARNING item " << itemid << ": material is "
						<< material.width << "x" << material.height << " (expected <=1024, wasteful)" << std::endl;

				size_t transparent = 0;

				for (size_t i = 3; i < material.bgra.size(); i += 4)
					if (material.bgra[i] < 250)
						transparent++;

				if (transparent * 20 > material.bgra.size() / 4)
					std::cout << "[AiSkin] WARNING item " << itemid << ": material is not opaque ("
						<< (transparent * 100 / (material.bgra.size() / 4))
						<< "% translucent pixels) ג€” background may leak into the equip" << std::endl;
			}

			const Material& load_material(int32_t itemid, nl::node info)
			{
				auto& cache = material_cache();
				auto iter = cache.find(itemid);

				if (iter != cache.end())
					return iter->second;

				Material material;
				nl::node skinnode = info["aiSkin"];

				if (skinnode.data_type() == nl::node::type::bitmap)
				{
					nl::bitmap bmp = skinnode;
					int w = bmp.width();
					int h = bmp.height();

					if (w > 0 && h > 0)
					{
						const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
						material.valid = true;
						material.width = w;
						material.height = h;
						material.bgra.assign(data, data + size_t(w) * h * 4);

						if (double scale = info["aiSkinScale"]; scale > 0.0)
							material.scale_span = float(scale);
						if (double mid = info["aiSkinShadeMid"]; mid > 0.0)
							material.shade_mid = float(mid);
						if (double strength = info["aiSkinShadeStrength"]; strength > 0.0)
							material.shade_strength = float(strength);
						if (double gloss = info["aiSkinGloss"]; gloss > 0.0)
							material.gloss = float(gloss);
						if (info["aiSkinBands"].data_type() == nl::node::type::integer)
							material.bands = int(int64_t(info["aiSkinBands"]));

						if (info["aiSkinTrim"].data_type() == nl::node::type::integer)
						{
							int64_t rgb = info["aiSkinTrim"];
							material.has_trim = true;
							material.trim_r = uint8_t((rgb >> 16) & 0xFF);
							material.trim_g = uint8_t((rgb >> 8) & 0xFF);
							material.trim_b = uint8_t(rgb & 0xFF);
						}

						nl::node decalnode = info["aiSkinDecal"];

						if (decalnode.data_type() == nl::node::type::bitmap)
						{
							nl::bitmap decal = decalnode;

							if (decal.width() > 0 && decal.height() > 0)
							{
								const uint8_t* ddata = static_cast<const uint8_t*>(decal.data());
								material.decal_w = decal.width();
								material.decal_h = decal.height();
								material.decal_bgra.assign(ddata, ddata + size_t(material.decal_w) * material.decal_h * 4);
								material.decal_pos = info["aiSkinDecalPos"];
							}
						}

						nl::node armnode = info["aiSkinArm"];

						if (armnode.data_type() == nl::node::type::bitmap)
						{
							nl::bitmap armbmp = armnode;

							if (armbmp.width() > 0 && armbmp.height() > 0)
							{
								// Same knobs as the main material, different swatch
								auto arm = std::make_shared<Material>(material);
								const uint8_t* adata = static_cast<const uint8_t*>(armbmp.data());
								arm->width = armbmp.width();
								arm->height = armbmp.height();
								arm->bgra.assign(adata, adata + size_t(arm->width) * arm->height * 4);
								arm->arm = nullptr;
								material.arm = std::move(arm);
							}
						}

						lint_material(itemid, material);
					}
				}

				return cache.emplace(itemid, std::move(material)).first->second;
			}

			// Area-averaged wrap-around sample: one sprite pixel covers about
			// scale x scale material texels; averaging over that footprint
			// avoids single-texel speckle on high-resolution materials.
			void sample_area(const Material& material, float cx, float cy, float radius, float& r, float& g, float& b)
			{
				int steps = std::clamp(int(radius), 1, 4);
				float sr = 0.0f, sg = 0.0f, sb = 0.0f;
				int count = 0;

				for (int iy = 0; iy < steps; ++iy)
				{
					for (int ix = 0; ix < steps; ++ix)
					{
						float ox = steps > 1 ? -radius + (2.0f * radius * ix) / (steps - 1) : 0.0f;
						float oy = steps > 1 ? -radius + (2.0f * radius * iy) / (steps - 1) : 0.0f;

						int x = int(std::floor(cx + ox)) % material.width;
						int y = int(std::floor(cy + oy)) % material.height;

						if (x < 0)
							x += material.width;
						if (y < 0)
							y += material.height;

						const uint8_t* p = &material.bgra[(size_t(y) * material.width + x) * 4];
						sb += p[0];
						sg += p[1];
						sr += p[2];
						count++;
					}
				}

				r = sr / count;
				g = sg / count;
				b = sb / count;
			}

			float shade_for(const Material& material, float lum)
			{
				float shade = 1.0f + (lum / material.shade_mid - 1.0f) * material.shade_strength;

				// Gloss reshapes the highlight response only
				if (shade > 1.0f)
					shade = 1.0f + (shade - 1.0f) * material.gloss;

				shade = std::clamp(shade, SHADE_MIN, SHADE_MAX);

				// Cel-style banding for a hand-drawn read
				if (material.bands > 1)
				{
					float t = (shade - SHADE_MIN) / (SHADE_MAX - SHADE_MIN);
					t = std::round(t * (material.bands - 1)) / (material.bands - 1);
					shade = SHADE_MIN + t * (SHADE_MAX - SHADE_MIN);
				}

				// Outline pixels stay dark (after banding, smooth dark ramp)
				if (lum < OUTLINE_LUM)
					shade *= OUTLINE_DARK + (1.0f - OUTLINE_DARK) * (lum / OUTLINE_LUM);

				return shade;
			}

			// Core retexture of one donor bitmap into a BGRA buffer
			bool build_pixels(const Material& material, nl::node bitmapnode,
				bool center_anchor, Point<int16_t> anchor, bool rotate90, bool allow_decal,
				std::vector<uint8_t>& out, int16_t& w, int16_t& h)
			{
				if (bitmapnode.data_type() != nl::node::type::bitmap)
					return false;

				nl::bitmap bmp = bitmapnode;
				w = bmp.width();
				h = bmp.height();

				if (w <= 0 || h <= 0)
					return false;

				const uint8_t* donor = static_cast<const uint8_t*>(bmp.data());
				float scale = material.width / material.scale_span;

				out.assign(size_t(w) * h * 4, 0);

				for (int16_t y = 0; y < h; ++y)
				{
					for (int16_t x = 0; x < w; ++x)
					{
						size_t i = (size_t(y) * w + x) * 4;
						uint8_t alpha = donor[i + 3];

						if (alpha == 0)
							continue;

						float bx, by;

						if (center_anchor)
						{
							bx = x - w * 0.5f;
							by = y - h * 0.5f;
						}
						else
						{
							bx = float(x - anchor.x());
							by = float(y - anchor.y());
						}

						if (rotate90)
						{
							float tmp = bx;
							bx = -by;
							by = tmp;
						}

						float r, g, b;
						sample_area(material, bx * scale, by * scale, scale * 0.5f, r, g, b);

						// Emblem decal over the material (body-space pinned)
						if (allow_decal && material.decal_w > 0)
						{
							int dx = int(std::floor(bx - material.decal_pos.x() + material.decal_w * 0.5f));
							int dy = int(std::floor(by - material.decal_pos.y() + material.decal_h * 0.5f));

							if (dx >= 0 && dy >= 0 && dx < material.decal_w && dy < material.decal_h)
							{
								const uint8_t* dp = &material.decal_bgra[(size_t(dy) * material.decal_w + dx) * 4];

								if (dp[3] >= 128)
								{
									b = dp[0];
									g = dp[1];
									r = dp[2];
								}
							}
						}

						float lum = (0.299f * donor[i + 2] + 0.587f * donor[i + 1] + 0.114f * donor[i]) / 255.0f;
						float shade = shade_for(material, lum);

						out[i] = uint8_t(std::min(b * shade, 255.0f));
						out[i + 1] = uint8_t(std::min(g * shade, 255.0f));
						out[i + 2] = uint8_t(std::min(r * shade, 255.0f));
						out[i + 3] = alpha;
					}
				}

				// Silhouette trim: recolor pixels on the alpha boundary
				if (material.has_trim && allow_decal)
				{
					auto donor_alpha = [&](int x, int y) -> uint8_t
					{
						if (x < 0 || y < 0 || x >= w || y >= h)
							return 0;

						return donor[(size_t(y) * w + x) * 4 + 3];
					};

					for (int16_t y = 0; y < h; ++y)
					{
						for (int16_t x = 0; x < w; ++x)
						{
							size_t i = (size_t(y) * w + x) * 4;

							if (donor[i + 3] == 0)
								continue;

							bool boundary =
								donor_alpha(x - 1, y) == 0 || donor_alpha(x + 1, y) == 0 ||
								donor_alpha(x, y - 1) == 0 || donor_alpha(x, y + 1) == 0;

							if (!boundary)
								continue;

							float lum = (0.299f * donor[i + 2] + 0.587f * donor[i + 1] + 0.114f * donor[i]) / 255.0f;
							float shade = shade_for(material, lum);

							out[i] = uint8_t(std::min(material.trim_b * shade, 255.0f));
							out[i + 1] = uint8_t(std::min(material.trim_g * shade, 255.0f));
							out[i + 2] = uint8_t(std::min(material.trim_r * shade, 255.0f));
						}
					}
				}

				return true;
			}

			Texture build(const Material& material, nl::node bitmapnode, const std::string& key,
				bool center_anchor, Point<int16_t> anchor, bool rotate90, bool allow_decal, Point<int16_t> origin)
			{
				std::vector<uint8_t> out;
				int16_t w = 0, h = 0;

				if (!build_pixels(material, bitmapnode, center_anchor, anchor, rotate90, allow_decal, out, w, h))
					return Texture();

				return Texture::from_pixels(key, w, h, std::move(out), origin);
			}

			// Part anchor resolution shared by synthesize and part_pixels
			void part_setup(const std::string& stancename, const std::string& part, nl::node partnode,
				bool& is_arm, bool& is_prone, Point<int16_t>& anchor, Point<int16_t>& origin)
			{
				origin = partnode["origin"];

				Point<int16_t> mappoint;

				for (nl::node mapnode : partnode["map"])
					if (mapnode.data_type() == nl::node::type::vector)
						mappoint = mapnode;

				anchor = origin + mappoint;
				is_arm = part.find("Arm") != std::string::npos;
				is_prone = stancename.rfind("prone", 0) == 0;
			}
		}

		bool available(int32_t itemid, nl::node info)
		{
			return load_material(itemid, info).valid;
		}

		Texture synthesize(int32_t itemid, const std::string& stancename, uint8_t frame, const std::string& part, nl::node partnode)
		{
			auto iter = material_cache().find(itemid);

			if (iter == material_cache().end() || !iter->second.valid)
				return Texture();

			bool is_arm, is_prone;
			Point<int16_t> anchor, origin;
			part_setup(stancename, part, partnode, is_arm, is_prone, anchor, origin);

			std::string key = "aiskin/" + std::to_string(itemid) + "/" + stancename + "." + std::to_string(frame) + "." + part;

			const Material& material = (is_arm && iter->second.arm) ? *iter->second.arm : iter->second;

			return build(material, partnode, key, is_arm, anchor, is_prone, !is_arm, origin);
		}

		Texture retexture_shell(int32_t itemid, nl::node viewnode, const std::string& key, bool rotate90)
		{
			auto iter = material_cache().find(itemid);

			if (iter == material_cache().end() || !iter->second.valid)
				return Texture();

			if (viewnode.data_type() != nl::node::type::bitmap)
				return Texture();

			Point<int16_t> origin = viewnode["origin"];

			return build(iter->second, viewnode, key, false, origin, rotate90, true, origin);
		}

		Texture squash_shell(int32_t itemid, nl::node info, nl::node viewnode, const std::string& key, float yscale)
		{
			if (viewnode.data_type() != nl::node::type::bitmap || yscale <= 0.0f || yscale > 1.0f)
				return Texture();

			// Retextured when a material exists, raw otherwise
			std::vector<uint8_t> px;
			int16_t w = 0, h = 0;

			if (!shell_pixels(itemid, info, viewnode, false, px, w, h))
				return Texture();

			int16_t out_h = std::max<int16_t>(1, int16_t(h * yscale));
			std::vector<uint8_t> out(size_t(w) * out_h * 4);

			for (int16_t y = 0; y < out_h; ++y)
			{
				int src_y = std::min<int>(h - 1, int(y / yscale));
				std::copy_n(&px[size_t(src_y) * w * 4], size_t(w) * 4, &out[size_t(y) * w * 4]);
			}

			Point<int16_t> origin = viewnode["origin"];
			Point<int16_t> squashed_origin(origin.x(), int16_t(origin.y() * yscale));

			return Texture::from_pixels(key, w, out_h, std::move(out), squashed_origin);
		}

		Texture prone_shell(int32_t itemid, nl::node info, nl::node viewnode, const std::string& key)
		{
			if (viewnode.data_type() != nl::node::type::bitmap)
				return Texture();

			std::vector<uint8_t> px;
			int16_t w = 0, h = 0;

			if (!shell_pixels(itemid, info, viewnode, false, px, w, h))
				return Texture();

			// Rotate CCW so the collar ends up on the LEFT (lying, head left ג€”
			// the prone authoring convention)
			int16_t out_w = h;
			int16_t out_h = w;
			std::vector<uint8_t> out(size_t(out_w) * out_h * 4);

			for (int16_t y = 0; y < out_h; ++y)
			{
				for (int16_t x = 0; x < out_w; ++x)
				{
					int src_x = (w - 1) - y;
					int src_y = x;
					std::copy_n(&px[(size_t(src_y) * w + src_x) * 4], 4, &out[(size_t(y) * out_w + x) * 4]);
				}
			}

			Point<int16_t> origin = viewnode["origin"];
			Point<int16_t> rotated_origin(origin.y(), int16_t((w - 1) - origin.x()));

			return Texture::from_pixels(key, out_w, out_h, std::move(out), rotated_origin);
		}

		Texture retexture_icon(int32_t itemid, nl::node iconnode, const std::string& variant)
		{
			auto iter = material_cache().find(itemid);

			if (iter == material_cache().end() || !iter->second.valid)
				return Texture();

			if (iconnode.data_type() != nl::node::type::bitmap)
				return Texture();

			std::string key = "aiskin/" + std::to_string(itemid) + "/icon." + variant;

			return build(iter->second, iconnode, key, true, Point<int16_t>(), false, false, iconnode["origin"]);
		}

		Texture icon_from_shell(int32_t itemid, nl::node info)
		{
			nl::node view = info["aiShell"]["upright"];

			if (view && view.data_type() != nl::node::type::bitmap)
				view = view["0"];

			if (view.data_type() != nl::node::type::bitmap)
				return Texture();

			// Shell pixels (retextured when a material exists, raw otherwise)
			std::vector<uint8_t> px;
			int16_t w = 0, h = 0;

			if (!shell_pixels(itemid, info, view, false, px, w, h))
				return Texture();

			// Fit into the standard 32x32 icon canvas (nearest, centered,
			// never upscaled)
			constexpr int16_t ICON = 32;
			int16_t fit_w = w, fit_h = h;

			if (w > ICON || h > ICON)
			{
				float f = std::min(float(ICON) / w, float(ICON) / h);
				fit_w = std::max<int16_t>(1, int16_t(w * f));
				fit_h = std::max<int16_t>(1, int16_t(h * f));
			}

			std::vector<uint8_t> icon(size_t(ICON) * ICON * 4, 0);
			int16_t off_x = (ICON - fit_w) / 2;
			int16_t off_y = (ICON - fit_h) / 2;

			for (int16_t y = 0; y < fit_h; ++y)
			{
				for (int16_t x = 0; x < fit_w; ++x)
				{
					int sx = std::min<int>(w - 1, x * w / fit_w);
					int sy = std::min<int>(h - 1, y * h / fit_h);
					size_t src_i = (size_t(sy) * w + sx) * 4;
					size_t dst_i = (size_t(y + off_y) * ICON + (x + off_x)) * 4;

					icon[dst_i] = px[src_i];
					icon[dst_i + 1] = px[src_i + 1];
					icon[dst_i + 2] = px[src_i + 2];
					icon[dst_i + 3] = px[src_i + 3];
				}
			}

			// Reuse the donor icon's origin so the icon draws like any other
			Point<int16_t> origin = info["icon"]["origin"];
			std::string key = "aiskin/" + std::to_string(itemid) + "/icon.shell";

			return Texture::from_pixels(key, ICON, ICON, std::move(icon), origin);
		}

		bool accent_color(int32_t itemid, nl::node info, float& r, float& g, float& b)
		{
			const Material& material = load_material(itemid, info);

			if (!material.valid)
				return false;

			// Saturation*value weighted average: picks out the material's
			// characteristic hue (crimson veins in dark scales) instead of the
			// murky overall average
			double sr = 0.0, sg = 0.0, sb = 0.0, sw = 0.0;

			for (size_t i = 0; i + 3 < material.bgra.size(); i += 4)
			{
				float pb = material.bgra[i] / 255.0f;
				float pg = material.bgra[i + 1] / 255.0f;
				float pr = material.bgra[i + 2] / 255.0f;

				float mx = std::max({ pr, pg, pb });
				float mn = std::min({ pr, pg, pb });
				float weight = (mx - mn) * mx + 0.01f;

				sr += pr * weight;
				sg += pg * weight;
				sb += pb * weight;
				sw += weight;
			}

			if (sw <= 0.0)
				return false;

			r = float(sr / sw);
			g = float(sg / sw);
			b = float(sb / sw);

			// Normalize to a bright tint (tinting multiplies the effect art,
			// so a dark tint would kill the aura)
			float mx = std::max({ r, g, b, 0.01f });
			r = std::min(r / mx, 1.0f);
			g = std::min(g / mx, 1.0f);
			b = std::min(b / mx, 1.0f);

			return true;
		}

		bool part_pixels(int32_t itemid, nl::node info, const std::string& stancename, const std::string& part, nl::node partnode, std::vector<uint8_t>& bgra, int16_t& width, int16_t& height)
		{
			const Material& material = load_material(itemid, info);

			if (!material.valid)
				return false;

			bool is_arm, is_prone;
			Point<int16_t> anchor, origin;
			part_setup(stancename, part, partnode, is_arm, is_prone, anchor, origin);

			const Material& chosen = (is_arm && material.arm) ? *material.arm : material;

			return build_pixels(chosen, partnode, is_arm, anchor, is_prone, !is_arm, bgra, width, height);
		}

		bool shell_pixels(int32_t itemid, nl::node info, nl::node viewnode, bool rotate90, std::vector<uint8_t>& bgra, int16_t& width, int16_t& height)
		{
			if (viewnode.data_type() != nl::node::type::bitmap)
				return false;

			const Material& material = load_material(itemid, info);

			if (material.valid)
			{
				Point<int16_t> origin = viewnode["origin"];

				return build_pixels(material, viewnode, false, origin, rotate90, true, bgra, width, height);
			}

			// No material: raw copy of the authored view
			nl::bitmap bmp = viewnode;
			width = bmp.width();
			height = bmp.height();

			if (width <= 0 || height <= 0)
				return false;

			const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
			bgra.assign(data, data + size_t(width) * height * 4);

			return true;
		}
	}
}
