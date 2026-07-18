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
#include "BodyDrawInfo.h"

#include "Body.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/bitmap.hpp>
#endif

// Arm-bend detection (bitmap centreline scan)
#include <vector>
#include <cmath>

namespace ms
{
	void BodyDrawInfo::init()
	{
		nl::node bodynode = nl::nx::character["00002000.img"];
		nl::node headnode = nl::nx::character["00012000.img"];

		for (nl::node stancenode : bodynode)
		{
			std::string ststr = stancenode.name();

			uint16_t attackdelay = 0;

			for (uint8_t frame = 0; nl::node framenode = stancenode[frame]; ++frame)
			{
				bool isaction = framenode["action"].data_type() == nl::node::type::string;

				if (isaction)
				{
					BodyAction action = framenode;
					body_actions[ststr][frame] = action;

					if (action.isattackframe())
						attack_delays[ststr].push_back(attackdelay);

					attackdelay += action.get_delay();
				}
				else
				{
					Stance::Id stance = Stance::by_string(ststr);
					int16_t delay = framenode["delay"];

					if (delay <= 0)
						delay = 100;

					stance_delays[stance][frame] = delay;

					std::unordered_map<Body::Layer, std::unordered_map<std::string, Point<int16_t>>> bodyshiftmap;

					// Track the layer the body "arm" part draws on this frame, so
					// the procedural sleeve can mirror it for per-frame z-order.
					Body::Layer arm_z = Body::Layer::ARM;
					bool arm_present = false;

					// Detected elbow, carried out of the part loop. arm_elbow_rel is in
					// the arm-anchor frame relative to the arm navel; adding the body
					// navel (known only after the loop) finishes the map to body space,
					// the same transform get_arm_frame uses for the hand.
					Point<int16_t> arm_elbow_rel;
					float arm_bend = 0.0f;
					bool arm_has_elbow = false;

					for (auto partnode : framenode)
					{
						std::string part = partnode.name();

						if (part != "delay" && part != "face")
						{
							std::string zstr = (std::string)partnode["z"];
							Body::Layer z = Body::layer_by_name(zstr);

							for (auto mapnode : partnode["map"])
								bodyshiftmap[z].emplace(mapnode.name(), mapnode);

							if (part == "arm")
							{
								arm_z = z;
								arm_present = true;

								// DETECT the elbow from the arm bitmap: scan each row for the
								// horizontal centre of the opaque pixels (the arm's
								// centreline), fit a straight shoulder->hand line, and take
								// the row of max perpendicular deviation as the elbow.
								// bend = that deviation in px (~0 straight, larger = a fold).
								nl::bitmap bmp = partnode;
								int w = bmp.width();
								int h = bmp.height();
								const uint8_t* pix = static_cast<const uint8_t*>(bmp.data());

								std::vector<int> cx(h, -1);
								int top = -1, bot = -1;
								if (pix && w > 0 && h > 0)
								{
									for (int y = 0; y < h; ++y)
									{
										int mn = -1, mx = -1;
										for (int x = 0; x < w; ++x)
										{
											uint8_t a = pix[(static_cast<size_t>(y) * w + x) * 4 + 3];
											if (a > 16) { if (mn < 0) mn = x; mx = x; }
										}
										if (mn >= 0) { cx[y] = (mn + mx) / 2; if (top < 0) top = y; bot = y; }
									}
								}

								float bend = 0.0f;
								int elbow_y = -1, elbow_x = -1;
								if (top >= 0 && bot > top)
								{
									float x0 = static_cast<float>(cx[top]);
									float dx = static_cast<float>(cx[bot] - cx[top]);
									float dy = static_cast<float>(bot - top);
									float len = std::sqrt(dx * dx + dy * dy);
									if (len < 1.0f) len = 1.0f;
									for (int y = top; y <= bot; ++y)
									{
										if (cx[y] < 0) continue;
										float d = std::fabs((cx[y] - x0) * dy - (y - top) * dx) / len;
										if (d > bend) { bend = d; elbow_y = y; elbow_x = cx[y]; }
									}
								}

								if (elbow_x >= 0)
								{
									// pixel -> arm-anchor frame relative to the arm navel:
									// (pixel - origin) - a_navel. Add body navel after the loop.
									Point<int16_t> origin = partnode["origin"];
									Point<int16_t> a_navel = partnode["map"]["navel"];
									arm_elbow_rel = Point<int16_t>(
										static_cast<int16_t>(elbow_x - origin.x() - a_navel.x()),
										static_cast<int16_t>(elbow_y - origin.y() - a_navel.y()));
									arm_bend = bend;
									arm_has_elbow = true;
								}
							}
						}
					}

					nl::node headmap = headnode[ststr][frame]["head"]["map"];

					for (auto mapnode : headmap)
						bodyshiftmap[Body::Layer::HEAD].emplace(mapnode.name(), mapnode);

					body_positions[stance][frame] = bodyshiftmap[Body::Layer::BODY]["navel"];
					neck_positions[stance][frame] = bodyshiftmap[Body::Layer::BODY]["neck"];

					arm_positions[stance][frame] = bodyshiftmap.count(Body::Layer::ARM) ?
						(bodyshiftmap[Body::Layer::ARM]["hand"] - bodyshiftmap[Body::Layer::ARM]["navel"] + bodyshiftmap[Body::Layer::BODY]["navel"]) :
						(bodyshiftmap[Body::Layer::ARM_OVER_HAIR]["hand"] - bodyshiftmap[Body::Layer::ARM_OVER_HAIR]["navel"] + bodyshiftmap[Body::Layer::BODY]["navel"]);

					hand_positions[stance][frame] = bodyshiftmap[Body::Layer::HAND_BELOW_WEAPON]["handMove"];
					head_positions[stance][frame] = bodyshiftmap[Body::Layer::BODY]["neck"] - bodyshiftmap[Body::Layer::HEAD]["neck"];
					face_positions[stance][frame] = bodyshiftmap[Body::Layer::BODY]["neck"] - bodyshiftmap[Body::Layer::HEAD]["neck"] + bodyshiftmap[Body::Layer::HEAD]["brow"];
					hair_positions[stance][frame] = bodyshiftmap[Body::Layer::HEAD]["brow"] - bodyshiftmap[Body::Layer::HEAD]["neck"] + bodyshiftmap[Body::Layer::BODY]["neck"];

					// Procedural-sleeve arm rig. neck/hand are raw body anchors
					// (same frame as the torso navel above); shoulder is the
					// derived pivot neck + SHOULDER_OFFSET, seeded (6,2) from the
					// vanilla sleeve and refined on the offline eyeball render.
					// v83 body data has no explicit shoulder joint, so this is a
					// tuned constant, not raw data.
					ArmFrame af;
					af.layer = static_cast<int16_t>(arm_z);
					af.neck = bodyshiftmap[Body::Layer::BODY]["neck"];
					af.shoulder = af.neck + Point<int16_t>(6, 2);
					af.hand = arm_positions[stance][frame];
					af.present = arm_present;

					// Finish mapping the detected elbow into body space and decide
					// whether this frame folds enough to draw the sleeve as 2 segments.
					af.bent = false;
					af.elbow = af.hand; // harmless default if no elbow detected
					if (arm_has_elbow)
					{
						af.elbow = arm_elbow_rel + bodyshiftmap[Body::Layer::BODY]["navel"];
						constexpr float BEND_THRESHOLD = 3.0f; // px; below this ~ straight
						af.bent = arm_bend > BEND_THRESHOLD;
					}

					arm_frames[stance][frame] = af;
				}
			}
		}
	}

	Point<int16_t> BodyDrawInfo::get_body_position(Stance::Id stance, uint8_t frame) const
	{
		auto iter = body_positions[stance].find(frame);

		if (iter == body_positions[stance].end())
			return {};

		return iter->second;
	}

	Point<int16_t> BodyDrawInfo::get_neck_position(Stance::Id stance, uint8_t frame) const
	{
		auto iter = neck_positions[stance].find(frame);

		if (iter == neck_positions[stance].end())
			return {};

		return iter->second;
	}

	Point<int16_t> BodyDrawInfo::get_arm_position(Stance::Id stance, uint8_t frame) const
	{
		auto iter = arm_positions[stance].find(frame);

		if (iter == arm_positions[stance].end())
			return {};

		return iter->second;
	}

	const BodyDrawInfo::ArmFrame& BodyDrawInfo::get_arm_frame(Stance::Id stance, uint8_t frame) const
	{
		// present=false sentinel for frames with no arm anchor (e.g. prone).
		static const ArmFrame absent{ 0, {}, {}, {}, {}, false, false };

		auto iter = arm_frames[stance].find(frame);
		return iter != arm_frames[stance].end() ? iter->second : absent;
	}

	Point<int16_t> BodyDrawInfo::get_hand_position(Stance::Id stance, uint8_t frame) const
	{
		auto iter = hand_positions[stance].find(frame);

		if (iter == hand_positions[stance].end())
			return {};

		return iter->second;
	}

	Point<int16_t> BodyDrawInfo::get_head_position(Stance::Id stance, uint8_t frame) const
	{
		auto iter = head_positions[stance].find(frame);

		if (iter == head_positions[stance].end())
			return {};

		return iter->second;
	}

	Point<int16_t> BodyDrawInfo::gethairpos(Stance::Id stance, uint8_t frame) const
	{
		auto iter = hair_positions[stance].find(frame);

		if (iter == hair_positions[stance].end())
			return {};

		return iter->second;
	}

	Point<int16_t> BodyDrawInfo::getfacepos(Stance::Id stance, uint8_t frame) const
	{
		auto iter = face_positions[stance].find(frame);

		if (iter == face_positions[stance].end())
			return {};

		return iter->second;
	}

	uint8_t BodyDrawInfo::nextframe(Stance::Id stance, uint8_t frame) const
	{
		if (stance_delays[stance].count(frame + 1))
			return frame + 1;
		else
			return 0;
	}

	uint16_t BodyDrawInfo::get_delay(Stance::Id stance, uint8_t frame) const
	{
		auto iter = stance_delays[stance].find(frame);

		if (iter == stance_delays[stance].end())
			return 100;

		return iter->second;
	}

	uint16_t BodyDrawInfo::get_attackdelay(std::string action, size_t no) const
	{
		auto action_iter = attack_delays.find(action);

		if (action_iter != attack_delays.end())
			if (no < action_iter->second.size())
				return action_iter->second[no];

		return 0;
	}

	uint8_t BodyDrawInfo::next_actionframe(std::string action, uint8_t frame) const
	{
		auto action_iter = body_actions.find(action);

		if (action_iter != body_actions.end())
			if (action_iter->second.count(frame + 1))
				return frame + 1;

		return 0;
	}

	const BodyAction* BodyDrawInfo::get_action(std::string action, uint8_t frame) const
	{
		auto action_iter = body_actions.find(action);

		if (action_iter != body_actions.end())
		{
			auto frame_iter = action_iter->second.find(frame);

			if (frame_iter != action_iter->second.end())
				return &(frame_iter->second);
		}

		return nullptr;
	}
}
