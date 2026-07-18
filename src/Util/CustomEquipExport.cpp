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
					preview_equip(itemid);
			}

			// Always refresh the hat mannequin templates (cheap, and the
			// launcher wipes Custom/ on NX sync)
			write_mannequin("mannequin_front", load_head("stand1"), load_hair());
			write_mannequin("mannequin_back", load_head("ladder"), load_back_hair());
		}
	}
}
