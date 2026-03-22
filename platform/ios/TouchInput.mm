//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Touch Input Implementation
//	Maps touch events to virtual controls and the game's UI input system.
//////////////////////////////////////////////////////////////////////////////////
#import "TouchInput.h"

#include "IO/UI.h"
#include "IO/KeyAction.h"
#include "IO/KeyType.h"
#include "Constants.h"
#include "KeyCodes.h"

namespace
{
	ms::VirtualControls vcontrols;

	// Track which button each touch is pressing (multi-touch support)
	struct TouchState
	{
		uintptr_t touch_id;
		ms::VirtualControls::Button button;
	};

	static const int MAX_TOUCHES = 10;
	TouchState active_touches[MAX_TOUCHES] = {};
	int active_count = 0;

	// Map virtual control buttons to GLFW keycodes for the game's key system
	int32_t button_to_keycode(ms::VirtualControls::Button btn)
	{
		switch (btn)
		{
		case ms::VirtualControls::DPAD_LEFT:  return GLFW_KEY_LEFT;
		case ms::VirtualControls::DPAD_RIGHT: return GLFW_KEY_RIGHT;
		case ms::VirtualControls::DPAD_UP:    return GLFW_KEY_UP;
		case ms::VirtualControls::DPAD_DOWN:  return GLFW_KEY_DOWN;
		case ms::VirtualControls::BTN_JUMP:   return GLFW_KEY_LEFT_ALT;   // Jump is typically Alt
		case ms::VirtualControls::BTN_ATTACK: return GLFW_KEY_LEFT_CONTROL; // Attack is typically Ctrl
		case ms::VirtualControls::BTN_PICKUP: return GLFW_KEY_Z;          // Pickup is Z
		case ms::VirtualControls::BTN_MENU:   return GLFW_KEY_ESCAPE;
		default: return 0;
		}
	}

	// Skill buttons use the server's keybinding system
	// They map to specific keyboard keys that the server assigns skills to
	int32_t skill_button_to_keycode(ms::VirtualControls::Button btn)
	{
		switch (btn)
		{
		case ms::VirtualControls::BTN_SKILL1: return GLFW_KEY_Q;
		case ms::VirtualControls::BTN_SKILL2: return GLFW_KEY_W;
		case ms::VirtualControls::BTN_SKILL3: return GLFW_KEY_E;
		default: return 0;
		}
	}

	ms::Point<int16_t> screen_to_game(CGPoint point, UIView* view)
	{
		int vw = ms::Constants::Constants::get().get_viewwidth();
		int vh = ms::Constants::Constants::get().get_viewheight();

		CGFloat view_w = view.bounds.size.width;
		CGFloat view_h = view.bounds.size.height;

		int16_t x = static_cast<int16_t>(point.x * vw / view_w);
		int16_t y = static_cast<int16_t>(point.y * vh / view_h);

		return ms::Point<int16_t>(x, y);
	}

	void add_touch(uintptr_t tid, ms::VirtualControls::Button btn)
	{
		if (active_count < MAX_TOUCHES)
		{
			active_touches[active_count] = { tid, btn };
			active_count++;
		}
	}

	ms::VirtualControls::Button find_touch(uintptr_t tid)
	{
		for (int i = 0; i < active_count; i++)
		{
			if (active_touches[i].touch_id == tid)
				return active_touches[i].button;
		}
		return ms::VirtualControls::NONE;
	}

	void remove_touch(uintptr_t tid)
	{
		for (int i = 0; i < active_count; i++)
		{
			if (active_touches[i].touch_id == tid)
			{
				active_touches[i] = active_touches[active_count - 1];
				active_count--;
				return;
			}
		}
	}

	void press_button(ms::VirtualControls::Button btn)
	{
		int32_t kc = button_to_keycode(btn);
		if (kc == 0)
			kc = skill_button_to_keycode(btn);
		if (kc != 0)
			ms::UI::get().send_key(kc, true);
	}

	void release_button(ms::VirtualControls::Button btn)
	{
		int32_t kc = button_to_keycode(btn);
		if (kc == 0)
			kc = skill_button_to_keycode(btn);
		if (kc != 0)
			ms::UI::get().send_key(kc, false);
	}
}

@implementation TouchInput

+ (ms::VirtualControls&)controls
{
	return vcontrols;
}

+ (void)initControls:(int16_t)width height:(int16_t)height
{
	vcontrols.init(width, height);
}

+ (void)handleTouchesBegan:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	for (UITouch* touch in touches)
	{
		CGPoint location = [touch locationInView:view];
		ms::Point<int16_t> pos = screen_to_game(location, view);
		uintptr_t tid = (uintptr_t)touch;

		// Check if touch hits a virtual control button
		auto btn = vcontrols.hit_test(pos.x(), pos.y());

		if (btn != ms::VirtualControls::NONE)
		{
			add_touch(tid, btn);
			press_button(btn);
		}
		else
		{
			// Touch is in the game area — treat as cursor click
			add_touch(tid, ms::VirtualControls::NONE);
			ms::UI::get().send_cursor(pos);
			ms::UI::get().send_cursor(true);
		}
	}
}

+ (void)handleTouchesMoved:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	for (UITouch* touch in touches)
	{
		CGPoint location = [touch locationInView:view];
		ms::Point<int16_t> pos = screen_to_game(location, view);
		uintptr_t tid = (uintptr_t)touch;

		auto existing = find_touch(tid);

		if (existing != ms::VirtualControls::NONE)
		{
			// Touch was on a control button — check if it slid to a different button
			auto new_btn = vcontrols.hit_test(pos.x(), pos.y());

			if (new_btn != existing)
			{
				// Release old button, press new one
				release_button(existing);
				remove_touch(tid);

				if (new_btn != ms::VirtualControls::NONE)
				{
					add_touch(tid, new_btn);
					press_button(new_btn);
				}
				else
				{
					add_touch(tid, ms::VirtualControls::NONE);
				}
			}
		}
		else
		{
			// Cursor movement in game area — always update position during drag
			// Use CLICKING state so the UI drag system knows finger is still held
			ms::UI::get().send_cursor(pos, ms::Cursor::State::CLICKING);
		}
	}
}

+ (void)handleTouchesEnded:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	for (UITouch* touch in touches)
	{
		CGPoint location = [touch locationInView:view];
		ms::Point<int16_t> pos = screen_to_game(location, view);
		uintptr_t tid = (uintptr_t)touch;

		auto btn = find_touch(tid);

		if (btn != ms::VirtualControls::NONE)
		{
			release_button(btn);
		}
		else
		{
			// Release cursor click
			ms::UI::get().send_cursor(pos);
			ms::UI::get().send_cursor(false);
		}

		remove_touch(tid);
	}
}

@end
