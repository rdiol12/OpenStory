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
#include "Window.h"

#include "UI.h"
#include "Gamepad.h"

#include "../Configuration.h"
#include "../Constants.h"
#include "../Timer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <Windows.h>
#include <ShlObj.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <chrono>
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

	void error_callback(int, const char*)
	{
	}

	void key_callback(GLFWwindow*, int key, int, int action, int)
	{
		if (action == GLFW_REPEAT && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER))
			return;

		UI::get().send_key(key, action != GLFW_RELEASE);
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

	// Viewport parameters for cursor mapping (set in initwindow)
	// s_vp_y_top is the top offset in screen coords (GLFW uses top-left origin)
	static int s_vp_x = 0, s_vp_y_top = 0, s_vp_w = 1920, s_vp_h = 1080;

	void cursor_callback(GLFWwindow*, double xpos, double ypos)
	{
		// Map screen coordinates to game logical coordinates
		// Account for viewport offset (letterbox/pillarbox) and scaling
		int vw = Constants::Constants::get().get_viewwidth();
		int vh = Constants::Constants::get().get_viewheight();

		int16_t x = static_cast<int16_t>((xpos - s_vp_x) * vw / s_vp_w);
		int16_t y = static_cast<int16_t>((ypos - s_vp_y_top) * vh / s_vp_h);
		Point<int16_t> pos = Point<int16_t>(x, y);
		UI::get().send_cursor(pos);
	}

	void clip_cursor_to_window(GLFWwindow* window)
	{
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
	}

	void focus_callback(GLFWwindow* window, int focused)
	{
		if (focused)
			clip_cursor_to_window(window);
		else
			ClipCursor(nullptr);

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

	Error Window::init()
	{
		fullscreen = Setting<Fullscreen>::get().load();

		if (!glfwInit())
			return Error::Code::GLFW;

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

		bool vsync = Setting<VSync>::get().load();
		glfwSwapInterval(vsync ? 1 : 0);

		int fb_width, fb_height;
		glfwGetFramebufferSize(glwnd, &fb_width, &fb_height);

		// Stretch to fill entire screen — no black bars
		s_vp_x = 0;
		s_vp_y_top = 0;
		s_vp_w = fb_width;
		s_vp_h = fb_height;
		glViewport(0, 0, fb_width, fb_height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glfwSetInputMode(glwnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

		double xpos, ypos;

		glfwGetCursorPos(glwnd, &xpos, &ypos);
		cursor_callback(glwnd, xpos, ypos);

		glfwSetInputMode(glwnd, GLFW_STICKY_KEYS, GL_TRUE);
		glfwSetKeyCallback(glwnd, key_callback);
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
		GetCurrentDirectoryA(256, buf);
		strcat_s(buf, sizeof(buf), "\\Icon.png");

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

		SystemParametersInfoA(SPI_SETMOUSESPEED, 0,
			reinterpret_cast<PVOID>(static_cast<INT_PTR>(sys_speed)), 0);
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
		std::tm tm_local;
		localtime_s(&tm_local, &t);

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