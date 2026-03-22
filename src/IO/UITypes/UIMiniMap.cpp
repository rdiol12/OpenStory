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
#include "UIMiniMap.h"

#include "UIWorldMap.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/MapleMap/Npc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIMiniMap::UIMiniMap(const CharStats& stats) : UIDragElement<PosMINIMAP>(Point<int16_t>(128, 20)), stats(stats)
	{
		big_map = true;
		scroll_locked = false;
		has_map = false;
		listNpc_enabled = false;
		drag_resize = false;
		zoom_scale = 1.0f;
		drag_start_y = 0;
		drag_start_zoom = 1.0f;
		listNpc_dimensions = Point<int16_t>(150, 170);
		listNpc_offset = 0;
		selected = -1;

		type = Setting<MiniMapType>::get().load();
		user_type = type;
		simpleMode = Setting<MiniMapSimpleMode>::get().load();

		std::string node = simpleMode ? "MiniMapSimpleMode" : "MiniMap";
		MiniMap = nl::nx::ui["UIWindow2.img"][node];
		listNpc = nl::nx::ui["UIWindow2.img"]["MiniMap"]["ListNpc"];

		buttons[Buttons::BT_MIN] = std::make_unique<MapleButton>(MiniMap["BtMin"], Point<int16_t>(195, -6));
		buttons[Buttons::BT_MAX] = std::make_unique<MapleButton>(MiniMap["BtMax"], Point<int16_t>(209, -6));
		buttons[Buttons::BT_SMALL] = std::make_unique<MapleButton>(MiniMap["BtSmall"], Point<int16_t>(223, -6));
		buttons[Buttons::BT_BIG] = std::make_unique<MapleButton>(MiniMap["BtBig"], Point<int16_t>(223, -6));
		buttons[Buttons::BT_MAP] = std::make_unique<MapleButton>(MiniMap["BtMap"], Point<int16_t>(237, -6));
		buttons[Buttons::BT_NPC] = std::make_unique<MapleButton>(MiniMap["BtNpc"], Point<int16_t>(276, -6));

		// Scroll lock toggle reuses BtBig/BtSmall sprites as a toggle indicator
		// BtBig = unlocked (map scrolls to follow player), BtSmall = locked (map stays fixed)
		buttons[Buttons::BT_SCROLL_LOCK] = std::make_unique<MapleButton>(MiniMap["BtBig"], Point<int16_t>(260, -6));
		buttons[Buttons::BT_SCROLL_LOCK]->set_active(false);

		region_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		town_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		combined_text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE);

		marker = Setting<MiniMapDefaultHelpers>::get().load() ? nl::nx::ui["UIWindow2.img"]["MiniMapSimpleMode"]["DefaultHelper"] : nl::nx::map["MapHelper.img"]["minimap"];

		player_marker = Animation(marker["user"]);
		selected_marker = Animation(MiniMap["iconNpc"]);
		portal_marker_anim = Animation(marker["portal"]);
		npc_marker_anim = Animation(marker["npc"]);
		npc_highlight = ColorBox(LISTNPC_ITEM_WIDTH, LISTNPC_ITEM_HEIGHT, Color::Name::YELLOW, 1.0f);

		// Load additional mark textures from MiniMap.img
		nl::node minimap_marks = nl::nx::ui["MiniMap.img"];
		if (minimap_marks.size() > 0)
		{
			for (auto mark_node : minimap_marks)
			{
				std::string name = mark_node.name();
				if (mark_node.data_type() == nl::node::type::bitmap)
					mark_textures[name] = Texture(mark_node);
			}
		}
	}

	void UIMiniMap::draw(float alpha) const
	{
		if (type == Type::MIN)
		{
			for (auto sprite : min_sprites)
				sprite.draw(position, alpha);

			combined_text.draw(position + Point<int16_t>(7, -3));
		}
		else if (type == Type::NORMAL)
		{
			for (auto sprite : normal_sprites)
				sprite.draw(position, alpha);

			if (has_map)
			{
				Point<int16_t> scroll_off = get_scroll_offset(normal_dimensions, 0);

				for (auto sprite : static_marker_info)
					portal_marker_anim.draw(position + sprite.second + scroll_off, alpha);

				draw_movable_markers(position + scroll_off, alpha);

				if (listNpc_enabled)
					draw_npclist(normal_dimensions, alpha);
			}
		}
		else
		{
			for (auto sprite : max_sprites)
				sprite.draw(position, alpha);

			region_text.draw(position + Point<int16_t>(48, 14));
			town_text.draw(position + Point<int16_t>(48, 28));

			if (has_map)
			{
				Point<int16_t> scroll_off = get_scroll_offset(max_dimensions, MAX_ADJ);

				for (auto sprite : static_marker_info)
					portal_marker_anim.draw(position + sprite.second + Point<int16_t>(0, MAX_ADJ) + scroll_off, alpha);

				draw_movable_markers(position + Point<int16_t>(0, MAX_ADJ) + scroll_off, alpha);

				if (listNpc_enabled)
					draw_npclist(max_dimensions, alpha);
			}
		}

		UIElement::draw(alpha);
	}

	void UIMiniMap::update()
	{
		int32_t mid = Stage::get().get_mapid();

		if (mid != mapid)
		{
			mapid = mid;
			Map = NxHelper::Map::get_map_node_name(mapid);

			nl::node town = Map["info"]["town"];
			nl::node miniMap = Map["miniMap"];

			if (!miniMap)
			{
				has_map = false;
				type = Type::MIN;
			}
			else
			{
				has_map = true;

				if (town && town.get_bool())
					type = Type::MAX;
				else
					type = user_type;
			}

			scale = std::pow(2, (int)miniMap["mag"]);
			center_offset = Point<int16_t>(miniMap["centerX"], miniMap["centerY"]);

			update_text();
			update_buttons();
			update_canvas();
			update_static_markers();
			toggle_buttons();
			update_npclist();
		}

		if (type == Type::MIN)
		{
			for (auto sprite : min_sprites)
				sprite.update();
		}
		else if (type == Type::NORMAL)
		{
			for (auto sprite : normal_sprites)
				sprite.update();
		}
		else
		{
			for (auto sprite : max_sprites)
				sprite.update();
		}

		if (listNpc_enabled)
			for (Sprite sprite : listNpc_sprites)
				sprite.update();

		if (selected >= 0)
			selected_marker.update();

		UIElement::update();
	}

	void UIMiniMap::remove_cursor()
	{
		UIDragElement::remove_cursor();

		listNpc_slider.remove_cursor();

		UI::get().clear_tooltip(Tooltip::Parent::MINIMAP);
	}

	Cursor::State UIMiniMap::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Handle drag-to-resize on the bottom edge (only in NORMAL/MAX mode)
		if (type != Type::MIN && has_map)
		{
			Point<int16_t> cursor_relative = cursorpos - position;
			int16_t bottom_y = dimension.y();
			int16_t right_x = dimension.x();

			// 8px resize zone on bottom and bottom-right corner
			bool on_bottom = cursor_relative.y() >= bottom_y - 8 && cursor_relative.y() <= bottom_y + 4
				&& cursor_relative.x() >= 0 && cursor_relative.x() <= right_x;

			if (drag_resize)
			{
				if (clicked)
				{
					// Use cursor Y movement from drag start to compute zoom change
					int16_t delta_y = cursorpos.y() - drag_start_y;
					float new_zoom = drag_start_zoom + static_cast<float>(delta_y) / 100.0f;
					new_zoom = std::max(0.5f, std::min(4.0f, new_zoom));

					if (new_zoom != zoom_scale)
					{
						zoom_scale = new_zoom;
						update_canvas();
						update_static_markers();
						toggle_buttons();
					}

					return Cursor::State::CLICKING;
				}
				else
				{
					drag_resize = false;
				}
			}
			else if (on_bottom && clicked)
			{
				drag_resize = true;
				drag_start_y = cursorpos.y();
				drag_start_zoom = zoom_scale;
				return Cursor::State::CLICKING;
			}
			else if (on_bottom)
			{
				return Cursor::State::VSCROLL;
			}
		}

		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> cursor_relative = cursorpos - position;

		if (listNpc_slider.isenabled())
			if (Cursor::State new_state = listNpc_slider.send_cursor(cursor_relative, clicked))
				return new_state;

		if (listNpc_enabled)
		{
			Point<int16_t> relative_point = cursor_relative - Point<int16_t>(10 + (type == Type::MAX ? max_dimensions : normal_dimensions).x(), 23);
			Rectangle<int16_t> list_bounds = Rectangle<int16_t>(0, LISTNPC_ITEM_WIDTH, 0, LISTNPC_ITEM_HEIGHT * 8);

			if (list_bounds.contains(relative_point))
			{
				int16_t list_index = listNpc_offset + relative_point.y() / LISTNPC_ITEM_HEIGHT;
				bool in_list = list_index < listNpc_names.size();

				if (clicked)
					select_npclist(in_list ? list_index : -1);
				else if (in_list)
					UI::get().show_text(Tooltip::Parent::MINIMAP, listNpc_full_names[list_index]);

				return Cursor::State::IDLE;
			}
		}

		bool found = false;
		auto npcs = Stage::get().get_npcs().get_npcs();

		for (auto npc = npcs->begin(); npc != npcs->end(); npc++)
		{
			Point<int16_t> npc_pos = scale_map_pos(npc->second->get_position()) + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
			Rectangle<int16_t> marker_spot = Rectangle<int16_t>(npc_pos - Point<int16_t>(4, 8), npc_pos);

			if (type == Type::MAX)
				marker_spot.shift(Point<int16_t>(0, MAX_ADJ));

			if (marker_spot.contains(cursor_relative))
			{
				found = true;

				auto n = static_cast<Npc*>(npc->second.get());
				std::string name = n->get_name();
				std::string func = n->get_func();

				UI::get().show_map(Tooltip::Parent::MINIMAP, name, func, {}, false);
				break;
			}
		}

		if (!found)
		{
			for (auto sprite : static_marker_info)
			{
				Rectangle<int16_t> marker_spot = Rectangle<int16_t>(sprite.second, sprite.second + 8);

				if (type == Type::MAX)
					marker_spot.shift(Point<int16_t>(0, MAX_ADJ));

				if (marker_spot.contains(cursor_relative))
				{
					nl::node portal_tm = Map["portal"][sprite.first]["tm"];
					std::string portal_cat = NxHelper::Map::get_map_category(portal_tm);
					nl::node portal_name = nl::nx::string["Map.img"][portal_cat][portal_tm]["mapName"];

					if (portal_name)
					{
						found = true;

						UI::get().show_map(Tooltip::Parent::MINIMAP, portal_name, "", portal_tm, false);
						break;
					}
				}
			}
		}

		return Cursor::State::IDLE;
	}

	void UIMiniMap::send_scroll(double yoffset)
	{
		if (listNpc_enabled && listNpc_slider.isenabled())
			listNpc_slider.send_scroll(yoffset);
	}

	void UIMiniMap::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (has_map)
		{
			if (type < Type::MAX)
				type++;
			else
				type = Type::MIN;

			user_type = type;

			toggle_buttons();
		}
	}

	Button::State UIMiniMap::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_MIN:
			type -= 1;
			toggle_buttons();
			return type == Type::MIN ? Button::State::DISABLED : Button::State::NORMAL;
		case BT_MAX:
			type += 1;
			toggle_buttons();
			return type == Type::MAX ? Button::State::DISABLED : Button::State::NORMAL;
		case BT_SMALL:
		case BT_BIG:
			big_map = !big_map;
			toggle_buttons();
			break;
		case BT_SCROLL_LOCK:
			scroll_locked = !scroll_locked;
			toggle_buttons();
			break;
		case BT_MAP:
			UI::get().emplace<UIWorldMap>();
			break;
		case BT_NPC:
			set_npclist_active(!listNpc_enabled);
			break;
		}

		return Button::State::NORMAL;
	}

	UIElement::Type UIMiniMap::get_type() const
	{
		return TYPE;
	}

	void UIMiniMap::update_buttons()
	{
		// Add one pixel for a space to the right of each button
		bt_min_width = buttons[Buttons::BT_MIN]->width() + 1;
		bt_max_width = buttons[Buttons::BT_MAX]->width() + 1;
		bt_map_width = buttons[Buttons::BT_MAP]->width() + 1;

		combined_text_width = combined_text.width();
	}

	void UIMiniMap::toggle_buttons()
	{
		int16_t bt_min_x;

		if (type == Type::MIN)
		{
			buttons[Buttons::BT_MAP]->set_active(true);
			buttons[Buttons::BT_MAX]->set_active(true);
			buttons[Buttons::BT_MIN]->set_active(true);
			buttons[Buttons::BT_NPC]->set_active(false);
			buttons[Buttons::BT_SMALL]->set_active(false);
			buttons[Buttons::BT_BIG]->set_active(false);
			buttons[Buttons::BT_SCROLL_LOCK]->set_active(false);

			buttons[Buttons::BT_MIN]->set_state(Button::State::DISABLED);

			if (has_map)
				buttons[Buttons::BT_MAX]->set_state(Button::State::NORMAL);
			else
				buttons[Buttons::BT_MAX]->set_state(Button::State::DISABLED);

			bt_min_x = combined_text_width + 11;

			buttons[Buttons::BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_min_width;

			buttons[Buttons::BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			min_dimensions = Point<int16_t>(bt_min_x + bt_map_width + 7, 20);

			update_dimensions();

			dragarea = dimension;

			set_npclist_active(false);
		}
		else
		{
			bool has_npcs = Stage::get().get_npcs().get_npcs()->size() > 0;

			buttons[Buttons::BT_MAP]->set_active(true);
			buttons[Buttons::BT_MAX]->set_active(true);
			buttons[Buttons::BT_MIN]->set_active(true);
			buttons[Buttons::BT_NPC]->set_active(has_npcs);
			buttons[Buttons::BT_SCROLL_LOCK]->set_active(true);

			if (big_map)
			{
				buttons[Buttons::BT_BIG]->set_active(false);
				buttons[Buttons::BT_SMALL]->set_active(true);
			}
			else
			{
				buttons[Buttons::BT_BIG]->set_active(true);
				buttons[Buttons::BT_SMALL]->set_active(false);
			}

			// Show scroll lock state: PRESSED = locked (fixed), NORMAL = unlocked (follows player)
			buttons[Buttons::BT_SCROLL_LOCK]->set_state(scroll_locked ? Button::State::PRESSED : Button::State::NORMAL);

			buttons[Buttons::BT_MIN]->set_state(Button::State::NORMAL);

			int16_t scroll_btn_width = buttons[Buttons::BT_SCROLL_LOCK]->width() + 1;
			bt_min_x = middle_right_x - (bt_min_width + buttons[Buttons::BT_SMALL]->width() + 1 + bt_max_width + bt_map_width + scroll_btn_width + (has_npcs ? buttons[Buttons::BT_NPC]->width() : 0));

			buttons[Buttons::BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_SMALL]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
			buttons[Buttons::BT_BIG]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_SCROLL_LOCK]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += scroll_btn_width;

			buttons[Buttons::BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_map_width;

			buttons[Buttons::BT_NPC]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			if (type == Type::MAX)
				buttons[Buttons::BT_MAX]->set_state(Button::State::DISABLED);
			else
				buttons[Buttons::BT_MAX]->set_state(Button::State::NORMAL);

			set_npclist_active(listNpc_enabled && has_npcs);

			dragarea = Point<int16_t>(dimension.x(), 20);
		}
	}

	void UIMiniMap::update_text()
	{
		NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(mapid);
		combined_text.change_text(map_info.full_name);
		region_text.change_text(map_info.name);
		town_text.change_text(map_info.street_name);
	}

	void UIMiniMap::update_canvas()
	{
		min_sprites.clear();
		normal_sprites.clear();
		max_sprites.clear();

		nl::node Min, Normal, Max;

		if (simpleMode)
		{
			Min = MiniMap["Window"]["Min"];
			Normal = MiniMap["Window"]["Normal"];
			Max = MiniMap["Window"]["Max"];
		}
		else
		{
			Min = MiniMap["Min"];
			Normal = MiniMap["MinMap"];
			Max = MiniMap["MaxMap"];
		}

		map_sprite = Texture(Map["miniMap"]["canvas"]);
		Point<int16_t> map_dimensions = map_sprite.get_dimensions();

		// Apply zoom scale to get the effective display size of the map
		int16_t zoomed_w = static_cast<int16_t>(map_dimensions.x() * zoom_scale);
		int16_t zoomed_h = static_cast<int16_t>(map_dimensions.y() * zoom_scale);

		// 48 (offset for text) + longer text's width + 10 (space for right side border)
		int16_t mark_text_width = 48 + std::max(region_text.width(), town_text.width()) + 10;
		int16_t c_stretch, ur_x_offset, m_stretch, down_y_offset;
		int16_t window_width = std::max(178, std::max((int)mark_text_width, zoomed_w + 20));

		c_stretch = std::max(0, window_width - 128);
		ur_x_offset = CENTER_START_X + c_stretch;
		map_draw_origin_x = std::max(10, window_width / 2 - zoomed_w / 2);

		if (zoomed_h <= 20)
		{
			m_stretch = 5;
			down_y_offset = 17 + m_stretch;
			map_draw_origin_y = 10 + m_stretch - zoomed_h;
		}
		else
		{
			m_stretch = zoomed_h - 17;
			down_y_offset = 17 + m_stretch;
			map_draw_origin_y = 20;
		}

		middle_right_x = ur_x_offset + 55;

		std::string Left = simpleMode ? "Left" : "w";
		std::string Center = simpleMode ? "Center" : "c";
		std::string Right = simpleMode ? "Right" : "e";

		std::string DownCenter = simpleMode ? "DownCenter" : "s";
		std::string DownLeft = simpleMode ? "DownLeft" : "sw";
		std::string DownRight = simpleMode ? "DownRight" : "se";
		std::string MiddleLeft = simpleMode ? "MiddleLeft" : "w";
		std::string MiddleRight = simpleMode ? "MiddleRight" : "e";
		std::string UpCenter = simpleMode ? "UpCenter" : "n";
		std::string UpLeft = simpleMode ? "UpLeft" : "nw";
		std::string UpRight = simpleMode ? "UpRight" : "ne";

		// SimpleMode's backdrop is opaque, the other is transparent but lightly colored
		// UI.wz v208 has normal center sprite in-linked to bottom right window frame, not sure why.
		nl::node MiddleCenter = simpleMode ? MiniMap["Window"]["Max"]["MiddleCenter"] : MiniMap["MaxMap"]["c"];

		int16_t dl_dr_y = std::max(map_dimensions.y(), (int16_t)10);

		// combined_text_width + 14 (7px buffer on both sides) + 4 (buffer between name and buttons) + 3 buttons' widths - 128 (length of left and right window borders)
		int16_t min_c_stretch = combined_text_width + 18 + bt_min_width + bt_max_width + bt_map_width - 128;

		// Min sprites queue
		min_sprites.emplace_back(Min[Center], DrawArgument(WINDOW_UL_POS + Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(min_c_stretch, 0)));
		min_sprites.emplace_back(Min[Left], DrawArgument(WINDOW_UL_POS));
		min_sprites.emplace_back(Min[Right], DrawArgument(WINDOW_UL_POS + Point<int16_t>(min_c_stretch + CENTER_START_X, 0)));

		// Normal sprites queue
		// (7, 10) is the top left corner of the inner window
		// 114 = 128 (width of left and right borders) - 14 (width of middle borders * 2).
		// 27 = height of inner frame drawn on up and down borders
		normal_sprites.emplace_back(MiddleCenter, DrawArgument(Point<int16_t>(7, 10), Point<int16_t>(c_stretch + 114, m_stretch + 27)));

		if (has_map)
			normal_sprites.emplace_back(Map["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y), zoom_scale, zoom_scale));

		normal_sprites.emplace_back(Normal[MiddleLeft], DrawArgument(Point<int16_t>(0, ML_MR_Y), Point<int16_t>(0, m_stretch)));
		normal_sprites.emplace_back(Normal[MiddleRight], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y), Point<int16_t>(0, m_stretch)));
		normal_sprites.emplace_back(Normal[UpCenter], DrawArgument(Point<int16_t>(CENTER_START_X, 0) + WINDOW_UL_POS, Point<int16_t>(c_stretch, 0)));
		normal_sprites.emplace_back(Normal[UpLeft], WINDOW_UL_POS);
		normal_sprites.emplace_back(Normal[UpRight], DrawArgument(Point<int16_t>(ur_x_offset, 0) + WINDOW_UL_POS));
		normal_sprites.emplace_back(Normal[DownCenter], DrawArgument(Point<int16_t>(CENTER_START_X, down_y_offset + 18), Point<int16_t>(c_stretch, 0)));
		normal_sprites.emplace_back(Normal[DownLeft], Point<int16_t>(0, down_y_offset));
		normal_sprites.emplace_back(Normal[DownRight], Point<int16_t>(ur_x_offset, down_y_offset));

		normal_dimensions = Point<int16_t>(ur_x_offset + 64, down_y_offset + 27);

		// Max sprites queue
		max_sprites.emplace_back(MiddleCenter, DrawArgument(Point<int16_t>(7, 50), Point<int16_t>(c_stretch + 114, m_stretch + 27)));

		if (has_map)
			max_sprites.emplace_back(Map["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y + MAX_ADJ), zoom_scale, zoom_scale));

		max_sprites.emplace_back(Max[MiddleLeft], DrawArgument(Point<int16_t>(0, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
		max_sprites.emplace_back(Max[MiddleRight], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
		max_sprites.emplace_back(Max[UpCenter], DrawArgument(Point<int16_t>(CENTER_START_X, 0) + WINDOW_UL_POS, Point<int16_t>(c_stretch, 0)));
		max_sprites.emplace_back(Max[UpLeft], WINDOW_UL_POS);
		max_sprites.emplace_back(Max[UpRight], DrawArgument(Point<int16_t>(ur_x_offset, 0) + WINDOW_UL_POS));
		max_sprites.emplace_back(Max[DownCenter], DrawArgument(Point<int16_t>(CENTER_START_X, down_y_offset + MAX_ADJ + 18), Point<int16_t>(c_stretch, 0)));
		max_sprites.emplace_back(Max[DownLeft], Point<int16_t>(0, down_y_offset + MAX_ADJ));
		max_sprites.emplace_back(Max[DownRight], Point<int16_t>(ur_x_offset, down_y_offset + MAX_ADJ));
		max_sprites.emplace_back(nl::nx::map["MapHelper.img"]["mark"][Map["info"]["mapMark"]], DrawArgument(Point<int16_t>(7, 17)));

		max_dimensions = normal_dimensions + Point<int16_t>(0, MAX_ADJ);
	}

	Point<int16_t> UIMiniMap::scale_map_pos(Point<int16_t> world_pos) const
	{
		Point<int16_t> map_pos = (world_pos + center_offset) / scale;
		return Point<int16_t>(
			static_cast<int16_t>(map_pos.x() * zoom_scale),
			static_cast<int16_t>(map_pos.y() * zoom_scale)
		);
	}

	void UIMiniMap::draw_movable_markers(Point<int16_t> init_pos, float alpha) const
	{
		if (!has_map)
			return;

		Animation marker_sprite;
		Point<int16_t> sprite_offset;
		Point<int16_t> origin(map_draw_origin_x, map_draw_origin_y);

		/// NPCs
		MapObjects* npcs = Stage::get().get_npcs().get_npcs();
		marker_sprite = Animation(marker["npc"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
		{
			Point<int16_t> npc_pos = npc->second.get()->get_position();
			marker_sprite.draw(scale_map_pos(npc_pos) - sprite_offset + origin + init_pos, alpha);
		}

		/// Other characters
		MapObjects* chars = Stage::get().get_chars().get_chars();
		marker_sprite = Animation(marker["another"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto chr = chars->begin(); chr != chars->end(); ++chr)
		{
			Point<int16_t> chr_pos = chr->second.get()->get_position();
			marker_sprite.draw(scale_map_pos(chr_pos) - sprite_offset + origin + init_pos, alpha);
		}

		/// Player
		Point<int16_t> player_pos = Stage::get().get_player().get_position();
		sprite_offset = player_marker.get_dimensions() / Point<int16_t>(2, 0);
		player_marker.draw(scale_map_pos(player_pos) - sprite_offset + origin + init_pos, alpha);
	}

	Point<int16_t> UIMiniMap::get_scroll_offset(Point<int16_t> view_dims, int16_t y_adj) const
	{
		if (scroll_locked || !has_map)
			return Point<int16_t>(0, 0);

		// Calculate where the player would be drawn on the minimap
		Point<int16_t> player_pos = Stage::get().get_player().get_position();
		Point<int16_t> player_on_map = scale_map_pos(player_pos) + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);

		// The visible area center (approximate center of the minimap view)
		int16_t view_center_x = view_dims.x() / 2;
		int16_t view_center_y = (view_dims.y() + y_adj) / 2;

		// Offset to center the player in the view
		Point<int16_t> offset = Point<int16_t>(view_center_x - player_on_map.x(), view_center_y - player_on_map.y());

		// Clamp so the map doesn't scroll past its edges
		Point<int16_t> map_dims = map_sprite.get_dimensions();
		int16_t zoomed_mx = static_cast<int16_t>(map_dims.x() * zoom_scale);
		int16_t zoomed_my = static_cast<int16_t>(map_dims.y() * zoom_scale);
		int16_t max_offset_x = 0;
		int16_t min_offset_x = std::min(0, (int)view_dims.x() - zoomed_mx - map_draw_origin_x * 2);
		int16_t max_offset_y = 0;
		int16_t min_offset_y = std::min(0, (int)(view_dims.y() - y_adj) - zoomed_my - map_draw_origin_y);

		offset = Point<int16_t>(
			static_cast<int16_t>(std::max((int)min_offset_x, std::min((int)max_offset_x, (int)offset.x()))),
			static_cast<int16_t>(std::max((int)min_offset_y, std::min((int)max_offset_y, (int)offset.y())))
		);

		return offset;
	}

	void UIMiniMap::update_static_markers()
	{
		static_marker_info.clear();

		if (!has_map)
			return;

		Animation marker_sprite;

		/// Portals
		nl::node portals = Map["portal"];
		marker_sprite = Animation(marker["portal"]);
		Point<int16_t> marker_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (nl::node portal = portals.begin(); portal != portals.end(); ++portal)
		{
			int portal_type = portal["pt"];

			if (portal_type == 2)
			{
				Point<int16_t> marker_pos = scale_map_pos(Point<int16_t>(portal["x"], portal["y"])) - marker_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
				static_marker_info.emplace_back(portal.name(), marker_pos);
			}
		}
	}

	void UIMiniMap::set_npclist_active(bool active)
	{
		listNpc_enabled = active;

		if (!active)
			select_npclist(-1);

		update_dimensions();
	}

	void UIMiniMap::update_dimensions()
	{
		if (type == Type::MIN)
		{
			dimension = min_dimensions;
		}
		else
		{
			Point<int16_t> base_dims = type == Type::MAX ? max_dimensions : normal_dimensions;
			dimension = base_dims;

			if (listNpc_enabled)
			{
				dimension += listNpc_dimensions;
				dimension.set_y(std::max(base_dims.y(), listNpc_dimensions.y()));
			}
		}
	}

	void UIMiniMap::update_npclist()
	{
		listNpc_sprites.clear();
		listNpc_names.clear();
		listNpc_full_names.clear();
		listNpc_list.clear();
		selected = -1;
		listNpc_offset = 0;

		if (simpleMode)
			return;

		auto npcs = Stage::get().get_npcs().get_npcs();

		for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
		{
			listNpc_list.emplace_back(npc->second.get());

			auto n = static_cast<Npc*>(npc->second.get());
			std::string name = n->get_name();
			std::string func = n->get_func();

			if (func != "")
				name += " (" + func + ")";

			Text name_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);

			listNpc_names.emplace_back(name_text);
			listNpc_full_names.emplace_back(name);
		}

		for (size_t i = 0; i < listNpc_names.size(); i++)
			string_format::format_with_ellipsis(listNpc_names[i], LISTNPC_TEXT_WIDTH - (listNpc_names.size() > 8 ? 0 : 20));

		const Point<int16_t> listNpc_pos = Point<int16_t>(type == Type::MAX ? max_dimensions.x() : normal_dimensions.x(), 0);
		int16_t c_stretch = 20;
		int16_t m_stretch = 102;

		if (listNpc_names.size() > 8)
		{
			listNpc_slider = Slider(
				Slider::DEFAULT_SILVER, Range<int16_t>(23, 11 + LISTNPC_ITEM_HEIGHT * 8), listNpc_pos.x() + LISTNPC_ITEM_WIDTH + 1, 8, listNpc_names.size(),
				[&](bool upwards)
				{
					int16_t shift = upwards ? -1 : 1;
					bool above = listNpc_offset + shift >= 0;
					bool below = listNpc_offset + 8 + shift <= listNpc_names.size();

					if (above && below)
						listNpc_offset += shift;
				}
			);

			c_stretch += 12;
		}
		else
		{
			listNpc_slider.setenabled(false);
			m_stretch = LISTNPC_ITEM_HEIGHT * listNpc_names.size() - 34;
			c_stretch -= 17;
		}

		listNpc_sprites.emplace_back(listNpc["c"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, M_START), Point<int16_t>(c_stretch, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["w"], DrawArgument(listNpc_pos + Point<int16_t>(0, M_START), Point<int16_t>(0, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["e"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START), Point<int16_t>(0, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["n"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["s"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, M_START + m_stretch), Point<int16_t>(c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["nw"], DrawArgument(listNpc_pos + Point<int16_t>(0, 0)));
		listNpc_sprites.emplace_back(listNpc["ne"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["sw"], DrawArgument(listNpc_pos + Point<int16_t>(0, M_START + m_stretch)));
		listNpc_sprites.emplace_back(listNpc["se"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START + m_stretch)));

		listNpc_dimensions = Point<int16_t>(CENTER_START_X * 2 + c_stretch, M_START + m_stretch + 30);

		update_dimensions();
	}

	void UIMiniMap::draw_npclist(Point<int16_t> minimap_dims, float alpha) const
	{

		for (Sprite sprite : listNpc_sprites)
			sprite.draw(position, alpha);

		Point<int16_t> listNpc_pos = position + Point<int16_t>(minimap_dims.x() + 10, 23);

		for (int8_t i = 0; i + listNpc_offset < listNpc_list.size() && i < 8; i++)
		{
			if (selected - listNpc_offset == i)
				npc_highlight.draw(listNpc_pos);

			npc_marker_anim.draw(DrawArgument(listNpc_pos + Point<int16_t>(0, 2), false, npc_marker_anim.get_dimensions() / 2), alpha);
			listNpc_names[listNpc_offset + i].draw(DrawArgument(listNpc_pos + Point<int16_t>(14, -2)));

			listNpc_pos.shift_y(LISTNPC_ITEM_HEIGHT);
		}

		if (listNpc_slider.isenabled())
			listNpc_slider.draw(position);

		if (selected >= 0)
		{
			Point<int16_t> npc_pos =
				scale_map_pos(listNpc_list[selected]->get_position()) +
				Point<int16_t>(map_draw_origin_x, map_draw_origin_y - npc_marker_anim.get_dimensions().y() + (type == Type::MAX ? MAX_ADJ : 0));

			selected_marker.draw(position + npc_pos, 0.5f);
		}
	}

	void UIMiniMap::select_npclist(int16_t choice)
	{
		if (selected == choice)
			return;

		if (selected >= 0 && selected < listNpc_names.size())
			listNpc_names[selected].change_color(Color::Name::WHITE);

		if (choice > listNpc_names.size() || choice < 0)
		{
			selected = -1;
		}
		else
		{
			selected = choice != selected ? choice : -1;

			if (selected >= 0)
				listNpc_names[selected].change_color(Color::Name::BLACK);
		}
	}

	void UIMiniMap::set_quest_npc(int32_t npcid)
	{
		if (npcid <= 0)
			return;

		// Enable NPC list if not already showing
		if (!listNpc_enabled)
			set_npclist_active(true);

		// Find the NPC in the list by its data ID
		for (size_t i = 0; i < listNpc_list.size(); i++)
		{
			auto npc = static_cast<Npc*>(listNpc_list[i]);

			if (npc && npc->get_npcid() == npcid)
			{
				select_npclist(static_cast<int16_t>(i));
				return;
			}
		}
	}
}