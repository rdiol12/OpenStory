//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Virtual Touch Controls Implementation
//	Renders semi-transparent D-pad and action buttons over the game scene.
//////////////////////////////////////////////////////////////////////////////////
#include "VirtualControls.h"
#include "Graphics/GraphicsGL.h"
#include "Constants.h"

#include <cmath>

namespace ms
{
	VirtualControls::VirtualControls()
	{
		is_visible = true;
		initialized = false;
		screen_width = 0;
		screen_height = 0;
	}

	void VirtualControls::init(int16_t sw, int16_t sh)
	{
		layout(sw, sh);
		initialized = true;
	}

	void VirtualControls::update_layout(int16_t sw, int16_t sh)
	{
		layout(sw, sh);
	}

	void VirtualControls::layout(int16_t sw, int16_t sh)
	{
		screen_width = sw;
		screen_height = sh;

		// D-pad layout — bottom-left corner
		// Sized relative to screen height for consistency
		int16_t pad_size = sh / 7;       // size of each d-pad button
		int16_t pad_margin = sh / 20;    // margin from edges
		int16_t pad_gap = 2;             // gap between buttons

		int16_t dpad_cx = pad_margin + pad_size + pad_size / 2;
		int16_t dpad_cy = sh - pad_margin - pad_size - pad_size / 2;

		dpad_left  = { static_cast<int16_t>(dpad_cx - pad_size - pad_size / 2 - pad_gap), static_cast<int16_t>(dpad_cy - pad_size / 2), pad_size, pad_size };
		dpad_right = { static_cast<int16_t>(dpad_cx + pad_size / 2 + pad_gap), static_cast<int16_t>(dpad_cy - pad_size / 2), pad_size, pad_size };
		dpad_up    = { static_cast<int16_t>(dpad_cx - pad_size / 2), static_cast<int16_t>(dpad_cy - pad_size - pad_size / 2 - pad_gap), pad_size, pad_size };
		dpad_down  = { static_cast<int16_t>(dpad_cx - pad_size / 2), static_cast<int16_t>(dpad_cy + pad_size / 2 + pad_gap), pad_size, pad_size };

		dpad_area = {
			static_cast<int16_t>(dpad_left.x - pad_margin / 2),
			static_cast<int16_t>(dpad_up.y - pad_margin / 2),
			static_cast<int16_t>(dpad_right.x + dpad_right.w - dpad_left.x + pad_margin),
			static_cast<int16_t>(dpad_down.y + dpad_down.h - dpad_up.y + pad_margin)
		};

		// Action buttons — bottom-right corner
		int16_t btn_size = static_cast<int16_t>(pad_size * 1.2);
		int16_t btn_margin = pad_margin;
		int16_t btn_gap = pad_size / 4;

		// Jump (bottom-right), Attack (above-left of jump)
		btn_jump   = { static_cast<int16_t>(sw - btn_margin - btn_size), static_cast<int16_t>(sh - btn_margin - btn_size), btn_size, btn_size };
		btn_attack = { static_cast<int16_t>(sw - btn_margin - btn_size * 2 - btn_gap), static_cast<int16_t>(sh - btn_margin - btn_size), btn_size, btn_size };
		btn_pickup = { static_cast<int16_t>(sw - btn_margin - btn_size), static_cast<int16_t>(sh - btn_margin - btn_size * 2 - btn_gap), btn_size, btn_size };

		// Skill buttons — row above action buttons
		int16_t skill_size = static_cast<int16_t>(pad_size * 0.9);
		btn_skill1 = { static_cast<int16_t>(sw - btn_margin - skill_size * 3 - btn_gap * 2), static_cast<int16_t>(sh - btn_margin - btn_size * 2 - btn_gap - skill_size - btn_gap), skill_size, skill_size };
		btn_skill2 = { static_cast<int16_t>(btn_skill1.x + skill_size + btn_gap), btn_skill1.y, skill_size, skill_size };
		btn_skill3 = { static_cast<int16_t>(btn_skill2.x + skill_size + btn_gap), btn_skill1.y, skill_size, skill_size };

		action_area = {
			static_cast<int16_t>(btn_skill1.x - btn_margin / 2),
			static_cast<int16_t>(btn_skill1.y - btn_margin / 2),
			static_cast<int16_t>(btn_jump.x + btn_jump.w - btn_skill1.x + btn_margin),
			static_cast<int16_t>(btn_jump.y + btn_jump.h - btn_skill1.y + btn_margin)
		};

		// Menu button — top-right
		int16_t menu_size = static_cast<int16_t>(pad_size * 0.7);
		btn_menu = { static_cast<int16_t>(sw - btn_margin - menu_size), btn_margin, menu_size, menu_size };
	}

	VirtualControls::Button VirtualControls::hit_test(int16_t x, int16_t y) const
	{
		if (!is_visible || !initialized)
			return NONE;

		// D-pad
		if (dpad_left.contains(x, y))  return DPAD_LEFT;
		if (dpad_right.contains(x, y)) return DPAD_RIGHT;
		if (dpad_up.contains(x, y))    return DPAD_UP;
		if (dpad_down.contains(x, y))  return DPAD_DOWN;

		// Action buttons
		if (btn_jump.contains(x, y))   return BTN_JUMP;
		if (btn_attack.contains(x, y)) return BTN_ATTACK;
		if (btn_pickup.contains(x, y)) return BTN_PICKUP;
		if (btn_skill1.contains(x, y)) return BTN_SKILL1;
		if (btn_skill2.contains(x, y)) return BTN_SKILL2;
		if (btn_skill3.contains(x, y)) return BTN_SKILL3;
		if (btn_menu.contains(x, y))   return BTN_MENU;

		return NONE;
	}

	bool VirtualControls::is_in_controls_area(int16_t x, int16_t y) const
	{
		if (!is_visible || !initialized)
			return false;

		return dpad_area.contains(x, y) || action_area.contains(x, y) || btn_menu.contains(x, y);
	}

	bool VirtualControls::visible() const
	{
		return is_visible;
	}

	void VirtualControls::set_visible(bool v)
	{
		is_visible = v;
	}

	void VirtualControls::draw() const
	{
		if (!is_visible || !initialized)
			return;

		auto& gl = GraphicsGL::get();

		float bg = 0.15f;
		float fg = 0.6f;
		float alpha = 0.35f;
		float alpha_btn = 0.4f;

		// D-pad buttons
		draw_rect(dpad_left,  bg, bg, bg, alpha);
		draw_rect(dpad_right, bg, bg, bg, alpha);
		draw_rect(dpad_up,    bg, bg, bg, alpha);
		draw_rect(dpad_down,  bg, bg, bg, alpha);

		// D-pad arrows
		draw_arrow(dpad_left.cx(),  dpad_left.cy(),  dpad_left.w / 4,  0); // left
		draw_arrow(dpad_right.cx(), dpad_right.cy(), dpad_right.w / 4, 1); // right
		draw_arrow(dpad_up.cx(),    dpad_up.cy(),    dpad_up.w / 4,    2); // up
		draw_arrow(dpad_down.cx(),  dpad_down.cy(),  dpad_down.w / 4,  3); // down

		// Action buttons — colored circles
		// Jump = green
		gl.drawrectangle(btn_jump.x, btn_jump.y, btn_jump.w, btn_jump.h, 0.1f, 0.6f, 0.2f, alpha_btn);
		// Attack = red
		gl.drawrectangle(btn_attack.x, btn_attack.y, btn_attack.w, btn_attack.h, 0.7f, 0.15f, 0.1f, alpha_btn);
		// Pickup = blue
		gl.drawrectangle(btn_pickup.x, btn_pickup.y, btn_pickup.w, btn_pickup.h, 0.1f, 0.3f, 0.7f, alpha_btn);

		// Skill buttons — purple tones
		gl.drawrectangle(btn_skill1.x, btn_skill1.y, btn_skill1.w, btn_skill1.h, 0.4f, 0.1f, 0.5f, alpha_btn);
		gl.drawrectangle(btn_skill2.x, btn_skill2.y, btn_skill2.w, btn_skill2.h, 0.5f, 0.1f, 0.4f, alpha_btn);
		gl.drawrectangle(btn_skill3.x, btn_skill3.y, btn_skill3.w, btn_skill3.h, 0.4f, 0.2f, 0.5f, alpha_btn);

		// Menu button
		gl.drawrectangle(btn_menu.x, btn_menu.y, btn_menu.w, btn_menu.h, 0.3f, 0.3f, 0.3f, 0.3f);
	}

	void VirtualControls::draw_rect(const Rect& rect, float r, float g, float b, float a) const
	{
		GraphicsGL::get().drawrectangle(rect.x, rect.y, rect.w, rect.h, r, g, b, a);
	}

	void VirtualControls::draw_arrow(int16_t cx, int16_t cy, int16_t size, int dir) const
	{
		// Draw a small filled rectangle as arrow indicator
		auto& gl = GraphicsGL::get();
		float fg = 0.9f;
		float a = 0.6f;

		switch (dir)
		{
		case 0: // left
			gl.drawrectangle(cx - size, cy - size / 3, size, size * 2 / 3, fg, fg, fg, a);
			break;
		case 1: // right
			gl.drawrectangle(cx, cy - size / 3, size, size * 2 / 3, fg, fg, fg, a);
			break;
		case 2: // up
			gl.drawrectangle(cx - size / 3, cy - size, size * 2 / 3, size, fg, fg, fg, a);
			break;
		case 3: // down
			gl.drawrectangle(cx - size / 3, cy, size * 2 / 3, size, fg, fg, fg, a);
			break;
		}
	}
}
