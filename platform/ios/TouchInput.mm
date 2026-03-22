//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Touch Input Implementation
//	Maps touch events to the game's UI input system.
//////////////////////////////////////////////////////////////////////////////////
#import "TouchInput.h"

#include "IO/UI.h"
#include "Constants.h"
#include "Timer.h"

namespace
{
	std::chrono::time_point<std::chrono::steady_clock> last_tap_time;
	bool is_pressed = false;

	ms::Point<int16_t> screen_to_game(CGPoint point, UIView* view)
	{
		CGFloat scale = [UIScreen mainScreen].nativeScale;
		int vw = ms::Constants::Constants::get().get_viewwidth();
		int vh = ms::Constants::Constants::get().get_viewheight();

		CGFloat view_w = view.bounds.size.width;
		CGFloat view_h = view.bounds.size.height;

		int16_t x = static_cast<int16_t>(point.x * vw / view_w);
		int16_t y = static_cast<int16_t>(point.y * vh / view_h);

		return ms::Point<int16_t>(x, y);
	}
}

@implementation TouchInput

+ (void)handleTouchesBegan:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:view];

	// Multi-touch: two fingers = right click
	if (touches.count >= 2)
	{
		ms::UI::get().rightclick();
		return;
	}

	ms::Point<int16_t> pos = screen_to_game(location, view);
	ms::UI::get().send_cursor(pos);
	ms::UI::get().send_cursor(true);
	is_pressed = true;
}

+ (void)handleTouchesMoved:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:view];

	ms::Point<int16_t> pos = screen_to_game(location, view);
	ms::UI::get().send_cursor(pos);
}

+ (void)handleTouchesEnded:(NSSet<UITouch*>*)touches inView:(UIView*)view
{
	if (!is_pressed)
		return;

	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:view];

	ms::Point<int16_t> pos = screen_to_game(location, view);
	ms::UI::get().send_cursor(pos);

	// Check for double tap
	auto now = ms::ContinuousTimer::get().start();
	auto diff_ms = ms::ContinuousTimer::get().stop(last_tap_time) / 1000;
	last_tap_time = now;

	if (diff_ms > 10 && diff_ms < 300)
		ms::UI::get().doubleclick();

	ms::UI::get().send_cursor(false);
	is_pressed = false;
}

@end
