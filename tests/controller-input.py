#!/usr/bin/env python3
"""Compile real controller/keyboard input with fake GLFW and a key-state UI."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='openstory-controller-') as directory:
    work = Path(directory)
    # Keep production code unchanged; its local UI include resolves to this stub.
    shutil.copyfile(root / 'src/IO/Gamepad.cpp', work / 'Gamepad.cpp')
    (work / 'UI.h').write_text(r'''
#pragma once
#include "Keyboard.h"
#include <set>
namespace ms {
struct UI {
    Keyboard keyboard;
    std::set<int32_t> held;
    static UI& get() { static UI ui; return ui; }
    Keyboard& get_keyboard() { return keyboard; }
    void send_key(int32_t key, bool pressed) {
        if (pressed) held.insert(key);
        else held.erase(key);
    }
};
}
''')
    source = work / 'check.cpp'
    source.write_text(r'''
#include "Gamepad.h"
#include "UI.h"
#include <iostream>

bool present = true, standardized = true, state_read = true;
int button_count = GLFW_GAMEPAD_BUTTON_LAST + 1, axis_count = GLFW_GAMEPAD_AXIS_LAST + 1;
GLFWgamepadstate frame{};
extern "C" {
int glfwJoystickPresent(int) { return present; }
const char* glfwGetJoystickName(int) { return "Test controller"; }
int glfwJoystickIsGamepad(int) { return standardized; }
int glfwGetGamepadState(int, GLFWgamepadstate* state) {
    if (state_read) *state = frame;
    return state_read;
}
const unsigned char* glfwGetJoystickButtons(int, int* count) {
    *count = button_count; return button_count ? frame.buttons : nullptr;
}
const float* glfwGetJoystickAxes(int, int* count) {
    *count = axis_count; return axis_count ? frame.axes : nullptr;
}
}

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL (" << (standardized ? "gamepad" : "raw joystick")
                      << "): " << message << '\n';
            ++failures;
        }
    };
    auto reset = [] {
        present = true;
        state_read = true;
        button_count = GLFW_GAMEPAD_BUTTON_LAST + 1;
        axis_count = GLFW_GAMEPAD_AXIS_LAST + 1;
        frame = {};
        ms::UI::get() = {};
    };
    auto& ui = ms::UI::get();
    for (bool use_gamepad : {true, false}) {
        standardized = use_gamepad;
        for (bool release_stick_first : {true, false}) {
            reset();
            ms::Gamepad pad;
            frame.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = -1.0f;
            pad.poll();
            frame.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS;
            pad.poll();
            check(ui.held.count(GLFW_KEY_LEFT), "stick and D-pad hold LEFT");
            if (release_stick_first) frame.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = 0;
            else frame.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_RELEASE;
            pad.poll();
            check(ui.held.count(GLFW_KEY_LEFT), release_stick_first
                  ? "releasing stick must preserve held D-pad LEFT"
                  : "releasing D-pad must preserve held stick LEFT");
            frame = {};
            pad.poll();
            check(ui.held.empty(), "releasing both releases LEFT");
        }
        {
            reset();
            ms::Gamepad pad;
            ui.keyboard.assign(56, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            ui.keyboard.assign(29, ms::KeyType::ACTION, ms::KeyAction::ATTACK);
            frame.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = -1.0f;
            frame.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = -1.0f;
            frame.buttons[GLFW_GAMEPAD_BUTTON_A] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_B] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_BACK] = GLFW_PRESS;
            pad.poll();
            check(ui.held.size() == 5, "movement and action keys are held");
            present = false;
            pad.poll();
            check(ui.held.empty(), "disconnect releases every held key");
            check(!pad.is_connected() && pad.get_name().empty(), "disconnect clears connection");
            present = true;
            frame = {};
            pad.poll();
            check(ui.held.empty(), "neutral reconnect leaves no keys held");
        }
        {
            reset();
            ms::Gamepad pad;
            ui.keyboard.assign(57, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            ui.keyboard.assign(45, ms::KeyType::ACTION, ms::KeyAction::ATTACK);
            ui.keyboard.assign(44, ms::KeyType::ACTION, ms::KeyAction::PICKUP);
            frame.buttons[GLFW_GAMEPAD_BUTTON_A] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_B] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_X] = GLFW_PRESS;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE, GLFW_KEY_X, GLFW_KEY_Z},
                  "actions use current keyboard bindings and GLFW key codes");
            frame = {};
            pad.poll();
            check(ui.held.empty(), "remapped action keys release");
        }
        {
            reset();
            ms::Gamepad pad;
            ui.keyboard.assign(57, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            ui.keyboard.assign(45, ms::KeyType::ACTION, ms::KeyAction::ATTACK);
            ui.keyboard.assign(16, ms::KeyType::ACTION, ms::KeyAction::PICKUP);
            frame.buttons[GLFW_GAMEPAD_BUTTON_A] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_B] = GLFW_PRESS;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE, GLFW_KEY_X},
                  "two simultaneous action keys are held");
            ui.keyboard.remove(57);
            ui.keyboard.assign(44, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_Z, GLFW_KEY_X},
                  "held keyboard remapping releases old key and engages new key");
            pad.set_mapping(ms::Gamepad::GP_A, ms::KeyAction::PICKUP);
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_Q, GLFW_KEY_X},
                  "held controller remapping releases old key and engages new key");
            present = false;
            pad.poll();
            check(ui.held.empty(), "disconnect releases both remapped held keys");
        }
        {
            reset();
            ms::Gamepad pad;
            ui.keyboard.assign(57, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            frame.buttons[GLFW_GAMEPAD_BUTTON_A] = GLFW_PRESS;
            frame.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = -1.0f;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE, GLFW_KEY_LEFT},
                  "keys held before failed state read");
            state_read = false;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE, GLFW_KEY_LEFT},
                  "failed standardized read falls back to available raw state");
            button_count = axis_count = 0;
            pad.poll();
            check(ui.held.empty(), "unavailable state releases all held keys");
        }
        if (!standardized) {
            reset();
            ms::Gamepad pad;
            ui.keyboard.assign(57, ms::KeyType::ACTION, ms::KeyAction::JUMP);
            ui.keyboard.assign(45, ms::KeyType::ACTION, ms::KeyAction::ATTACK);
            frame.buttons[GLFW_GAMEPAD_BUTTON_A] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_B] = GLFW_PRESS;
            frame.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE, GLFW_KEY_X, GLFW_KEY_LEFT},
                  "raw buttons held before button count shrinks");
            button_count = 1;
            pad.poll();
            check(ui.held == std::set<int32_t>{GLFW_KEY_SPACE},
                  "fewer raw buttons release missing buttons and preserve remaining button");
            frame = {};
            pad.poll();
            check(ui.held.empty(), "remaining raw button releases");
        }
    }
    return failures ? 1 : 0;
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-include', 'cstddef', '-DGLFW_INCLUDE_NONE=',
                    '-I' + str(work), '-I' + str(root / 'src/IO'),
                    '-I' + str(root / 'vendor/glfw-3.3.2.bin.WIN64/include/GLFW'),
                    str(source), str(work / 'Gamepad.cpp'),
                    str(root / 'src/IO/Keyboard.cpp'), '-o', str(binary)], check=True)
    subprocess.run([binary], check=True)
print('PASS: held movement/remapping, disconnect, failed reads, and shorter raw button state')
