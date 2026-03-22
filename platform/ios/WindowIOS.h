//////////////////////////////////////////////////////////////////////////////////
//	OpenStory2-iOS — Window (iOS Implementation)
//	Replaces the GLFW-based Window class for iOS.
//	The GL context is managed by GLKViewController, so most Window operations
//	are simplified or become no-ops.
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Error.h"
#include "Template/Singleton.h"
#include "Graphics/GraphicsGL.h"

#include <functional>
#include <string>

namespace ms
{
	class Window : public Singleton<Window>
	{
	public:
		Window();
		~Window();

		Error init();
		Error initwindow();

		bool not_closed() const;
		void update();
		void begin() const;
		void end() const;
		void fadeout(float step, std::function<void()> fadeprocedure);
		void check_events();

		void setclipboard(const std::string& text) const;
		std::string getclipboard() const;

		void toggle_fullscreen();

	private:
		void updateopc();

		bool closed;
		bool fullscreen;
		float opacity;
		float opcstep;
		std::function<void()> fadeprocedure;
		int16_t width;
		int16_t height;
	};
}
