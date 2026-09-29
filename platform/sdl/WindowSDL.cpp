// SDL input/audio are independent of video. On PS5 EGL alone owns the display.
#include "IO/Window.h"
#include "IO/UI.h"
#include "IO/Gamepad.h"
#include "ConsoleMenu.h"
#include "Configuration.h"
#include "Constants.h"
#include "Util/Paths.h"
#include "Util/CrashLog.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <codecvt>
#include <locale>
#include <cstdio>
#include <cstdint>
#include <ctime>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#ifdef PLATFORM_PS5
#include "../ps5/Keyboard.h"
#include <sys/stat.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifdef OPENSTORY_LAN_LOG
#include "../shared/ColorProbe.h"
#endif
#else
#include <filesystem>
#endif

#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_phase(const char*, uintptr_t);
#endif

namespace
{
    void window_phase(const char* label, uintptr_t value = 0)
    {
#ifdef OPENSTORY_LAN_LOG
        openstory_diagnostics_phase(label, value);
#endif
    }
    SDL_GameController* controller = nullptr;
    bool keyboard_visible = false;
    bool gamepad_blocked = false;
    bool trigger_left = false, trigger_right = false;
    std::array<bool, SDL_CONTROLLER_BUTTON_MAX> previous{};
    int keyboard_cell = 0;
    float cursor_x = 640, cursor_y = 360;
    Uint32 cursor_time = 0;
    Uint32 last_click_time = 0;
    float last_click_x = 0, last_click_y = 0;
    constexpr int buttons[] = {SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_GUIDE, SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT};
    // Printable ASCII plus a final Enter key; passwords are never copied here.
    constexpr int keyboard_columns = 12, keyboard_count = 96;
    int display_width = 1920, display_height = 1080;
#ifdef PLATFORM_PS5
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
#else
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
#endif

    int keycode(SDL_Keycode key)
    {
        if (key >= SDLK_a && key <= SDLK_z) return key - SDLK_a + GLFW_KEY_A;
        if (key >= SDLK_SPACE && key <= SDLK_BACKQUOTE) return static_cast<int>(key);
        if (key >= SDLK_F1 && key <= SDLK_F12) return key - SDLK_F1 + GLFW_KEY_F1;
        switch (key)
        {
        case SDLK_RETURN: return GLFW_KEY_ENTER;
        case SDLK_KP_ENTER: return GLFW_KEY_KP_ENTER;
        case SDLK_ESCAPE: return GLFW_KEY_ESCAPE;
        case SDLK_TAB: return GLFW_KEY_TAB;
        case SDLK_BACKSPACE: return GLFW_KEY_BACKSPACE;
        case SDLK_DELETE: return GLFW_KEY_DELETE;
        case SDLK_INSERT: return GLFW_KEY_INSERT;
        case SDLK_LEFT: return GLFW_KEY_LEFT;
        case SDLK_RIGHT: return GLFW_KEY_RIGHT;
        case SDLK_UP: return GLFW_KEY_UP;
        case SDLK_DOWN: return GLFW_KEY_DOWN;
        case SDLK_HOME: return GLFW_KEY_HOME;
        case SDLK_END: return GLFW_KEY_END;
        case SDLK_PAGEUP: return GLFW_KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return GLFW_KEY_PAGE_DOWN;
        case SDLK_LSHIFT: return GLFW_KEY_LEFT_SHIFT;
        case SDLK_RSHIFT: return GLFW_KEY_RIGHT_SHIFT;
        case SDLK_LCTRL: return GLFW_KEY_LEFT_CONTROL;
        case SDLK_RCTRL: return GLFW_KEY_RIGHT_CONTROL;
        case SDLK_LALT: return GLFW_KEY_LEFT_ALT;
        case SDLK_RALT: return GLFW_KEY_RIGHT_ALT;
        default: return GLFW_KEY_UNKNOWN;
        }
    }

    void tap(int key)
    {
        ms::UI::get().send_key(key, true);
        ms::UI::get().send_key(key, false);
    }

    bool double_click(Uint32 now, float x, float y)
    {
        bool twice = last_click_time && now - last_click_time <= 350 &&
            std::abs(x - last_click_x) <= 6 && std::abs(y - last_click_y) <= 6;
        last_click_time = twice ? 0 : now;
        last_click_x = x;
        last_click_y = y;
        return twice;
    }

    void poll_controller()
    {
        if (controller && !SDL_GameControllerGetAttached(controller)) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
            previous.fill(false);
            trigger_left = trigger_right = false;
            keyboard_visible = false;
            gamepad_blocked = true;
        }
        if (!controller)
            for (int i = 0; i < SDL_NumJoysticks(); ++i)
                if (SDL_IsGameController(i) && (controller = SDL_GameControllerOpen(i))) break;
        if (!controller) return;
        std::array<bool, SDL_CONTROLLER_BUTTON_MAX> down{};
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
            down[i] = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(i));
        auto pressed = [&](SDL_GameControllerButton id) { return down[id] && !previous[id]; };
        auto axis = [](SDL_GameControllerAxis id) { return SDL_GameControllerGetAxis(controller, id); };
        const bool left = axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000;
        const bool right = axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000;
#ifdef PLATFORM_PS5
        if (ms::native_keyboard::busy()) { previous = down; return; }
#endif
        const bool was_keyboard = keyboard_visible;
        const bool menu = ms::UI::get().controller_menu();
        const Uint32 now = SDL_GetTicks();
        static int last_direction = 0;
        static Uint32 repeat_at = 0;
        const int direction = down[SDL_CONTROLLER_BUTTON_DPAD_LEFT] || axis(SDL_CONTROLLER_AXIS_LEFTX) < -16000 ? GLFW_KEY_LEFT :
            down[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] || axis(SDL_CONTROLLER_AXIS_LEFTX) > 16000 ? GLFW_KEY_RIGHT :
            down[SDL_CONTROLLER_BUTTON_DPAD_UP] || axis(SDL_CONTROLLER_AXIS_LEFTY) < -16000 ? GLFW_KEY_UP :
            down[SDL_CONTROLLER_BUTTON_DPAD_DOWN] || axis(SDL_CONTROLLER_AXIS_LEFTY) > 16000 ? GLFW_KEY_DOWN : 0;
        bool navigate = direction && (direction != last_direction || static_cast<Sint32>(now-repeat_at) >= 0);
        if (navigate) repeat_at = now + (direction != last_direction ? 400 : 120);
        last_direction = direction;
        if (gamepad_blocked && !keyboard_visible) {
            bool held = left || right || direction;
            for (bool value : down) held = held || value;
            if (!held) gamepad_blocked = false;
        } else if (keyboard_visible) {
            if (navigate) {
                keyboard_cell += direction == GLFW_KEY_LEFT ? -1 : direction == GLFW_KEY_RIGHT ? 1 :
                    direction == GLFW_KEY_UP ? -keyboard_columns : keyboard_columns;
                keyboard_cell = (keyboard_cell + keyboard_count) % keyboard_count;
            }
            if (pressed(SDL_CONTROLLER_BUTTON_A)) {
                if (keyboard_cell == 95) { tap(GLFW_KEY_ENTER); keyboard_visible = false; }
                else ms::UI::get().send_char(32 + keyboard_cell);
            }
            if (pressed(SDL_CONTROLLER_BUTTON_X)) tap(GLFW_KEY_BACKSPACE);
            if (pressed(SDL_CONTROLLER_BUTTON_Y)) tap(GLFW_KEY_TAB);
            if (pressed(SDL_CONTROLLER_BUTTON_B) || pressed(SDL_CONTROLLER_BUTTON_START)) keyboard_visible = false;
        } else if (menu) {
            if (navigate) ms::UI::get().controller_input(direction, now);
            if (pressed(SDL_CONTROLLER_BUTTON_A)) ms::UI::get().controller_input(GLFW_KEY_ENTER, now);
            if (pressed(SDL_CONTROLLER_BUTTON_B)) ms::UI::get().controller_input(GLFW_KEY_ESCAPE, now);
            if (pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) ms::UI::get().controller_cycle_window();
            if (pressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) ms::UI::get().controller_input(GLFW_KEY_TAB, now);
            if (pressed(SDL_CONTROLLER_BUTTON_Y) && ms::UI::get().has_textfield()) ms::Window::get().show_keyboard();
            if (left && !trigger_left) ms::UI::get().controller_input(GLFW_KEY_PAGE_UP, now);
            if (right && !trigger_right) ms::UI::get().controller_input(GLFW_KEY_PAGE_DOWN, now);
        } else {
            if (pressed(SDL_CONTROLLER_BUTTON_START)) ms::UI::get().emplace<ms::UIControllerMenu>();
            if (pressed(SDL_CONTROLLER_BUTTON_TOUCHPAD)) ms::UI::get().controller_open(ms::KeyAction::INTERACT_HARVEST);
            if (left && !trigger_left) tap(GLFW_KEY_Z);
            if (right && !trigger_right) tap(GLFW_KEY_X);
        }
        if ((was_keyboard && !keyboard_visible) || (menu && !ms::UI::get().controller_menu()))
            gamepad_blocked = true;
        previous = down;
        trigger_left = left;
        trigger_right = right;
    }

    void draw_keyboard()
    {
#ifdef PLATFORM_PS5
        if (ms::native_keyboard::busy()) return;
#endif
        using namespace ms;
        auto& graphics = GraphicsGL::get();
        auto& dimensions = Constants::Constants::get();
        if (!keyboard_visible) {
            if (!controller || !UI::get().controller_menu()) return;
            if (auto selector = UI::get().get_element<UIServerSelect>()) if (selector->is_active()) return;
            const int y = dimensions.get_viewheight() - 35;
            graphics.drawrectangle(0, y, dimensions.get_viewwidth(), 35, .03f,.03f,.03f,.95f);
            Text(Text::A13M, Text::CENTER, Color::Name::WHITE,
                "D-pad: select   Cross: choose   Circle: close   R1: menu   L1: tab   L2/R2: scroll").draw(
                Point<int16_t>(dimensions.get_viewwidth()/2, y+10));
            return;
        }
        const int x = (dimensions.get_viewwidth() - 600) / 2;
        const int y = std::max(0, dimensions.get_viewheight() - 330);
        graphics.drawrectangle(x, y, 600, 330, 0.05f, 0.06f, 0.09f, 1);
        Text title(Text::A12M, Text::LEFT, Color::Name::WHITE,
            "D-pad: choose   Cross: type   Square: erase   Triangle: next field   Circle: done");
        title.draw(DrawArgument(Point<int16_t>(x + 10, y + 12)));
        for (int i = 0; i < keyboard_count; ++i)
        {
            const int cx = x + (i % keyboard_columns) * 49 + 6;
            const int cy = y + (i / keyboard_columns) * 34 + 45;
            if (i == keyboard_cell) graphics.drawrectangle(cx, cy, 46, 31, 0.15f, 0.4f, 0.8f, 1);
            std::string label = i == 95 ? "Enter" : i == 0 ? "Space" : std::string(1, char(32 + i));
            Text text(Text::A12M, Text::LEFT, Color::Name::WHITE, label);
            text.draw(DrawArgument(Point<int16_t>(cx + 5, cy + 7)));
        }
    }
}

// Only the joystick portion of GLFW is retained by the existing Gamepad class.
extern "C" int glfwJoystickPresent(int) { return controller != nullptr; }
extern "C" const char* glfwGetJoystickName(int) { return controller ? SDL_GameControllerName(controller) : nullptr; }
extern "C" int glfwJoystickIsGamepad(int) { return GLFW_TRUE; }
extern "C" int glfwGetGamepadState(int, GLFWgamepadstate* state)
{
    *state = {};
#ifdef PLATFORM_PS5
    if (ms::native_keyboard::busy()) return GLFW_TRUE;
#endif
    if (!controller || keyboard_visible || gamepad_blocked || ms::UI::get().controller_menu()) return GLFW_TRUE; // releases held game keys
    for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
        state->buttons[i] = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(buttons[i]));
    state->axes[GLFW_GAMEPAD_AXIS_LEFT_X] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32768.f;
    state->axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / 32768.f;
    return GLFW_TRUE;
}
extern "C" const unsigned char* glfwGetJoystickButtons(int, int* count) { *count = 0; return nullptr; }
extern "C" const float* glfwGetJoystickAxes(int, int* count) { *count = 0; return nullptr; }

bool check_sdl_input()
{
    // A test window may remain unfocused under a desktop compositor.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    bool clicks = !double_click(100, 20, 20) && double_click(300, 20, 20) &&
        !double_click(320, 20, 20) && !double_click(330, 80, 20) && !double_click(900, 80, 20);
    last_click_time = 0;
    if (!clicks) return false;
    if (keycode(SDLK_a) != GLFW_KEY_A || keycode(SDLK_RETURN) != GLFW_KEY_ENTER ||
        keycode(SDLK_LEFT) != GLFW_KEY_LEFT || keycode(SDLK_F12) != GLFW_KEY_F12 ||
        keycode(SDLK_UNKNOWN) != GLFW_KEY_UNKNOWN) return false;
    int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
    if (device < 0 || !(controller = SDL_GameControllerOpen(device))) {
        std::fprintf(stderr, "Virtual controller: %s\n", SDL_GetError());
        return false;
    }
    auto* joystick = SDL_GameControllerGetJoystick(controller);
    bool passed = true;
    for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
    {
        SDL_JoystickSetVirtualButton(joystick, buttons[i], 1);
        SDL_JoystickUpdate();
        GLFWgamepadstate state;
        glfwGetGamepadState(0, &state);
        for (int j = 0; j <= GLFW_GAMEPAD_BUTTON_LAST; ++j) {
            if (state.buttons[j] != (j == i)) {
                std::fprintf(stderr, "Controller button %d: expected %d, got %d\n", j, j == i, state.buttons[j]);
                passed = false;
            }
        }
        SDL_JoystickSetVirtualButton(joystick, buttons[i], 0);
    }
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, -32768);
    SDL_JoystickUpdate();
    GLFWgamepadstate state;
    glfwGetGamepadState(0, &state);
    passed = passed && state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] == -1;
    if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] != -1) std::fprintf(stderr, "Virtual left axis: %f\n", state.axes[0]);
    keyboard_visible = true;
    glfwGetGamepadState(0, &state);
    passed = passed && state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] == 0;
    keyboard_visible = false;
    gamepad_blocked = true;
    glfwGetGamepadState(0, &state);
    passed = passed && state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] == 0;
    gamepad_blocked = false;
    SDL_GameControllerClose(controller);
    controller = nullptr;
    SDL_JoystickDetachVirtual(device);
    return passed;
}

namespace ms
{
    void Window::show_keyboard() {
#ifdef PLATFORM_PS5
        if (native_keyboard::open()) { keyboard_visible = false; gamepad_blocked = true; return; }
#endif
        keyboard_visible = true;
    }

    Window::Window() : closed(false), fullscreen(true), opacity(1), opcstep(0), width(1920), height(1080) {}
    Window::~Window()
    {
#ifdef PLATFORM_PS5
        native_keyboard::shutdown();
#endif
        openstory_diagnostics_checkpoint("window-destroy-controller");
        if (controller) SDL_GameControllerClose(controller);
#ifdef PLATFORM_PS5
        if (display != EGL_NO_DISPLAY)
        {
            openstory_diagnostics_checkpoint("window-release-context");
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            openstory_diagnostics_checkpoint("window-destroy-context");
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            openstory_diagnostics_checkpoint("window-destroy-surface");
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            openstory_diagnostics_checkpoint("window-terminate-egl");
            eglTerminate(display);
        }
#else
        if (context) SDL_GL_DeleteContext(context);
        if (window) SDL_DestroyWindow(window);
#endif
        openstory_diagnostics_checkpoint("window-quit-sdl");
        SDL_Quit();
        openstory_diagnostics_checkpoint("window-destroyed");
    }
    Error Window::init()
    {
        window_phase("sdl-main-ready");
        SDL_SetMainReady();
#ifdef PLATFORM_PS5
        window_phase("sdl-input-init");
        if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) < 0) return Error::WINDOW;
        window_phase("egl-display");
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        window_phase("egl-initialize");
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr) || !eglBindAPI(EGL_OPENGL_API))
            return Error::WINDOW;
        const EGLint attributes[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE};
        EGLConfig config;
        EGLint count;
        window_phase("egl-config");
        if (!eglChooseConfig(display, attributes, &config, 1, &count) || count != 1) return Error::WINDOW;
        window_phase("egl-surface");
        surface = eglCreateWindowSurface(display, config, 0, nullptr);
        const EGLint context_attributes[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
        window_phase("egl-context");
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
        window_phase("egl-make-current");
        if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT || !eglMakeCurrent(display, surface, surface, context))
            return Error::WINDOW;
        if (!eglQuerySurface(display, surface, EGL_WIDTH, &display_width) ||
            !eglQuerySurface(display, surface, EGL_HEIGHT, &display_height)) return Error::WINDOW;
        eglSwapInterval(display, 1);
#else
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) return Error::WINDOW;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        window = SDL_CreateWindow("OpenStory", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (!window || !(context = SDL_GL_CreateContext(window))) return Error::WINDOW;
        SDL_GL_SetSwapInterval(1);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_StartTextInput();
        SDL_GL_GetDrawableSize(window, &display_width, &display_height);
#endif
        Configuration::get().set_max_width(static_cast<int16_t>(display_width));
        Configuration::get().set_max_height(static_cast<int16_t>(display_height));
        window_phase("graphics-init");
        if (auto error = GraphicsGL::get().init()) return error;
        return initwindow();
    }
    Error Window::initwindow()
    {
        window_phase("graphics-reinit");
        width = Constants::Constants::get().get_physicalwidth();
        height = Constants::Constants::get().get_physicalheight();
        glViewport(0, 0, display_width, display_height);
        GraphicsGL::get().reinit();
#if defined(PLATFORM_PS5) && defined(OPENSTORY_LAN_LOG)
        static bool color_probe_done = false;
        if (!color_probe_done) {
            color_probe_done = true;
            check_renderer_colors(display_width, display_height);
        }
#endif
        return Error::NONE;
    }
    bool Window::not_closed() const { return !closed; }
    void Window::update() { updateopc(); }
    void Window::updateopc()
    {
        if (!opcstep) return;
        opacity += opcstep;
        if (opacity >= 1) { opacity = 1; opcstep = 0; }
        else if (opacity <= 0) { opacity = 0; opcstep = -opcstep; fadeprocedure(); }
    }
    void Window::check_events()
    {
        SDL_Event event;
        openstory_diagnostics_checkpoint("sdl-poll");
        while (SDL_PollEvent(&event))
        {
#ifdef PLATFORM_PS5
            if (native_keyboard::busy() && event.type != SDL_QUIT) continue;
#endif
            openstory_diagnostics_checkpoint("event-dispatch");
            switch (event.type)
            {
            case SDL_QUIT:
                window_phase("sdl-quit-received");
                std::fprintf(stderr, "[Window] SDL_QUIT received\n");
                closed = true;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (!event.key.repeat || (event.key.keysym.sym != SDLK_ESCAPE && event.key.keysym.sym != SDLK_RETURN))
                    UI::get().send_key(keycode(event.key.keysym.sym), event.type == SDL_KEYDOWN);
                break;
            case SDL_TEXTINPUT:
                try {
                    for (auto cp : std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().from_bytes(event.text.text))
                        UI::get().send_char(cp);
                } catch (const std::range_error&) {}
                break;
            case SDL_MOUSEMOTION:
                cursor_x = float(event.motion.x) * Constants::Constants::get().get_viewwidth() / display_width;
                cursor_y = float(event.motion.y) * Constants::Constants::get().get_viewheight() / display_height;
                UI::get().send_cursor(Point<int16_t>(int16_t(cursor_x), int16_t(cursor_y)));
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    UI::get().send_cursor(event.type == SDL_MOUSEBUTTONDOWN);
                    if (event.type == SDL_MOUSEBUTTONUP && event.button.clicks == 2) UI::get().doubleclick();
                }
                else if (event.button.button == SDL_BUTTON_RIGHT && event.type == SDL_MOUSEBUTTONDOWN) UI::get().rightclick();
                break;
            case SDL_MOUSEWHEEL: UI::get().send_scroll(event.wheel.y); break;
#ifndef PLATFORM_PS5
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GL_GetDrawableSize(window, &display_width, &display_height);
                    initwindow();
                }
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) UI::get().send_focus(0);
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) UI::get().send_focus(1);
                break;
#endif
            }
            openstory_diagnostics_checkpoint("sdl-poll");
        }
#ifdef PLATFORM_PS5
        const bool was_native = native_keyboard::busy();
        const bool activation = (controller && SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) ||
            (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
        if (native_keyboard::poll(activation, SDL_GetTicks())) keyboard_visible = UI::get().has_textfield();
        if (was_native && !native_keyboard::busy()) gamepad_blocked = true;
#endif
        openstory_diagnostics_checkpoint("controller-poll");
        poll_controller();
        openstory_diagnostics_checkpoint("gamepad-poll");
        Gamepad::get().poll();
        openstory_diagnostics_checkpoint("window-resize-check");
        if (width != Constants::Constants::get().get_physicalwidth() || height != Constants::Constants::get().get_physicalheight())
            initwindow();
    }
    void Window::begin() const { GraphicsGL::get().clearscene(); }
    void Window::end() const
    {
        openstory_diagnostics_checkpoint("keyboard-draw");
        draw_keyboard();
        openstory_diagnostics_checkpoint("graphics-flush");
        GraphicsGL::get().flush(opacity);
#if defined(PLATFORM_PS5) && defined(OPENSTORY_LAN_LOG)
        static unsigned capture_frame = 0;
        if (capture_frame < 60 && ++capture_frame == 60) {
            openstory_diagnostics_checkpoint("screenshot-readback");
            Window::get().take_screenshot();
        }
#endif
        openstory_diagnostics_checkpoint("buffer-swap");
#ifdef PLATFORM_PS5
        if (!eglSwapBuffers(display, surface)) {
            const auto egl_error = eglGetError();
            const auto gl_error = glGetError();
            window_phase("egl-swap-error", egl_error);
            window_phase("gl-error-after-swap", gl_error);
            std::fprintf(stderr, "[Window] eglSwapBuffers failed: EGL=%#x GL=%#x\n",
                         egl_error, gl_error);
            UI::get().quit();
        }
#else
        SDL_GL_SwapWindow(window);
#endif
    }
    void Window::fadeout(float step, std::function<void()> proc) { opcstep = -step; fadeprocedure = std::move(proc); }
    void Window::setclipboard(const std::string& text) const { SDL_SetClipboardText(text.c_str()); }
    std::string Window::getclipboard() const
    {
        char* text = SDL_GetClipboardText();
        std::string result = text ? text : "";
        SDL_free(text);
        return result;
    }
    void Window::toggle_fullscreen()
    {
#ifndef PLATFORM_PS5
        fullscreen = !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#endif
    }
    void Window::apply_mouse_speed(int) {} // System pointer speed is a Windows setting.
    void Window::take_screenshot()
    {
        std::vector<unsigned char> pixels(size_t(display_width) * display_height * 4);
        glReadPixels(0, 0, display_width, display_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        if (const auto error = glGetError(); error != GL_NO_ERROR) {
            std::fprintf(stderr, "[Window] screenshot-read-failed GL=%#x\n", error);
            return;
        }
        stbi_flip_vertically_on_write(1);
#ifdef PLATFORM_PS5
        // Keep private settings in their 0700 directory; expose only this diagnostic image.
        const auto filename = "/download0/openstory-frame-" + std::to_string(std::time(nullptr)) + ".png";
#else
        const auto filename = data_path("OpenStory-" + std::to_string(std::time(nullptr)) + ".png");
#endif
        if (!stbi_write_png(filename.c_str(), display_width, display_height, 4, pixels.data(), display_width * 4)) {
            std::fprintf(stderr, "[Window] screenshot-write-failed path=%s\n", filename.c_str());
            return;
        }
#ifdef PLATFORM_PS5
        if (chmod(filename.c_str(), 0644) != 0) {
            std::fprintf(stderr, "[Window] screenshot-permission-failed path=%s\n", filename.c_str());
            return;
        }
#endif
        std::printf("[Window] screenshot-saved path=%s width=%d height=%d\n",
                    filename.c_str(), display_width, display_height);
    }
}
