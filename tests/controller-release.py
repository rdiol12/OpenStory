#!/usr/bin/env python3
"""Check the production key-release prelude before text-field/modal routing."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/IO/UI.cpp').read_text()
start = source.index('void UI::send_key(int32_t keycode, bool pressed)')
end = source.index('\n\t\tif ((is_key_down[GLFW_KEY_LEFT_ALT]', start)
prelude = source[start:end]
with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    (work / 'check.cpp').write_text(r'''
#include <cassert>
#include <cstdint>
#include <map>
namespace ms {
namespace KeyType { enum Id { ACTION, SKILL, MENU }; }
namespace KeyAction { enum Id { LEFT, RIGHT, UP, DOWN, JUMP, ATTACK, RETURN }; }
struct Mapping { KeyType::Id type; KeyAction::Id action; };
struct Keyboard {
    Mapping mapping{KeyType::ACTION, KeyAction::LEFT};
    Mapping get_mapping(int) { return mapping; }
};
struct Stage {
    int releases = 0;
    static Stage& get() { static Stage stage; return stage; }
    void send_key(KeyType::Id type, int action, bool down) { assert(type == KeyType::ACTION && !down); ++releases; }
};
struct UI {
    Keyboard keyboard;
    std::map<int, bool> is_key_down;
    int modal_events = 0;
    void send_key(int32_t keycode, bool pressed);
};
''' + prelude + r'''
    ++modal_events; // The existing focused text-field/modal consumes the rest.
}
}
int main() {
    ms::UI ui;
    for (auto action : {ms::KeyAction::LEFT, ms::KeyAction::RIGHT, ms::KeyAction::UP,
         ms::KeyAction::DOWN, ms::KeyAction::JUMP, ms::KeyAction::ATTACK}) {
        ui.keyboard.mapping = {ms::KeyType::ACTION, action};
        int previous = ms::Stage::get().releases;
        ui.is_key_down[123] = true;
        ui.send_key(123, false);
        assert(ms::Stage::get().releases == previous + 1 && !ui.is_key_down[123]);
    }
    assert(ui.modal_events == 0);
    ui.keyboard.mapping = {ms::KeyType::SKILL, ms::KeyAction::LEFT};
    ui.send_key(123, false);
    ui.keyboard.mapping = {ms::KeyType::ACTION, ms::KeyAction::LEFT};
    ui.send_key(123, true);
    assert(ui.modal_events == 2);
}
''')
    subprocess.run(['g++', '-std=c++17', work/'check.cpp', '-o', work/'check'], check=True)
    subprocess.run([work/'check'], check=True)
print('PASS: movement/attack/jump releases reach gameplay before modal routing')
