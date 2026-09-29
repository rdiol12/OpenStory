// Normal PS5 IME application API. ABI matches the pinned ProsperoTV iptv_ime.c.
#include "Keyboard.h"
#include "IO/UI.h"
#include <algorithm>
#include <array>
#include <codecvt>
#include <cstdio>
#include <locale>

namespace {
    struct Parameter {
        int32_t user, type;
        uint64_t languages;
        int32_t enter_label, input_method;
        void* filter;
        uint32_t option, max_length;
        char16_t* text;
        float x, y;
        int32_t horizontal, vertical;
        const char16_t *placeholder, *title;
        int8_t reserved[16];
    };
    struct Result { int32_t outcome; int8_t reserved[12]; };
    static_assert(sizeof(Parameter) == 96 && sizeof(Result) == 16);
    std::array<char16_t, 256> buffer{};
    ms::UI::TextInput input;
    bool requested = false, active = false, cancelled = false, loaded = false;
    uint32_t started = 0;
    void clear() {
        volatile char16_t* memory = buffer.data();
        for (size_t i = 0; i < buffer.size(); ++i) memory[i] = 0;
        std::fill(input.text.begin(), input.text.end(), '\0');
        input.text.clear();
        requested = active = cancelled = false;
    }
    void report(const char* operation, int result) {
        std::fprintf(stderr, "[Keyboard] %s result=0x%x\n", operation, static_cast<unsigned>(result));
    }
}
extern "C" {
    int sceCommonDialogInitialize();
    int sceSysmoduleLoadModule(uint16_t);
    int sceUserServiceGetForegroundUser(int32_t*);
    int sceImeDialogInit(const Parameter*, const void*);
    int sceImeDialogGetStatus();
    int sceImeDialogGetResult(Result*);
    int sceImeDialogAbort();
    int sceImeDialogTerm();
}

namespace ms::native_keyboard {
    bool busy() { return requested || active; }
    bool open() {
        if (busy()) return true;
        if (!UI::get().text_input(input)) return false;
        requested = true;
        return true;
    }
    bool poll(bool activation_held, uint32_t now) {
        if (!busy()) return false;
        UI::TextInput current;
        if (!UI::get().text_input(current) || current.revision != input.revision) {
            if (!active) { clear(); return false; }
            if (!cancelled) { report("abort-focus-change", sceImeDialogAbort()); cancelled = true; }
        }
        if (requested) {
            if (activation_held) return false; // Do not let the opener also type/submit.
            int result = 0;
            if (!loaded) {
                result = sceCommonDialogInitialize();
                report("common-dialog", result);
                if (result < 0 && static_cast<uint32_t>(result) != 0x80b80002u) { clear(); return true; }
                result = sceSysmoduleLoadModule(0x0096);
                report("load-ime", result);
                if (result < 0) { clear(); return true; }
                loaded = true;
            }
            Parameter parameter{};
            result = sceUserServiceGetForegroundUser(&parameter.user);
            if (result < 0) { report("foreground-user", result); clear(); return true; }
            try {
                auto text = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(input.text);
                size_t size = std::min(text.size(), buffer.size()-1);
                if (size && text[size-1] >= 0xd800 && text[size-1] <= 0xdbff) --size;
                std::copy_n(text.data(), size, buffer.data());
                std::fill(text.begin(), text.end(), 0);
            } catch (const std::range_error&) { clear(); return true; }
            parameter.type = input.password ? 1 : 0;
            parameter.option = input.password ? 4 : 0;
            parameter.max_length = static_cast<uint32_t>(std::min(input.limit, buffer.size()-1));
            parameter.text = buffer.data();
            parameter.horizontal = parameter.vertical = 1;
            parameter.title = input.password ? u"Password" : u"Enter text";
            parameter.placeholder = u"";
            result = sceImeDialogInit(&parameter, nullptr);
            report("open", result);
            if (result < 0) { clear(); return true; }
            requested = false; active = true; started = now;
        }
        int status = sceImeDialogGetStatus();
        if (status == 1 || (status == 0 && now-started < 1000)) return false;
        bool failed = status != 2;
        if (status == 2 && !cancelled) {
            Result result{};
            int code = sceImeDialogGetResult(&result);
            report("result", code);
            failed = code < 0;
            if (code >= 0 && result.outcome == 0) {
                try {
                    const auto end = std::find(buffer.begin(), buffer.end(), 0);
                    auto value = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(buffer.data(), buffer.data()+(end-buffer.begin()));
                    UI::get().text_input_result(input.revision, value);
                    std::fill(value.begin(), value.end(), '\0');
                } catch (const std::range_error&) { failed = true; }
            }
        }
        report("close", sceImeDialogTerm());
        clear();
        return failed;
    }
    void shutdown() {
        if (active) { sceImeDialogAbort(); sceImeDialogTerm(); }
        clear();
    }
}
