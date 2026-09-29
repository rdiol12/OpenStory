#include "Audio/Audio.h"
#include "Configuration.h"
#include "IO/UI.h"
#include "IO/Components/AreaButton.h"
#include "IO/UITypes/UILogin.h"
#include "IO/UITypes/UITermsOfService.h"
#include "../platform/sdl/ConsoleMenu.h"
#include <glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace ms { Error init(bool offline); }

using namespace ms;
static std::vector<UIElement::Type> actions;
static uint16_t last_button = 0;
static int keyboard_requests = 0;

// Keep the real field/controller routing, but record the platform keyboard request.
extern "C" void __wrap__ZN2ms6Window13show_keyboardEv(void*) { ++keyboard_requests; }

template<UIElement::Type Kind, bool Modal = false>
class Panel : public UIElement {
public:
    static constexpr Type TYPE = Kind;
    static constexpr bool FOCUSED = Modal, TOGGLED = false;
    Panel() : UIElement({20, 20}, {180, 100}) {
        buttons[0] = std::make_unique<AreaButton>(Point<int16_t>{0, 0}, Point<int16_t>{60, 40});
        buttons[1] = std::make_unique<AreaButton>(Point<int16_t>{80, 0}, Point<int16_t>{60, 40});
    }
    Type get_type() const override { return TYPE; }
    Button::State button_state(uint16_t id) const { return buttons.at(id)->get_state(); }
    Button::State button_pressed(uint16_t button) override {
        actions.push_back(TYPE);
        last_button = button;
        return Button::NORMAL;
    }
};

using First = Panel<UIElement::PARTYSETTINGS>;
using Second = Panel<UIElement::PARTYINVITE>;
using Third = Panel<UIElement::PARTYMEMBERMENU>;
using Modal = Panel<UIElement::PARTYSEARCHSTART, true>;

static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        Configuration::get().load();
        if (auto error = ms::init(true))
            throw std::runtime_error(std::string(error.get_message()) + error.get_args());
        struct CloseAudio { ~CloseAudio() { Sound::close(); } } close_audio;
        auto& ui = UI::get();
        auto clear = [&] {
            for (auto type : {UIElement::START, UIElement::LOGIN, UIElement::SERVERSELECT,
                              First::TYPE, Second::TYPE, Third::TYPE, Modal::TYPE})
                ui.remove(type);
            ui.update();
            check(!ui.controller_menu(), "test panels must start with no other active window");
        };
        auto activate = [&](uint32_t now, UIElement::Type expected) {
            const auto count = actions.size();
            ui.controller_input(GLFW_KEY_ENTER, now);
            ui.update_controller_focus();
            check(actions.size() == count + 1 && actions.back() == expected,
                  "single Cross must activate exactly once on the selected panel");
        };

        clear();
        auto first = ui.emplace<First>();
        auto second = ui.emplace<Second>();
        auto third = ui.emplace<Third>();
        ui.update_controller_focus();
        check(third->button_state(0) == Button::MOUSEOVER && third->button_state(1) == Button::NORMAL,
              "controller focus uses the selected button's existing hover state");
        activate(1000, Third::TYPE);
        check(last_button == 0, "first Cross activates the initially focused control");
        ui.controller_input(GLFW_KEY_RIGHT, 1100);
        check(third->button_state(1) == Button::MOUSEOVER && third->button_state(0) == Button::NORMAL,
              "moving controller focus clears the previous button hover");
        ui.controller_input(GLFW_KEY_ENTER, 1200);
        ui.controller_input(GLFW_KEY_ENTER, 1700);
        check(actions.size() == 3 && last_button == 1, "repeated Cross presses activate the same D-pad selection");
        for (auto selected : {First::TYPE, Second::TYPE, Third::TYPE}) {
            ui.controller_cycle_window();
            activate(2000 + actions.size() * 1000, selected);
        }
        ui.controller_cycle_window();
        ui.controller_input(GLFW_KEY_ESCAPE, 8000);
        check(!first->is_active() && second->is_active() && third->is_active(), "Circle must close only the cycled first panel");
        ui.controller_cycle_window();
        ui.controller_input(GLFW_KEY_ESCAPE, 8100);
        check(!second->is_active() && third->is_active(), "cycling after close must reach the remaining second panel");
        ui.controller_input(GLFW_KEY_ESCAPE, 8200);
        check(!third->is_active() && !ui.controller_menu(), "Circle must close the last panel");

        clear();
        ui.emplace<First>();
        ui.emplace<Second>();
        third = ui.emplace<Third>();
        auto modal = ui.emplace<Modal>();
        ui.controller_cycle_window();
        activate(10000, Modal::TYPE);
        ui.controller_input(GLFW_KEY_ESCAPE, 10200);
        check(!modal->is_active() && third->is_active(), "Circle closes modal without closing its background");
        activate(11000, Third::TYPE);

        clear();
        ui.emplace<First>();
        ui.controller_input(GLFW_KEY_RIGHT, 12000);
        const auto count = actions.size();
        ui.remove(First::TYPE);
        ui.update(); // Free the old owner before another same-type allocation.
        ui.emplace<First>();
        ui.controller_input(GLFW_KEY_ENTER, 12100);
        check(actions.size() == count + 1 && last_button == 0, "replacement panel starts at its first control and accepts Cross");
        ui.controller_input(GLFW_KEY_ENTER, 12200);
        check(actions.size() == count + 2 && last_button == 0, "repeated Cross must preserve the replacement panel's selection");

        ui.controller_input(GLFW_KEY_RIGHT, 12300);
        ui.change_state(UI::LOGIN);
        ui.remove(UIElement::START);
        ui.remove(UIElement::LOGIN);
        ui.emplace<First>();
        ui.controller_input(GLFW_KEY_ENTER, 12400);
        check(actions.size() == count + 3 && last_button == 0, "UI state reset must activate its own initial selection");
        ui.controller_input(GLFW_KEY_ENTER, 12500);
        check(actions.size() == count + 4 && last_button == 0, "new UI state must preserve selection across Cross presses");

        clear();
        auto login = ui.emplace<UILogin>();
        ui.update();
        auto requests = keyboard_requests;
        ui.controller_input(GLFW_KEY_ENTER, 13000);
        check(ui.has_textfield() && keyboard_requests == requests + 1,
              "first Cross must open the focused credential's keyboard");
        ui.controller_input(GLFW_KEY_ENTER, 13100);
        UI::TextInput selected_field;
        check(ui.text_input(selected_field) && !selected_field.password && keyboard_requests == requests + 2,
              "another Cross must reopen the same account field without moving to password");

        ui.remove(UIElement::LOGIN);
        login = ui.emplace<UILogin>(); // The credential is already focused by the constructor.
        ui.update();
        const auto bounds = login->controller_targets().front().bounds;
        const Point<int16_t> point((bounds.left() + bounds.right()) / 2,
                                   (bounds.top() + bounds.bottom()) / 2);
        for (int attempt = 0; attempt < 2; ++attempt) {
            requests = keyboard_requests;
            ui.send_cursor(point);
            ui.send_cursor(true);
            ui.send_cursor(false);
            check(ui.has_textfield() && keyboard_requests > requests,
                  "clicking an already-focused credential must request/reopen the platform keyboard");
        }

        UI::TextInput pending, current;
        check(ui.text_input(pending), "credential activation exposes text input metadata");
        ui.remove(UIElement::LOGIN);
        ui.update();
        login = ui.emplace<UILogin>();
        ui.update();
        check(ui.text_input(current), "replacement credential is focused");
        const auto replacement_text = current.text;
        ui.text_input_result(pending.revision, "stale");
        check(ui.text_input(current) && current.text == replacement_text && current.revision != pending.revision,
              "an IME result for a destroyed/replaced field must be ignored");
        pending = current;
        ui.change_state(UI::LOGIN);
        ui.remove(UIElement::START);
        login = ui.emplace<UILogin>();
        ui.update();
        check(ui.text_input(current), "new UI state exposes its credential field");
        const auto new_state_text = current.text;
        ui.text_input_result(pending.revision, "stale");
        check(ui.text_input(current) && current.text == new_state_text && current.revision != pending.revision,
              "an IME result must not cross a UI state transition");

        Textfield field(Text::A13M, Text::LEFT, Color::Name::WHITE,
                        {Point<int16_t>{0, 0}, Point<int16_t>{150, 30}}, 5);
        field.change_text("saved");
        field.set_cryptchar('*');
        field.set_state(Textfield::FOCUSED);
        check(ui.text_input(pending) && pending.text == "saved" && pending.limit == 5 && pending.password,
              "native keyboard receives current text, byte limit, and password mode");
        ui.remove_textfield(); // A cancelled dialog delivers no result.
        check(field.get_text() == "saved" && !ui.text_input(current), "cancel/defocus must preserve the field text");
        field.set_state(Textfield::FOCUSED);
        ui.text_input_result(pending.revision, "stale");
        check(field.get_text() == "saved", "a reopened field must reject its previous dialog result");
        check(ui.text_input(current), "reopened field exposes a fresh revision");
        ui.text_input_result(current.revision, "new");
        check(field.get_text() == "new", "accepted IME text replaces rather than appends");
        check(ui.text_input(current), "field remains available after replacement");
        ui.text_input_result(current.revision, "");
        check(field.get_text().empty(), "an accepted empty result clears the field");
        check(ui.text_input(current), "field remains available after clearing");
        ui.text_input_result(current.revision, "abcdefgh");
        check(field.get_text() == "abcde", "accepted text respects the field byte limit");
        check(ui.text_input(current), "field remains available after truncation");
        ui.text_input_result(current.revision, u8"abc\u00e9z");
        check(field.get_text() == u8"abc\u00e9", "a complete UTF-8 character at the byte limit is retained");
        check(ui.text_input(current), "field remains available for UTF-8 boundary check");
        ui.text_input_result(current.revision, u8"abcd\u00e9");
        check(field.get_text() == "abcd", "byte limit must not split a UTF-8 character");
        field.set_state(Textfield::NORMAL); // Some existing Enter callbacks defocus directly.
        check(!ui.text_input(pending), "a normal field must not expose an active native text session");
        ui.text_input_result(current.revision, "late");
        check(field.get_text() == "abcd", "a delayed native result must not edit a normal field");
        ui.remove_textfield();

        clear();
        login = ui.emplace<UILogin>();
        ui.update();
        ui.update_controller_focus(); // Controller selection remains on account/index 0.
        check(ui.text_input(current) && !current.password, "controller begins on the account field");
        login->controller_activate(login->controller_targets()[1]);
        check(ui.text_input(pending) && pending.password, "explicit activation focuses the password field");
        ui.update_controller_focus();
        check(ui.text_input(current) && current.password && current.revision == pending.revision,
              "controller hover must preserve the password focus/session chosen by explicit activation");

        clear();
        auto server = ui.emplace<UIServerSelect>();
        ui.update();
        ui.update_controller_focus(); // Controller selection remains on host/index 0.
        check(ui.text_input(current) && current.limit == 15, "controller begins on the server host field");
        ui.send_key(GLFW_KEY_TAB, true);
        ui.send_key(GLFW_KEY_TAB, false);
        check(ui.text_input(pending) && pending.limit == 5 && pending.text == server->service(),
              "keyboard Tab focuses the server port field");
        ui.update_controller_focus();
        check(ui.text_input(current) && current.limit == 5 && current.revision == pending.revision,
              "controller hover must preserve the server port focus/session chosen by Tab");
        clear();
        server = ui.emplace<UIServerSelect>();
        ui.update_controller_focus();
        check(ui.text_input(current), "fresh server selector focuses the host field");
        ui.text_input_result(current.revision, "127.0.0.1");
        ui.remove_textfield(); // The native keyboard closes after submitting the address.
        ui.controller_input(GLFW_KEY_RIGHT, 30000); // Host -> Port.
        ui.controller_input(GLFW_KEY_RIGHT, 30100); // Port -> Connect.
        ui.update_controller_focus();
        ui.controller_input(GLFW_KEY_ENTER, 30200);
        check(server->take_choice() == UIServerSelect::CONNECT,
              "first Cross after D-pad selects Connect must connect after keyboard dismissal");
        clear();
        auto terms = ui.emplace<UITermsOfService>([] {});
        check(terms->controller_targets().size() == 1, "Accept starts disabled before reading the terms");
        ui.controller_input(GLFW_KEY_PAGE_UP, 31000);
        check(terms->controller_targets().size() == 1, "L2 at the top must not unlock Accept");
        for (int page = 0; page < 512; ++page)
            ui.controller_input(GLFW_KEY_PAGE_DOWN, 32000 + page);
        check(terms->controller_targets().size() == 2,
              "R2 must scroll the real Terms window to the bottom and enable Accept");
        ui.controller_input(GLFW_KEY_PAGE_UP, 33000);
        check(terms->controller_targets().size() == 1, "L2 must scroll the Terms window back up");
        ui.controller_input(GLFW_KEY_PAGE_DOWN, 33100);
        check(terms->controller_targets().size() == 2, "scrolling must clamp at the bottom and remain reversible");
        ui.remove(UIElement::TOS);
        clear();
        std::cout << "PASS: production UI navigation, Terms scrolling, credential activation, and native text-result lifetime/limits\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
