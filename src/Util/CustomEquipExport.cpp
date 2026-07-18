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
#include "CustomEquipExport.h"

#include "../Character/Look/AiSkin.h"
#include "../Character/Look/Stance.h"
#include "../Data/EquipData.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stb_image_write.h>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	namespace CustomEquipExport
	{
		namespace
		{
			// Dump one bitmap node as RGBA png
			bool write_png(const std::string& path, nl::bitmap bmp)
			{
				int width = bmp.width();
				int height = bmp.height();

				if (width <= 0 || height <= 0)
					return false;

				const uint8_t* bgra = static_cast<const uint8_t*>(bmp.data());
				std::vector<uint8_t> rgba(bgra, bgra + width * height * 4);

				for (size_t i = 0; i < rgba.size(); i += 4)
					std::swap(rgba[i], rgba[i + 2]);

				return stbi_write_png(path.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
			}

			void export_equip(int32_t itemid)
			{
				std::string strid = "0" + std::to_string(itemid);
				std::string category = EquipData::get(itemid).get_itemdata().get_category();
				nl::node src = nl::nx::character[category][strid + ".img"];

				if (!src)
				{
					std::cout << "[CustomEquip] Export: no .img found for item " << itemid << std::endl;

					return;
				}

				std::string outdir = "Custom/Export/" + std::to_string(itemid);
				std::filesystem::create_directories(outdir);

				std::ofstream meta(outdir + "/frames.txt");
				meta << "# item " << itemid << " (" << category << "/" << strid << ".img)\n";
				meta << "# file  size  origin  anchors  z\n";
				int exported = 0;

				for (auto iter : Stance::names)
				{
					const std::string& stancename = iter.second;
					nl::node stancenode = src[stancename];

					if (!stancenode)
						continue;

					for (uint8_t frame = 0; nl::node framenode = stancenode[frame]; ++frame)
					{
						for (nl::node partnode : framenode)
						{
							if (partnode.data_type() != nl::node::type::bitmap)
								continue;

							std::string name = stancename + "." + std::to_string(frame) + "." + partnode.name() + ".png";

							if (!write_png(outdir + "/" + name, partnode))
								continue;

							nl::bitmap bmp = partnode;
							nl::node origin = partnode["origin"];

							meta << name
								<< "  " << bmp.width() << "x" << bmp.height()
								<< "  origin=(" << origin.x() << "," << origin.y() << ")";

							for (auto mapnode : partnode["map"])
								if (mapnode.data_type() == nl::node::type::vector)
									meta << "  " << mapnode.name() << "=(" << mapnode.x() << "," << mapnode.y() << ")";

							meta << "  z=" << (std::string)partnode["z"] << "\n";
							exported++;
						}
					}
				}

				std::cout << "[CustomEquip] Exported " << exported << " frames for item " << itemid << " to " << outdir << std::endl;
			}

			// Write a raw BGRA buffer as an RGBA png
			bool write_pixels(const std::string& path, const std::vector<uint8_t>& bgra, int16_t width, int16_t height)
			{
				if (width <= 0 || height <= 0 || bgra.size() != size_t(width) * height * 4)
					return false;

				std::vector<uint8_t> rgba(bgra);

				for (size_t i = 0; i < rgba.size(); i += 4)
					std::swap(rgba[i], rgba[i + 2]);

				return stbi_write_png(path.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
			}

			// The character's head sprite + its position relative to the brow
			// anchor — the mannequin for hat-fit composites
			struct HeadRef
			{
				bool valid = false;
				int16_t width = 0;
				int16_t height = 0;
				std::vector<uint8_t> bgra;
				Point<int16_t> tl_rel_brow;
			};

			HeadRef load_head(const char* stancename)
			{
				HeadRef ref;
				nl::node head = nl::nx::character["00012000.img"][stancename]["0"]["head"];

				if (head.data_type() != nl::node::type::bitmap || !head["map"]["brow"])
					return ref;

				nl::bitmap bmp = head;

				if (bmp.width() <= 0 || bmp.height() <= 0)
					return ref;

				Point<int16_t> origin = head["origin"];
				Point<int16_t> brow = head["map"]["brow"];

				const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
				ref.width = bmp.width();
				ref.height = bmp.height();
				ref.bgra.assign(data, data + size_t(ref.width) * ref.height * 4);
				ref.tl_rel_brow = Point<int16_t>(
					static_cast<int16_t>(-(origin.x() + brow.x())),
					static_cast<int16_t>(-(origin.y() + brow.y())));
				ref.valid = true;

				return ref;
			}

			// A default hair (id 30000) piece set for the composite — the
			// in-game head is haired, and hats that ignore hair volume look
			// sunken/small in game while fitting a bald composite perfectly
			struct HairPiece
			{
				int16_t width = 0;
				int16_t height = 0;
				std::vector<uint8_t> bgra;
				Point<int16_t> tl_rel_brow;
			};

			std::vector<HairPiece> load_hair()
			{
				std::vector<HairPiece> pieces;
				nl::node d = nl::nx::character["Hair"]["00030000.img"]["default"];

				for (nl::node piecenode : d)
				{
					if (piecenode.data_type() != nl::node::type::bitmap || !piecenode["map"]["brow"])
						continue;

					if (piecenode.name() == "hairShade")
						continue;

					nl::bitmap bmp = piecenode;

					if (bmp.width() <= 0 || bmp.height() <= 0)
						continue;

					Point<int16_t> origin = piecenode["origin"];
					Point<int16_t> brow = piecenode["map"]["brow"];

					HairPiece piece;
					const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
					piece.width = bmp.width();
					piece.height = bmp.height();
					piece.bgra.assign(data, data + size_t(piece.width) * piece.height * 4);
					piece.tl_rel_brow = Point<int16_t>(
						static_cast<int16_t>(-(origin.x() + brow.x())),
						static_cast<int16_t>(-(origin.y() + brow.y())));
					pieces.push_back(std::move(piece));
				}

				return pieces;
			}

			// Alpha-over blit into a BGRA canvas
			void blit(std::vector<uint8_t>& dst, int dst_w, int dst_h, const std::vector<uint8_t>& src, int src_w, int src_h, int off_x, int off_y)
			{
				for (int y = 0; y < src_h; ++y)
				{
					int dy = y + off_y;

					if (dy < 0 || dy >= dst_h)
						continue;

					for (int x = 0; x < src_w; ++x)
					{
						int dx = x + off_x;

						if (dx < 0 || dx >= dst_w)
							continue;

						const uint8_t* s = &src[(size_t(y) * src_w + x) * 4];
						uint8_t sa = s[3];

						if (sa == 0)
							continue;

						uint8_t* d = &dst[(size_t(dy) * dst_w + dx) * 4];

						for (int ch = 0; ch < 3; ++ch)
							d[ch] = uint8_t((s[ch] * sa + d[ch] * (255 - sa)) / 255);

						d[3] = uint8_t(sa + d[3] * (255 - sa) / 255);
					}
				}
			}

			// Composite a hat view over the head at exactly the offset the
			// client will use in game (view origin pinned to the brow), then
			// write it 4x upscaled — the fit is judged BEFORE publishing
			void write_onhead(const std::string& path, const HeadRef& head, const std::vector<HairPiece>& hair, const std::vector<uint8_t>& hat, int16_t hat_w, int16_t hat_h, Point<int16_t> hat_origin)
			{
				int hat_x = -hat_origin.x();
				int hat_y = -hat_origin.y();

				int left = std::min<int>(head.tl_rel_brow.x(), hat_x) - 2;
				int top = std::min<int>(head.tl_rel_brow.y(), hat_y) - 2;
				int right = std::max<int>(head.tl_rel_brow.x() + head.width, hat_x + hat_w) + 2;
				int bottom = std::max<int>(head.tl_rel_brow.y() + head.height, hat_y + hat_h) + 2;

				for (const auto& piece : hair)
				{
					left = std::min<int>(left, piece.tl_rel_brow.x() - 2);
					top = std::min<int>(top, piece.tl_rel_brow.y() - 2);
					right = std::max<int>(right, piece.tl_rel_brow.x() + piece.width + 2);
					bottom = std::max<int>(bottom, piece.tl_rel_brow.y() + piece.height + 2);
				}

				int canvas_w = right - left;
				int canvas_h = bottom - top;

				if (canvas_w <= 0 || canvas_h <= 0 || canvas_w > 512 || canvas_h > 512)
					return;

				std::vector<uint8_t> canvas(size_t(canvas_w) * canvas_h * 4, 0);
				blit(canvas, canvas_w, canvas_h, head.bgra, head.width, head.height, head.tl_rel_brow.x() - left, head.tl_rel_brow.y() - top);

				for (const auto& piece : hair)
					blit(canvas, canvas_w, canvas_h, piece.bgra, piece.width, piece.height, piece.tl_rel_brow.x() - left, piece.tl_rel_brow.y() - top);

				blit(canvas, canvas_w, canvas_h, hat, hat_w, hat_h, hat_x - left, hat_y - top);

				// 4x nearest for readability
				int out_w = canvas_w * 4;
				int out_h = canvas_h * 4;
				std::vector<uint8_t> big(size_t(out_w) * out_h * 4);

				for (int y = 0; y < out_h; ++y)
					for (int x = 0; x < out_w; ++x)
						std::copy_n(&canvas[(size_t(y / 4) * canvas_w + x / 4) * 4], 4, &big[(size_t(y) * out_w + x) * 4]);

				write_pixels(path, big, int16_t(out_w), int16_t(out_h));
			}

			// Dump how an AI item's frames will actually look after synthesis —
			// review a new item without ever equipping it in game
			void preview_equip(int32_t itemid)
			{
				std::string strid = "0" + std::to_string(itemid);
				std::string category = EquipData::get(itemid).get_itemdata().get_category();
				nl::node src = nl::nx::character[category][strid + ".img"];

				if (!src)
				{
					std::cout << "[CustomEquip] Preview: no .img found for item " << itemid << std::endl;

					return;
				}

				nl::node info = src["info"];
				std::string outdir = "Custom/Preview/" + std::to_string(itemid);
				std::filesystem::create_directories(outdir);

				int written = 0;
				std::vector<uint8_t> px;
				int16_t w = 0, h = 0;

				// Synthesized donor part frames (material items)
				if (AiSkin::available(itemid, info))
				{
					for (auto iter : Stance::names)
					{
						const std::string& stancename = iter.second;
						nl::node stancenode = src[stancename];

						if (!stancenode)
							continue;

						for (uint8_t frame = 0; nl::node framenode = stancenode[frame]; ++frame)
							for (nl::node partnode : framenode)
								if (partnode.data_type() == nl::node::type::bitmap &&
									AiSkin::part_pixels(itemid, info, stancename, partnode.name(), partnode, px, w, h))
								{
									std::string name = stancename + "." + std::to_string(frame) + "." + partnode.name() + ".png";

									if (write_pixels(outdir + "/" + name, px, w, h))
										written++;
								}
					}
				}

				// Shell views (retextured when a material exists, raw otherwise)
				nl::node shell = info["aiShell"];

				for (const char* family : { "upright", "prone", "back", "sit", "attack" })
				{
					nl::node view = shell[family];

					if (!view)
						continue;

					bool rotate = std::string(family) == "prone";

					if (view.data_type() == nl::node::type::bitmap)
					{
						if (AiSkin::shell_pixels(itemid, info, view, rotate, px, w, h) &&
							write_pixels(outdir + "/shell." + family + ".png", px, w, h))
							written++;
					}
					else
					{
						for (int sub = 0; view[std::to_string(sub)].data_type() == nl::node::type::bitmap; ++sub)
							if (AiSkin::shell_pixels(itemid, info, view[std::to_string(sub)], rotate, px, w, h) &&
								write_pixels(outdir + "/shell." + family + "." + std::to_string(sub) + ".png", px, w, h))
								written++;
					}
				}

				// Hats: composite over the real head sprite at the exact
				// in-game offset — the "does it fit" answer without equipping
				// anything. Uses the same fit-class path the game renders with.
				if (category == "Cap")
				{
					HeadRef front_head = load_head("stand1");
					HeadRef back_head = load_head("ladder");

					// Hair in the composite mirrors the in-game cover type:
					// full-cover vslots (Ay/As) hide it, everything else shows it
					std::string vslot = (std::string)info["vslot"];
					bool fullcover = vslot.find("As") != std::string::npos || vslot.find("Ay") != std::string::npos;
					std::vector<HairPiece> hair = fullcover ? std::vector<HairPiece>() : load_hair();

					for (const char* family : { "upright", "back" })
					{
						bool is_back = std::string(family) == "back";

						// Art source: shell view, else the donor-canvas part
						nl::node view = shell[family];

						if (view && view.data_type() != nl::node::type::bitmap)
							view = view["0"];

						if (view.data_type() != nl::node::type::bitmap)
						{
							nl::node framenode = src[is_back ? "ladder" : "stand1"]["0"];

							for (nl::node partnode : framenode)
								if (partnode.data_type() == nl::node::type::bitmap)
								{
									view = partnode;
									break;
								}
						}

						if (view.data_type() != nl::node::type::bitmap)
							continue;

						const HeadRef& headref =
							(is_back && back_head.valid) ? back_head : front_head;

						if (!headref.valid)
							continue;

						Point<int16_t> hat_origin;
						bool got = false;

						if (AiSkin::shell_pixels(itemid, info, view, false, px, w, h))
						{
							hat_origin = view["origin"];
							got = true;
						}

						if (got && !px.empty())
						{
							write_onhead(outdir + "/onhead." + family + ".png", headref,
								is_back ? std::vector<HairPiece>() : hair, px, w, h, hat_origin);
							written++;
						}
					}
				}

				// Inline aura frames (info/effect subtree), with the material
				// accent tint applied when effectTint=1 — previews the final
				// in-game glow color
				nl::node eff = info["effect"];

				if (eff && eff.data_type() != nl::node::type::string)
				{
					float tr = 1.0f, tg = 1.0f, tb = 1.0f;

					if (static_cast<int32_t>(info["effectTint"]) != 0)
						AiSkin::accent_color(itemid, info, tr, tg, tb);

					auto dump_effect_frame = [&](nl::node bmpnode, const std::string& name)
					{
						if (bmpnode.data_type() != nl::node::type::bitmap)
							return;

						nl::bitmap bmp = bmpnode;

						if (bmp.width() <= 0 || bmp.height() <= 0)
							return;

						const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
						std::vector<uint8_t> tinted(data, data + size_t(bmp.width()) * bmp.height() * 4);

						for (size_t i = 0; i + 3 < tinted.size(); i += 4)
						{
							tinted[i] = uint8_t(tinted[i] * tb);
							tinted[i + 1] = uint8_t(tinted[i + 1] * tg);
							tinted[i + 2] = uint8_t(tinted[i + 2] * tr);
						}

						if (write_pixels(outdir + "/" + name, tinted, bmp.width(), bmp.height()))
							written++;
					};

					for (const char* layer : { "0", "1" })
					{
						nl::node layernode = eff[layer];

						if (layernode.data_type() == nl::node::type::bitmap)
							dump_effect_frame(layernode, std::string("effect.") + layer + ".png");
						else
							for (int sub = 0; layernode[std::to_string(sub)]; ++sub)
								dump_effect_frame(layernode[std::to_string(sub)], std::string("effect.") + layer + "." + std::to_string(sub) + ".png");
					}
				}

				if (written > 0)
					std::cout << "[CustomEquip] Preview: " << written << " synthesized frames for item " << itemid << " -> " << outdir << std::endl;
				else
					std::cout << "[CustomEquip] Preview: item " << itemid << " has no aiSkin/aiShell data" << std::endl;
			}
		}

			// ---- Full-character on-body composites ----
			// The item rendered on the complete mannequin character (body +
			// head + hair) in key poses, using the same anchor math and shell
			// family/lean logic the game uses — armor/capes/hats are judged
			// from these images before anything is published.

			std::vector<HairPiece> load_back_hair();

			struct Sprite
			{
				std::vector<uint8_t> px;
				int16_t w = 0, h = 0;
				Point<int16_t> tl;   // top-left in navel-space
			};

			Point<int16_t> map_point(nl::node part, const char* name)
			{
				return part["map"][name] ? Point<int16_t>(part["map"][name]) : Point<int16_t>(0, 0);
			}

			bool raw_pixels(nl::node bmpnode, std::vector<uint8_t>& out, int16_t& w, int16_t& h)
			{
				if (bmpnode.data_type() != nl::node::type::bitmap)
					return false;

				nl::bitmap bmp = bmpnode;
				w = bmp.width();
				h = bmp.height();

				if (w <= 0 || h <= 0)
					return false;

				const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
				out.assign(data, data + size_t(w) * h * 4);

				return true;
			}

			// Rotate a sprite around a pivot (given in sprite-local coords) —
			// the same nearest-sample rotation the client's lean/prone paths use
			void rotate_sprite(Sprite& s, Point<int16_t> pivot, float rot)
			{
				float c = std::cos(rot);
				float sn = std::sin(rot);

				float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;

				for (int corner = 0; corner < 4; ++corner)
				{
					float cx = (corner & 1) ? float(s.w) : 0.0f;
					float cy = (corner & 2) ? float(s.h) : 0.0f;
					float rx = c * (cx - pivot.x()) - sn * (cy - pivot.y());
					float ry = sn * (cx - pivot.x()) + c * (cy - pivot.y());

					min_x = std::min(min_x, rx);
					max_x = std::max(max_x, rx);
					min_y = std::min(min_y, ry);
					max_y = std::max(max_y, ry);
				}

				int16_t out_w = int16_t(std::ceil(max_x - min_x)) + 1;
				int16_t out_h = int16_t(std::ceil(max_y - min_y)) + 1;

				if (out_w <= 0 || out_h <= 0 || out_w > 512 || out_h > 512)
					return;

				Point<int16_t> out_pivot(int16_t(-min_x), int16_t(-min_y));
				std::vector<uint8_t> out(size_t(out_w) * out_h * 4, 0);

				for (int16_t y = 0; y < out_h; ++y)
				{
					for (int16_t x = 0; x < out_w; ++x)
					{
						float rx = float(x - out_pivot.x());
						float ry = float(y - out_pivot.y());
						int src_x = int(std::floor(c * rx + sn * ry + pivot.x()));
						int src_y = int(std::floor(-sn * rx + c * ry + pivot.y()));

						if (src_x < 0 || src_y < 0 || src_x >= s.w || src_y >= s.h)
							continue;

						std::copy_n(&s.px[(size_t(src_y) * s.w + src_x) * 4], 4, &out[(size_t(y) * out_w + x) * 4]);
					}
				}

				// Keep the pivot's navel-space position fixed
				s.tl = s.tl + pivot - out_pivot;
				s.px = std::move(out);
				s.w = out_w;
				s.h = out_h;
			}

			void composite_onbody(int32_t itemid)
			{
				std::string strid = "0" + std::to_string(itemid);
				std::string category = EquipData::get(itemid).get_itemdata().get_category();

				if (category != "Longcoat" && category != "Coat" && category != "Cape" && category != "Cap")
					return;

				nl::node src = nl::nx::character[category][strid + ".img"];

				if (!src)
					return;

				nl::node info = src["info"];
				std::string outdir = "Custom/Preview/" + std::to_string(itemid);
				std::filesystem::create_directories(outdir);
				int written = 0;

				struct PoseDef { const char* st; uint8_t fr; };
				const PoseDef poses[] = {
					{ "stand1", 0 }, { "walk1", 1 }, { "swingT1", 1 },
					{ "prone", 0 }, { "jump", 0 }, { "ladder", 0 }
				};

				bool is_cape = category == "Cape";
				bool is_coat = category == "Longcoat" || category == "Coat";
				bool is_cap = category == "Cap";
				bool has_material = AiSkin::available(itemid, info);
				nl::node shell = info["aiShell"];
				bool has_shell = shell["upright"].data_type() == nl::node::type::bitmap;

				// Spine baseline for the attack lean (same math as the client)
				nl::node base_body = nl::nx::character["00002000.img"]["stand1"]["0"]["body"];
				Point<int16_t> base_navel = map_point(base_body, "navel");
				Point<int16_t> base_neck = map_point(base_body, "neck");
				float base_spine = std::atan2(
					float(base_neck.x() - base_navel.x()),
					float(base_navel.y() - base_neck.y()));

				for (const auto& pose : poses)
				{
					nl::node bodyfr = nl::nx::character["00002000.img"][pose.st][pose.fr];
					nl::node bodypart = bodyfr["body"];

					if (bodypart.data_type() != nl::node::type::bitmap || !bodypart["map"]["navel"])
						continue;

					Point<int16_t> m_navel = map_point(bodypart, "navel");
					Point<int16_t> m_neck = map_point(bodypart, "neck");
					Point<int16_t> neck_world = m_neck - m_navel;

					std::string stname = pose.st;
					bool is_swing = stname.rfind("swing", 0) == 0 || stname.rfind("stab", 0) == 0;
					bool is_prone_pose = stname.rfind("prone", 0) == 0;
					bool is_climb = stname == "ladder" || stname == "rope";
					bool is_jump = stname == "jump";

					std::vector<Sprite> layers;

					auto add_navel_part = [&](nl::node part, bool use_material)
					{
						Sprite s;
						std::vector<uint8_t> px;
						int16_t w = 0, h = 0;
						bool got = false;

						if (use_material && has_material)
							got = AiSkin::part_pixels(itemid, info, stname, part.name(), part, px, w, h);

						if (!got)
							got = raw_pixels(part, px, w, h);

						if (!got || !part["map"]["navel"])
							return;

						Point<int16_t> o = part["origin"];
						s.px = std::move(px);
						s.w = w;
						s.h = h;
						s.tl = Point<int16_t>(0, 0) - o - map_point(part, "navel");
						layers.push_back(std::move(s));
					};

					// Shell view for this pose (family + stand-ins + lean),
					// mirroring the client's selection
					auto make_shell_sprite = [&](Sprite& out_sprite) -> bool
					{
						nl::node view = shell["upright"];
						bool rotate_prone = false;
						bool squash_jump = false;

						if (is_prone_pose)
						{
							if (shell["prone"])
								view = shell["prone"];
							else
								rotate_prone = true;
						}
						else if (is_climb && shell["back"])
							view = shell["back"];
						else if (is_swing && shell["attack"])
							view = shell["attack"];
						else if (is_jump)
						{
							if (shell["jump"])
								view = shell["jump"];
							else
								squash_jump = true;
						}

						if (view && view.data_type() != nl::node::type::bitmap)
							view = view["0"];

						if (view.data_type() != nl::node::type::bitmap)
							return false;

						std::vector<uint8_t> px;
						int16_t w = 0, h = 0;

						if (!AiSkin::shell_pixels(itemid, info, view, false, px, w, h))
							return false;

						Sprite s;
						Point<int16_t> origin = view["origin"];
						s.px = std::move(px);
						s.w = w;
						s.h = h;
						s.tl = neck_world - origin;

						if (squash_jump)
						{
							int16_t new_h = std::max<int16_t>(1, int16_t(h * 0.82f));
							std::vector<uint8_t> sq(size_t(w) * new_h * 4);

							for (int16_t y = 0; y < new_h; ++y)
								std::copy_n(&s.px[size_t(std::min<int>(h - 1, int(y / 0.82f))) * w * 4], size_t(w) * 4, &sq[size_t(y) * w * 4]);

							s.px = std::move(sq);
							s.h = new_h;
						}

						if (rotate_prone)
							rotate_sprite(s, origin, -1.5707963f);

						if (is_swing)
						{
							float spine = std::atan2(
								float(m_neck.x() - m_navel.x()),
								float(m_navel.y() - m_neck.y()));
							float lean = std::clamp(spine - base_spine, -0.9f, 0.9f);
							int deg = int(std::lround(lean * 180.0f / 3.14159265f / 5.0f)) * 5;

							if (std::abs(deg) >= 10)
								rotate_sprite(s, origin, float(deg) * 3.14159265f / 180.0f);
						}

						out_sprite = std::move(s);

						return true;
					};

					// 1. Cape item (or its shell) — behind everything
					if (is_cape)
					{
						Sprite s;

						if (has_shell && make_shell_sprite(s))
							layers.push_back(std::move(s));
						else
							for (nl::node part : src[stname][pose.fr])
								if (part.data_type() == nl::node::type::bitmap)
									add_navel_part(part, true);
					}

					// 2. Body
					{
						Sprite s;
						std::vector<uint8_t> px;
						int16_t w = 0, h = 0;

						if (raw_pixels(bodypart, px, w, h))
						{
							Point<int16_t> o = bodypart["origin"];
							s.px = std::move(px);
							s.w = w;
							s.h = h;
							s.tl = Point<int16_t>(0, 0) - o - m_navel;
							layers.push_back(std::move(s));
						}
					}

					// 3. Coat item: shell or retextured donor mail
					if (is_coat)
					{
						Sprite s;

						if (has_shell && make_shell_sprite(s))
							layers.push_back(std::move(s));
						else
							for (nl::node part : src[stname][pose.fr])
								if (part.data_type() == nl::node::type::bitmap && part.name() == "mail")
									add_navel_part(part, true);
					}

					// 4. Remaining body parts (arm, hands) over the coat
					for (nl::node part : bodyfr)
						if (part.data_type() == nl::node::type::bitmap && part.name() != "body")
							add_navel_part(part, false);

					// 5. Coat sleeves
					if (is_coat)
						for (nl::node part : src[stname][pose.fr])
							if (part.data_type() == nl::node::type::bitmap && part.name() == "mailArm")
								add_navel_part(part, true);

					// 6. Head
					nl::node headpart = nl::nx::character["00012000.img"][stname][pose.fr]["head"];
					Point<int16_t> brow_world;
					bool have_head = false;

					if (headpart.data_type() == nl::node::type::bitmap && headpart["map"]["neck"])
					{
						Sprite s;
						std::vector<uint8_t> px;
						int16_t w = 0, h = 0;

						if (raw_pixels(headpart, px, w, h))
						{
							Point<int16_t> o = headpart["origin"];
							Point<int16_t> hneck = map_point(headpart, "neck");
							s.px = std::move(px);
							s.w = w;
							s.h = h;
							s.tl = neck_world - o - hneck;
							brow_world = neck_world - hneck + map_point(headpart, "brow");
							have_head = true;
							layers.push_back(std::move(s));
						}
					}

					// 7. Hair
					if (have_head)
					{
						std::vector<HairPiece> hair = is_climb ? load_back_hair() : load_hair();
						std::string vslot = (std::string)info["vslot"];
						bool fullcover = is_cap &&
							(vslot.find("As") != std::string::npos || vslot.find("Ay") != std::string::npos);

						if (!fullcover)
							for (const auto& piece : hair)
							{
								Sprite s;
								s.px = piece.bgra;
								s.w = piece.width;
								s.h = piece.height;
								s.tl = brow_world + piece.tl_rel_brow;
								layers.push_back(std::move(s));
							}
					}

					// 8. Hat item
					if (is_cap && have_head)
						for (nl::node part : src[stname][pose.fr])
							if (part.data_type() == nl::node::type::bitmap)
							{
								Sprite s;
								std::vector<uint8_t> px;
								int16_t w = 0, h = 0;

								if (!raw_pixels(part, px, w, h))
									continue;

								Point<int16_t> o = part["origin"];
								s.px = std::move(px);
								s.w = w;
								s.h = h;
								s.tl = brow_world - o - map_point(part, "brow");
								layers.push_back(std::move(s));
							}

					if (layers.empty())
						continue;

					// Compose
					int left = 9999, top = 9999, right = -9999, bottom = -9999;

					for (const auto& s : layers)
					{
						left = std::min<int>(left, s.tl.x() - 2);
						top = std::min<int>(top, s.tl.y() - 2);
						right = std::max<int>(right, s.tl.x() + s.w + 2);
						bottom = std::max<int>(bottom, s.tl.y() + s.h + 2);
					}

					int cw = right - left;
					int ch = bottom - top;

					if (cw <= 0 || ch <= 0 || cw > 512 || ch > 512)
						continue;

					std::vector<uint8_t> canvas(size_t(cw) * ch * 4, 0);

					for (const auto& s : layers)
						blit(canvas, cw, ch, s.px, s.w, s.h, s.tl.x() - left, s.tl.y() - top);

					// 3x upscale for readability
					int ow = cw * 3, oh = ch * 3;
					std::vector<uint8_t> big(size_t(ow) * oh * 4);

					for (int y = 0; y < oh; ++y)
						for (int x = 0; x < ow; ++x)
							std::copy_n(&canvas[(size_t(y / 3) * cw + x / 3) * 4], 4, &big[(size_t(y) * ow + x) * 4]);

					if (write_pixels(outdir + "/onbody." + stname + ".png", big, int16_t(ow), int16_t(oh)))
						written++;
				}

				if (written > 0)
					std::cout << "[CustomEquip] On-body: " << written << " pose composites for item " << itemid << " -> " << outdir << std::endl;
			}

			// Mannequin templates for the paint-on-head hat pipeline: the AI
			// paints a hat ONTO these; the diff against the template isolates
			// the hat pixels, and the known brow position yields an exact
			// origin — placement is baked into the generation, never guessed.
			void write_mannequin(const char* name, const HeadRef& head, const std::vector<HairPiece>& hair)
			{
				if (!head.valid)
					return;

				// Fixed 1x canvas around the brow: brow lands at (28, 24)
				constexpr int LEFT = -28, TOP = -24, W = 64, H = 56, SCALE = 8;

				std::vector<uint8_t> canvas(size_t(W) * H * 4);

				// Flat green background — unchanged pixels diff out later
				for (size_t i = 0; i < canvas.size(); i += 4)
				{
					canvas[i] = 48;       // B
					canvas[i + 1] = 190;  // G
					canvas[i + 2] = 106;  // R
					canvas[i + 3] = 255;
				}

				blit(canvas, W, H, head.bgra, head.width, head.height, head.tl_rel_brow.x() - LEFT, head.tl_rel_brow.y() - TOP);

				for (const auto& piece : hair)
					blit(canvas, W, H, piece.bgra, piece.width, piece.height, piece.tl_rel_brow.x() - LEFT, piece.tl_rel_brow.y() - TOP);

				// 8x nearest upscale
				std::vector<uint8_t> big(size_t(W) * SCALE * H * SCALE * 4);

				for (int y = 0; y < H * SCALE; ++y)
					for (int x = 0; x < W * SCALE; ++x)
						std::copy_n(&canvas[(size_t(y / SCALE) * W + x / SCALE) * 4], 4, &big[(size_t(y) * W * SCALE + x) * 4]);

				std::filesystem::create_directories("Custom/HatTemplate");
				write_pixels(std::string("Custom/HatTemplate/") + name + ".png", big, int16_t(W * SCALE), int16_t(H * SCALE));

				std::ofstream meta(std::string("Custom/HatTemplate/") + name + ".txt");
				meta << "canvas_1x=" << W << "x" << H << "\n";
				meta << "scale=" << SCALE << "\n";
				meta << "brow_px_8x=(" << (-LEFT * SCALE) << "," << (-TOP * SCALE) << ")\n";
				meta << "brow_1x=(" << -LEFT << "," << -TOP << ")\n";
			}

			std::vector<HairPiece> load_back_hair()
			{
				std::vector<HairPiece> pieces;
				nl::node d = nl::nx::character["Hair"]["00030000.img"]["backDefault"];

				for (nl::node piecenode : d)
				{
					if (piecenode.data_type() != nl::node::type::bitmap || !piecenode["map"]["brow"])
						continue;

					nl::bitmap bmp = piecenode;

					if (bmp.width() <= 0 || bmp.height() <= 0)
						continue;

					Point<int16_t> origin = piecenode["origin"];
					Point<int16_t> brow = piecenode["map"]["brow"];

					HairPiece piece;
					const uint8_t* data = static_cast<const uint8_t*>(bmp.data());
					piece.width = bmp.width();
					piece.height = bmp.height();
					piece.bgra.assign(data, data + size_t(piece.width) * piece.height * 4);
					piece.tl_rel_brow = Point<int16_t>(
						static_cast<int16_t>(-(origin.x() + brow.x())),
						static_cast<int16_t>(-(origin.y() + brow.y())));
					pieces.push_back(std::move(piece));
				}

				return pieces;
			}

		void run()
		{
			std::ifstream trigger("Custom/export.txt");

			if (trigger)
			{
				int32_t itemid;

				while (trigger >> itemid)
					export_equip(itemid);
			}

			std::ifstream preview("Custom/preview.txt");

			if (preview)
			{
				int32_t itemid;

				while (preview >> itemid)
				{
					preview_equip(itemid);
					composite_onbody(itemid);
				}
			}

			// Always refresh the hat mannequin templates (cheap, and the
			// launcher wipes Custom/ on NX sync)
			write_mannequin("mannequin_front", load_head("stand1"), load_hair());
			write_mannequin("mannequin_back", load_head("ladder"), load_back_hair());
		}
	}
}
