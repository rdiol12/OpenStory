#!/usr/bin/env python3
"""Check real UI state-transition cleanup before its previous owner is freed."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
ui_source = (root / 'src/IO/UI.cpp').read_text()
start = ui_source.index('\tvoid UI::change_state(')
change_state = ui_source[start:ui_source.index('\n\tvoid UI::quit()', start)]
start = ui_source.index('\tvoid UI::remove_textfield()')
remove_textfield = ui_source[start:ui_source.index('\n\tbool UI::text_input(', start)]

with tempfile.TemporaryDirectory(prefix='openstory-controller-state-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(r'''
#include "ControllerNavigation.h"
#include <cassert>
#include <functional>
#include <memory>
namespace ms {
struct Textfield {
    enum State { NORMAL, FOCUSED };
    State state = FOCUSED;
    void set_state(State value) { state = value; }
};
struct UIElement {};
struct UIState {
    Textfield field;
    std::function<void()> before_destroy;
    virtual ~UIState() { if (before_destroy) before_destroy(); }
};
struct UIStateLogin : UIState {};
struct UIStateGame : UIState {};
struct UIStateCashShop : UIState {};
struct UI {
    enum class State { LOGIN, GAME, CASHSHOP };
    UIElement* controller_front = nullptr;
    UIElement* controller_selected = nullptr;
    ControllerNavigation controller_navigation;
    std::unique_ptr<UIState> state = std::make_unique<UIState>();
    Textfield* focusedtextfield = nullptr;
    uint64_t textfield_revision = 0;
    void change_state(State);
    void remove_textfield();
};
''' + change_state + remove_textfield + r'''
}
int main() {
    for (auto destination : {ms::UI::State::LOGIN, ms::UI::State::GAME, ms::UI::State::CASHSHOP}) {
        ms::UI ui;
        auto* old_field = &ui.state->field;
        ui.focusedtextfield = old_field;
        ms::UIElement element;
        ui.controller_front = ui.controller_selected = &element;
        ui.controller_navigation.enter(reinterpret_cast<std::uintptr_t>(&element), 3);
        ui.controller_navigation.step(1, 3); // Move away from the initial selection.
        bool destroyed = false;
        ui.state->before_destroy = [&] {
            assert(!ui.focusedtextfield && "clear text focus before destroying its owner");
            assert(ui.textfield_revision == 1 && "invalidate pending native keyboard results before destruction");
            assert(old_field->state == ms::Textfield::NORMAL && "neutralize the old field while alive");
            assert(!ui.controller_front && !ui.controller_selected && "clear controller window pointers before destruction");
            assert(ui.controller_navigation.index() == 0 && "reset controller selection before destruction");
            destroyed = true;
        };
        ui.change_state(destination);
        assert(destroyed && "transition must replace the old state");
    }
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-I' + str(root / 'src/IO'),
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([binary], check=True)
print('PASS: text focus and controller selection reset before old UI destruction')
