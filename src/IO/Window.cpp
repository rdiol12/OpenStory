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
#include "../../platform/shared/PlatformConfig.h"

#include "Window.h"

#include <algorithm>
#include <iostream>

#include "UI.h"
#include "Gamepad.h"

#include "../Configuration.h"
#include "../Constants.h"
#include "../Timer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef _WIN32
#include <Windows.h>
// ShlObj.h is included for historical reasons only -- nothing in this
// translation unit calls an SHxxx/known-folder API. The screenshot directory
// comes from Setting<ScreenshotFolder> and is resolved relative to the working
// directory, which needs no platform header.
#include <ShlObj.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ms
{
	Window::Window()
	{
		context = nullptr;
		glwnd = nullptr;
		opacity = 1.0f;
		opcstep = 0.0f;
		width = Constants::Constants::get().get_physicalwidth();
		height = Constants::Constants::get().get_physicalheight();
	}

	Window::~Window()
	{
		glfwTerminate();
	}

	// Constants defaults to a 1920x1080 window. That is larger than the usable
	// area of plenty of displays -- a 14" MacBook Pro is 1512x982 points -- and a
	// window bigger than the screen gets created but never composited, so the
	// client appears to start with no visible window at all. The existing
	// oversize handling in check_events() cannot catch this: it only runs when
	// the size *changes*, and at startup width already equals physicalwidth.
	//
	// Shrink to the largest standard size that fits the monitor work area, and
	// settle the resolution for the whole session so the logical view never
	// changes underneath the UI (see the comment on Setting<Width> below).
	static void fit_window_to_monitor()
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		if (!monitor)
			return;

		int wx = 0, wy = 0, ww = 0, wh = 0;
		glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);

		if (ww <= 0 || wh <= 0)
			return;

		// Leave room for the title bar and a little breathing space.
		const int avail_w = ww - 32;
		const int avail_h = wh - 64;

		// Start from the saved setting rather than the Constants default,
		// because SetFieldHandler re-reads Setting<Width>/<Height> on every map
		// load and pushes them back into Constants. Resolving both to the same
		// numbers here means the logical view never changes mid-session, so no
		// UI element is left laid out for a size that no longer applies.
		int phys_w = Setting<Width>::get().load();
		int phys_h = Setting<Height>::get().load();

		if (phys_w <= 0 || phys_h <= 0)
		{
			phys_w = Constants::Constants::get().get_physicalwidth();
			phys_h = Constants::Constants::get().get_physicalheight();
		}

		if (phys_w > avail_w || phys_h > avail_h)
		{
			static const int candidates[][2] = {
				{ 1920, 1080 }, { 1600, 900 }, { 1440, 900 }, { 1366, 768 },
				{ 1280, 720 }, { 1024, 768 }, { 800, 600 },
			};

			phys_w = avail_w;
			phys_h = avail_h;

			for (const auto& candidate : candidates)
			{
				if (candidate[0] <= avail_w && candidate[1] <= avail_h)
				{
					phys_w = candidate[0];
					phys_h = candidate[1];
					break;
				}
			}
		}

		// Mirror SetFieldHandler's formula exactly so the logical view it
		// computes on a map load matches the one used here.
		float ui_scale = phys_w / 1280.0f;
		ui_scale = std::max(1.0f, std::min(ui_scale, 4.0f));

		Constants::Constants::get().set_ui_scale(ui_scale);
		Constants::Constants::get().set_viewwidth(static_cast<int16_t>(phys_w));
		Constants::Constants::get().set_viewheight(static_cast<int16_t>(phys_h));

		// Written back so the map-load path reads these and not a stale size.
		Setting<Width>::get().save(static_cast<uint16_t>(phys_w));
		Setting<Height>::get().save(static_cast<uint16_t>(phys_h));

		std::cout << "[Init] Window " << phys_w << "x" << phys_h
		          << " (monitor work area " << ww << "x" << wh << "), UI scale "
		          << ui_scale << ", logical view "
		          << Constants::Constants::get().get_viewwidth() << "x"
		          << Constants::Constants::get().get_viewheight() << std::endl;
	}

	void error_callback(int, const char*)
	{
	}

	void key_callback(GLFWwindow*, int key, int, int action, int)
	{
		if (action == GLFW_REPEAT && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER))
			return;

		UI::get().send_key(key, action != GLFW_RELEASE);
	}

	// Text as the OS produced it, after the active keyboard layout and any IME.
	// key_callback only reports physical keys, so on a Hebrew layout it reports
	// the Latin key at that position -- this is the only path that can deliver
	// non-ASCII characters at all.
	void char_callback(GLFWwindow*, unsigned int codepoint)
	{
		UI::get().send_char(codepoint);
	}

	std::chrono::time_point<std::chrono::steady_clock> start = ContinuousTimer::get().start();

	void mousekey_callback(GLFWwindow*, int button, int action, int)
	{
		switch (button)
		{
		case GLFW_MOUSE_BUTTON_LEFT:
			switch (action)
			{
			case GLFW_PRESS:
				UI::get().send_cursor(true);
				break;
			case GLFW_RELEASE:
			{
				auto diff_ms = ContinuousTimer::get().stop(start) / 1000;
				start = ContinuousTimer::get().start();

				if (diff_ms > 10 && diff_ms < 200)
					UI::get().doubleclick();

				UI::get().send_cursor(false);
			}
			break;
			}

			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			switch (action)
			{
			case GLFW_PRESS:
				UI::get().rightclick();
				break;
			}

			break;
		}
	}

	// Viewport parameters for cursor mapping (set in initwindow).
	// s_vp_y_top is the top offset in screen coords (GLFW uses top-left origin).
	// IMPORTANT: these are in LOGICAL WINDOW coordinates, not framebuffer
	// pixels. glfwGetCursorPos reports logical window coordinates, and on a
	// HiDPI/Retina display the framebuffer is larger than the window (2x on a
	// Mac), so mapping cursor input against framebuffer pixels would halve
	// every position. On Windows the two are identical, so this is a no-op
	// there.
	static int s_vp_x = 0, s_vp_y_top = 0, s_vp_w = 1920, s_vp_h = 1080;

	void cursor_callback(GLFWwindow*, double xpos, double ypos)
	{
		// Map screen coordinates to game logical coordinates
		// Account for viewport offset (letterbox/pillarbox) and scaling
		int vw = Constants::Constants::get().get_viewwidth();
		int vh = Constants::Constants::get().get_viewheight();

		if (s_vp_w <= 0 || s_vp_h <= 0)
			return;

		int16_t x = static_cast<int16_t>((xpos - s_vp_x) * vw / s_vp_w);
		int16_t y = static_cast<int16_t>((ypos - s_vp_y_top) * vh / s_vp_h);
		Point<int16_t> pos = Point<int16_t>(x, y);
		UI::get().send_cursor(pos);
	}

#ifndef _WIN32
	// The game draws its own cursor sprite, so the OS pointer has to stay
	// invisible. GLFW's cursor modes are mutually exclusive and
	// GLFW_CURSOR_CAPTURED (used to confine the pointer below) forces the
	// cursor visible, so the hiding is done with a fully transparent cursor
	// image instead, which applies in every mode.
	static GLFWcursor* s_blank_cursor = nullptr;

	void apply_blank_cursor(GLFWwindow* window)
	{
		if (!s_blank_cursor)
		{
			unsigned char pixels[4] = { 0, 0, 0, 0 };

			GLFWimage image;
			image.width = 1;
			image.height = 1;
			image.pixels = pixels;

			s_blank_cursor = glfwCreateCursor(&image, 0, 0);
		}

		if (s_blank_cursor)
			glfwSetCursor(window, s_blank_cursor);
	}
#endif

	// Confine the cursor to the window content area.
	void clip_cursor_to_window(GLFWwindow* window)
	{
		if (!window)
			return;

#ifdef _WIN32
		// The vendored Windows GLFW is 3.3.2, which predates
		// GLFW_CURSOR_CAPTURED -- keep the native Win32 implementation.
		HWND hwnd = glfwGetWin32Window(window);
		if (hwnd)
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			POINT tl = { rect.left, rect.top };
			POINT br = { rect.right, rect.bottom };
			ClientToScreen(hwnd, &tl);
			ClientToScreen(hwnd, &br);
			RECT screen_rect = { tl.x, tl.y, br.x, br.y };
			ClipCursor(&screen_rect);
		}
#elif defined(GLFW_CURSOR_CAPTURED)
		// Portable GLFW 3.4+ path (Homebrew GLFW here is 3.5.1).
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
#endif
	}

	void release_cursor(GLFWwindow* window)
	{
		if (!window)
			return;

#ifdef _WIN32
		ClipCursor(nullptr);
#elif defined(GLFW_CURSOR_CAPTURED)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif
	}

	void focus_callback(GLFWwindow* window, int focused)
	{
		if (focused)
			clip_cursor_to_window(window);
		else
			release_cursor(window);

		UI::get().send_focus(focused);
	}

	void scroll_callback(GLFWwindow*, double xoffset, double yoffset)
	{
		UI::get().send_scroll(yoffset);
	}

	void close_callback(GLFWwindow* window)
	{
		UI::get().send_close();

		glfwSetWindowShouldClose(window, GL_FALSE);
	}

	// Apple only ships a 4.1 Core profile; without these hints GLFW hands back
	// a legacy 2.1 context, which has no glGenVertexArrays and would break the
	// renderer. Must be applied before EVERY glfwCreateWindow in this file
	// (including the hidden 1x1 sharing context), or context sharing fails.
	// Deliberately a no-op everywhere else so Windows behaviour is unchanged.
	void apply_gl_context_hints()
	{
#ifdef PLATFORM_MACOS
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_CONTEXT_VERSION_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_CONTEXT_VERSION_MINOR);
#ifdef GL_CORE_PROFILE
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	}

	Error Window::init()
	{
		fullscreen = Setting<Fullscreen>::get().load();

		if (!glfwInit())
			return Error::Code::GLFW;

		fit_window_to_monitor();

		// The constructor cached these before the monitor was known.
		width = Constants::Constants::get().get_physicalwidth();
		height = Constants::Constants::get().get_physicalheight();

		apply_gl_context_hints();

		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
		context = glfwCreateWindow(1, 1, "", nullptr, nullptr);
		glfwMakeContextCurrent(context);
		glfwSetErrorCallback(error_callback);
		glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		if (Error error = GraphicsGL::get().init())
			return error;

		return initwindow();
	}

	Error Window::initwindow()
	{
		if (glwnd)
			glfwDestroyWindow(glwnd);

		int wnd_width = width;
		int wnd_height = height;

		if (fullscreen)
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (mode)
			{
				wnd_width = mode->width;
				wnd_height = mode->height;
			}
			glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		}
		else
		{
			glfwWindowHint(GLFW_DECORATED, GL_TRUE);
		}

		apply_gl_context_hints();

		glwnd = glfwCreateWindow(
			wnd_width,
			wnd_height,
			Configuration::get().get_title().c_str(),
			nullptr,
			context
		);

		if (!glwnd)
			return Error::Code::WINDOW;

		if (fullscreen)
			glfwSetWindowPos(glwnd, 0, 0);

		glfwMakeContextCurrent(glwnd);

		// GraphicsGL::init() ran against the hidden 1x1 context. Its VAO does
		// not exist on this one, so bind a VAO owned by this context before any
		// drawing happens.
		GraphicsGL::get().bind_context_vao();

		bool vsync = Setting<VSync>::get().load();
		glfwSwapInterval(vsync ? 1 : 0);

		// glViewport wants framebuffer PIXELS.
		int fb_width, fb_height;
		glfwGetFramebufferSize(glwnd, &fb_width, &fb_height);

		// Cursor mapping wants LOGICAL WINDOW coordinates, which is what
		// glfwGetCursorPos reports. These are equal on Windows but differ by
		// the content scale on a Retina display (framebuffer is 2x window).
		int win_width = fb_width;
		int win_height = fb_height;
		glfwGetWindowSize(glwnd, &win_width, &win_height);

		if (win_width <= 0 || win_height <= 0)
		{
			win_width = fb_width;
			win_height = fb_height;
		}

		// Stretch to fill entire screen — no black bars
		s_vp_x = 0;
		s_vp_y_top = 0;
		s_vp_w = win_width;
		s_vp_h = win_height;
		glViewport(0, 0, fb_width, fb_height);
#ifndef PLATFORM_MACOS
		// Fixed-function matrix stack; removed in a 4.1 Core profile.
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
#endif

#ifdef _WIN32
		glfwSetInputMode(glwnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#else
		apply_blank_cursor(glwnd);
#endif

		double xpos, ypos;

		glfwGetCursorPos(glwnd, &xpos, &ypos);
		cursor_callback(glwnd, xpos, ypos);

		glfwSetInputMode(glwnd, GLFW_STICKY_KEYS, GL_TRUE);
		glfwSetKeyCallback(glwnd, key_callback);
		glfwSetCharCallback(glwnd, char_callback);
		glfwSetMouseButtonCallback(glwnd, mousekey_callback);
		glfwSetCursorPosCallback(glwnd, cursor_callback);
		glfwSetWindowFocusCallback(glwnd, focus_callback);
		glfwSetScrollCallback(glwnd, scroll_callback);
		glfwSetWindowCloseCallback(glwnd, close_callback);

		// Confine cursor to window so it can't escape to desktop/taskbar
		clip_cursor_to_window(glwnd);

		// Apply saved mouse speed (SystemParametersInfo SPI_SETMOUSESPEED).
		apply_mouse_speed();

		char buf[256];
#ifdef _WIN32
		GetCurrentDirectoryA(256, buf);
		strcat_s(buf, sizeof(buf), "\\Icon.png");
#else
		std::error_code cwd_ec;
		std::filesystem::path icon = std::filesystem::current_path(cwd_ec) / "Icon.png";
		std::string icon_str = icon.string();

		std::snprintf(buf, sizeof(buf), "%s", icon_str.c_str());
#endif

		GLFWimage images[1];

		auto stbi = stbi_load(buf, &images[0].width, &images[0].height, 0, 4);

		if (stbi != NULL){
				// return Error(Error::Code::MISSING_ICON, stbi_failure_reason());
			images[0].pixels = stbi;

			glfwSetWindowIcon(glwnd, 1, images);
			stbi_image_free(images[0].pixels);
		}

		GraphicsGL::get().reinit();

		return Error::Code::NONE;
	}

	bool Window::not_closed() const
	{
		return glfwWindowShouldClose(glwnd) == 0;
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
		int16_t max_width = Configuration::get().get_max_width();
		int16_t max_height = Configuration::get().get_max_height();
		int16_t new_width = Constants::Constants::get().get_physicalwidth();
		int16_t new_height = Constants::Constants::get().get_physicalheight();

		if (width != new_width || height != new_height)
		{
			width = new_width;
			height = new_height;

			if (new_width >= max_width || new_height >= max_height)
				fullscreen = true;

			initwindow();
		}

		glfwPollEvents();

		Gamepad::get().poll();
	}

	void Window::begin() const
	{
		GraphicsGL::get().clearscene();
	}

	void Window::end() const
	{
		GraphicsGL::get().flush(opacity);
		glfwSwapBuffers(glwnd);
	}

	void Window::fadeout(float step, std::function<void()> fadeproc)
	{
		opcstep = -step;
		fadeprocedure = fadeproc;
	}

	void Window::setclipboard(const std::string& text) const
	{
		glfwSetClipboardString(glwnd, text.c_str());
	}

	std::string Window::getclipboard() const
	{
		const char* text = glfwGetClipboardString(glwnd);

		return text ? text : "";
	}

	void Window::apply_mouse_speed(int slider_value)
	{
		// Slider stored 0..100. Windows SPI_SETMOUSESPEED takes 1..20 (10 = default).
		if (slider_value < 0)
			slider_value = Setting<MouseSpeed>::get().load();

		if (slider_value < 0) slider_value = 0;
		if (slider_value > 100) slider_value = 100;

		// Map 0..100 -> 1..20 linearly, rounding to nearest.
		int sys_speed = 1 + static_cast<int>(((slider_value * 19) + 50) / 100);
		if (sys_speed < 1) sys_speed = 1;
		if (sys_speed > 20) sys_speed = 20;

#ifdef _WIN32
		SystemParametersInfoA(SPI_SETMOUSESPEED, 0,
			reinterpret_cast<PVOID>(static_cast<INT_PTR>(sys_speed)), 0);
#else
		// macOS has no public API to set the system pointer speed (it lives
		// behind IOHIDSystem/defaults write com.apple.mouse.scaling and needs
		// a logout to take effect), so the setting is a no-op here.
		(void)sys_speed;
#endif
	}

	void Window::take_screenshot()
	{
		if (!glwnd)
			return;

		int fb_w = 0, fb_h = 0;
		glfwGetFramebufferSize(glwnd, &fb_w, &fb_h);
		if (fb_w <= 0 || fb_h <= 0)
			return;

		// Read RGBA pixels from the default framebuffer.
		std::vector<uint8_t> pixels(static_cast<size_t>(fb_w) * fb_h * 4);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		// OpenGL origin is bottom-left; PNG expects top-left — flip vertically.
		std::vector<uint8_t> flipped(pixels.size());
		size_t row_bytes = static_cast<size_t>(fb_w) * 4;
		for (int y = 0; y < fb_h; ++y)
		{
			std::memcpy(
				flipped.data() + (fb_h - 1 - y) * row_bytes,
				pixels.data() + y * row_bytes,
				row_bytes);
		}

		// Resolve output folder (from Setting<ScreenshotFolder>; create if missing).
		std::string folder = Setting<ScreenshotFolder>::get().load();
		if (folder.empty())
			folder = "screenshots";

		std::error_code ec;
		std::filesystem::create_directories(folder, ec);

		// Timestamped filename: maple_YYYYMMDD_HHMMSS.png
		auto now = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm_local{};
#ifdef _WIN32
		if (localtime_s(&tm_local, &t) != 0)
			return;
#else
		// POSIX equivalent: arguments reversed, returns nullptr on failure.
		if (localtime_r(&t, &tm_local) == nullptr)
			return;
#endif

		std::ostringstream name;
		name << "maple_"
			<< std::put_time(&tm_local, "%Y%m%d_%H%M%S")
			<< ".png";

		std::filesystem::path out = std::filesystem::path(folder) / name.str();
		stbi_write_png(out.string().c_str(), fb_w, fb_h, 4, flipped.data(),
			static_cast<int>(row_bytes));
	}

	void Window::toggle_fullscreen()
	{
		int16_t max_width = Configuration::get().get_max_width();
		int16_t max_height = Configuration::get().get_max_height();

		if (width < max_width && height < max_height)
		{
			fullscreen = !fullscreen;
			Setting<Fullscreen>::get().save(fullscreen);

			initwindow();
			glfwPollEvents();
		}
	}
}