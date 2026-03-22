//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Virtual Touch Controls
//	Provides on-screen D-pad and action buttons for gameplay.
//	Rendered as semi-transparent overlays on top of the game scene.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef PLATFORM_IOS
#include <OpenGLES/ES3/gl.h>
#else
#include <glew.h>
#endif

#include <cstdint>
#include <functional>

namespace ms
{
	class VirtualControls
	{
	public:
		// Button identifiers
		enum Button : uint8_t
		{
			NONE = 0,
			DPAD_LEFT,
			DPAD_RIGHT,
			DPAD_UP,
			DPAD_DOWN,
			BTN_JUMP,
			BTN_ATTACK,
			BTN_PICKUP,
			BTN_SKILL1,
			BTN_SKILL2,
			BTN_SKILL3,
			BTN_MENU,
			NUM_BUTTONS
		};

		VirtualControls();

		// Initialize GL resources for rendering controls
		void init(int16_t screen_w, int16_t screen_h);

		// Process a touch at game coordinates; returns which button was hit
		Button hit_test(int16_t x, int16_t y) const;

		// Check if a point is within the controls area (to suppress cursor movement)
		bool is_in_controls_area(int16_t x, int16_t y) const;

		// Draw the virtual controls overlay
		void draw() const;

		// Get whether controls are visible
		bool visible() const;
		void set_visible(bool v);

		// Update screen dimensions (e.g. on rotation)
		void update_layout(int16_t screen_w, int16_t screen_h);

	private:
		struct Rect
		{
			int16_t x, y, w, h;

			bool contains(int16_t px, int16_t py) const
			{
				return px >= x && px < x + w && py >= y && py < y + h;
			}

			int16_t cx() const { return x + w / 2; }
			int16_t cy() const { return y + h / 2; }
		};

		void layout(int16_t screen_w, int16_t screen_h);
		void draw_circle(int16_t cx, int16_t cy, int16_t radius, float r, float g, float b, float a) const;
		void draw_rect(const Rect& rect, float r, float g, float b, float a) const;
		void draw_arrow(int16_t cx, int16_t cy, int16_t size, int dir) const;

		Rect dpad_left, dpad_right, dpad_up, dpad_down;
		Rect btn_jump, btn_attack, btn_pickup;
		Rect btn_skill1, btn_skill2, btn_skill3;
		Rect btn_menu;
		Rect dpad_area;   // bounding box around entire d-pad
		Rect action_area; // bounding box around action buttons

		int16_t screen_width;
		int16_t screen_height;
		bool is_visible;
		bool initialized;
	};
}
