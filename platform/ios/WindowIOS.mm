//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Window Implementation for iOS
//	The GL context and swap buffers are managed by GLKViewController.
//	This class provides the same interface as the desktop Window class.
//////////////////////////////////////////////////////////////////////////////////
#include "WindowIOS.h"

#include "Configuration.h"
#include "Constants.h"
#include "Timer.h"

#import <UIKit/UIKit.h>

namespace ms
{
	Window::Window()
	{
		closed = false;
		fullscreen = true;
		opacity = 1.0f;
		opcstep = 0.0f;
		width = Constants::Constants::get().get_physicalwidth();
		height = Constants::Constants::get().get_physicalheight();
	}

	Window::~Window()
	{
	}

	Error Window::init()
	{
		// On iOS, the GL context is already set up by GameViewController
		// Just initialize the graphics engine
		if (Error error = GraphicsGL::get().init())
			return error;

		return initwindow();
	}

	Error Window::initwindow()
	{
		// Get the screen size from UIScreen
		CGRect screenBounds = [UIScreen mainScreen].bounds;
		CGFloat scale = [UIScreen mainScreen].nativeScale;

		int fb_width = static_cast<int>(screenBounds.size.width * scale);
		int fb_height = static_cast<int>(screenBounds.size.height * scale);

		glViewport(0, 0, fb_width, fb_height);

		GraphicsGL::get().reinit();

		return Error::Code::NONE;
	}

	bool Window::not_closed() const
	{
		return !closed;
	}

	void Window::update()
	{
		updateopc();
	}

	void Window::updateopc()
	{
		if (opcstep != 0.0f)
		{
			opacity += opcstep;

			if (opacity >= 1.0f)
			{
				opacity = 1.0f;
				opcstep = 0.0f;
			}
			else if (opacity <= 0.0f)
			{
				opacity = 0.0f;
				opcstep = -opcstep;

				fadeprocedure();
			}
		}
	}

	void Window::check_events()
	{
		// On iOS, events are delivered via the responder chain (UITouch)
		// No polling needed — this is a no-op

		int16_t new_width = Constants::Constants::get().get_physicalwidth();
		int16_t new_height = Constants::Constants::get().get_physicalheight();

		if (width != new_width || height != new_height)
		{
			width = new_width;
			height = new_height;
			initwindow();
		}
	}

	void Window::begin() const
	{
		GraphicsGL::get().clearscene();
	}

	void Window::end() const
	{
		GraphicsGL::get().flush(opacity);
		// GLKView handles presentRenderbuffer automatically
	}

	void Window::fadeout(float step, std::function<void()> fadeproc)
	{
		opcstep = -step;
		fadeprocedure = fadeproc;
	}

	void Window::setclipboard(const std::string& text) const
	{
		NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
		[UIPasteboard generalPasteboard].string = nsText;
	}

	std::string Window::getclipboard() const
	{
		NSString* text = [UIPasteboard generalPasteboard].string;
		return text ? std::string([text UTF8String]) : "";
	}

	void Window::toggle_fullscreen()
	{
		// iOS is always fullscreen — no-op
	}
}
