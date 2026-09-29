#!/usr/bin/env python3
"""Exercise production PS5 keyboard lifecycle with ordinary API/UI test doubles."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='openstory-native-keyboard-') as directory:
    work = Path(directory)
    (work / 'IO').mkdir()
    (work / 'IO/UI.h').write_text(r'''
#pragma once
#include <cstdint>
#include <string>
namespace ms {
struct UI {
    struct TextInput { std::string text; size_t limit; bool password; uint64_t revision; };
    TextInput input{u8"h\u00e9\U0001f600", 64, true, 7};
    bool focused = true;
    int applied = 0;
    uint64_t applied_revision = 0;
    static UI& get() { static UI ui; return ui; }
    bool text_input(TextInput& out) const { if (!focused) return false; out = input; return true; }
    void text_input_result(uint64_t revision, const std::string& text) {
        applied_revision = revision;
        if (focused && revision == input.revision) { input.text = text; ++applied; }
    }
};
}
''')
    source = work / 'check.cpp'
    source.write_text(r'''
#include "platform/ps5/Keyboard.cpp"
#include <cassert>
#include <cstring>

int common_result = 0, module_result = 0, user_result = 0, init_result = 0, result_code = 0;
int status = 1, outcome = 0, init_calls = 0, abort_calls = 0, term_calls = 0, result_calls = 0;
Parameter opened{};
std::u16string initial;
extern "C" {
int sceCommonDialogInitialize() { return common_result; }
int sceSysmoduleLoadModule(uint16_t module) { assert(module == 0x0096); return module_result; }
int sceUserServiceGetForegroundUser(int32_t* user) { *user = 42; return user_result; }
int sceImeDialogInit(const Parameter* value, const void*) {
    ++init_calls; opened = *value; initial = value->text; return init_result;
}
int sceImeDialogGetStatus() { return status; }
int sceImeDialogGetResult(Result* value) { ++result_calls; value->outcome = outcome; return result_code; }
int sceImeDialogAbort() { ++abort_calls; return 0; }
int sceImeDialogTerm() { ++term_calls; return 0; }
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    auto& ui = ms::UI::get();
    namespace keyboard = ms::native_keyboard;
    if (mode == "common") common_result = -1;
    if (mode == "module") module_result = -1;
    if (mode == "user") user_result = -1;
    if (mode == "init") init_result = -1;
    if (mode == "already-initialized") common_result = static_cast<int32_t>(0x80b80002u);
    assert(!keyboard::busy());
    assert(keyboard::open() && keyboard::busy());
    assert(keyboard::open());
    assert(!keyboard::poll(true, 100) && init_calls == 0); // Wait for activation release.
    if (mode == "requested-stale") {
        ui.focused = false;
        assert(!keyboard::poll(false, 200) && !keyboard::busy() && init_calls == 0);
        return 0;
    }
    const bool failed = keyboard::poll(false, 200);
    if (mode == "common" || mode == "module" || mode == "user" || mode == "init") {
        assert(failed && !keyboard::busy() && ui.applied == 0);
        return 0;
    }
    assert(!failed && init_calls == 1 && keyboard::busy());
    assert(opened.user == 42 && opened.type == 1 && opened.option == 4 && opened.max_length == 64);
    assert(initial == u"h\u00e9\U0001f600"); // UTF-8 -> UTF-16 includes a surrogate pair.
    assert(keyboard::open() && !keyboard::poll(false, 250) && init_calls == 1);
    if (mode == "shutdown") {
        keyboard::shutdown();
        assert(!keyboard::busy() && abort_calls == 1 && term_calls == 1 && ui.applied == 0);
        assert(opened.text[0] == 0);
        return 0;
    }
    if (mode == "stale") {
        ++ui.input.revision;
        assert(!keyboard::poll(false, 300) && abort_calls == 1);
        assert(!keyboard::poll(false, 350) && abort_calls == 1);
        status = 2;
        assert(!keyboard::poll(false, 400));
        assert(!keyboard::busy() && result_calls == 0 && term_calls == 1 && ui.applied == 0);
        return 0;
    }
    if (mode == "status") {
        status = -1;
        assert(keyboard::poll(false, 300) && !keyboard::busy() && term_calls == 1 && ui.applied == 0);
        return 0;
    }
    status = 2;
    if (mode == "result") result_code = -1;
    if (mode == "cancel") outcome = 1;
    const std::u16string replacement = u"new \u05d0\U0001f600";
    std::copy(replacement.begin(), replacement.end(), opened.text);
    opened.text[replacement.size()] = 0;
    assert(keyboard::poll(false, 400) == (mode == "result"));
    assert(!keyboard::busy() && result_calls == 1 && term_calls == 1 && opened.text[0] == 0);
    if (mode == "cancel" || mode == "result") {
        assert(ui.applied == 0 && ui.input.text == u8"h\u00e9\U0001f600");
    } else {
        assert(ui.applied == 1 && ui.applied_revision == 7 && ui.input.text == u8"new \u05d0\U0001f600");
    }
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-I' + str(work), '-I' + str(root),
                    str(source), '-o', str(binary)], check=True)
    for case in ('accept', 'cancel', 'stale', 'requested-stale', 'shutdown', 'common',
                 'module', 'user', 'init', 'status', 'result', 'already-initialized'):
        subprocess.run([binary, case], check=True)
print('PASS: native keyboard release gate, failures/fallback, accept/cancel, stale focus, UTF-8, password, and cleanup')
