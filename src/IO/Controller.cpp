#include "UI.h"
#include "Window.h"
#include "Gamepad.h"
#include "UITypes/UIChatBar.h"
#include "UITypes/UIJoypad.h"
#include <algorithm>

namespace ms
{
    void UI::controller_open(KeyAction::Id action)
    {
        if (!enabled) return;
        remove_textfield();
        state->send_key(KeyType::MENU, action, true, false);
#ifdef OPENSTORY_SDL
        if (focusedtextfield) Window::get().show_keyboard();
#endif
    }
    std::vector<UIElement*> UIState::controller_windows()
    {
        std::list<UIElement::Type> types;
        for (int i = 1; i < UIElement::NUM_TYPES; ++i) {
            const auto type = static_cast<UIElement::Type>(i);
            switch (type) {
            case UIElement::STATUSBAR: case UIElement::STATUSMESSENGER:
            case UIElement::CHATBAR: case UIElement::BUFFLIST: case UIElement::MINIMAP:
            case UIElement::CLOCK: case UIElement::QUESTHELPER: case UIElement::PARTYHUD:
            case UIElement::NOTIFICATIONLIST: case UIElement::TOASTSTACK: case UIElement::AVATARBANNER:
                break;
            default: types.push_back(type); break;
            }
        }
        std::vector<UIElement*> result;
        while (auto* front = get_front(types)) {
            result.push_back(front);
            types.remove(front->get_type());
        }
        return result;
    }

    UIElement* UI::controller_window()
    {
        const auto windows = state->controller_windows();
        auto* front = windows.empty() ? nullptr : windows.front();
        if (front != controller_front) {
            controller_front = front;
            controller_selected = front;
        }
        if (std::find(windows.begin(), windows.end(), controller_selected) == windows.end())
            controller_selected = front;
        return controller_selected;
    }

    bool UI::controller_menu()
    {
        return controller_window() != nullptr;
    }

    void UI::controller_cycle_window()
    {
        auto* current = controller_window();
        auto windows = state->controller_windows();
        if (windows.size() < 2) return;
        std::sort(windows.begin(), windows.end(), [](const auto* a, const auto* b) { return a->get_type() < b->get_type(); });
        auto it = std::find(windows.begin(), windows.end(), current);
        controller_selected = it == windows.end() || ++it == windows.end() ? windows.front() : *it;
        state->controller_raise(controller_selected->get_type());
        const auto raised = state->controller_windows();
        controller_front = raised.empty() ? nullptr : raised.front();
        controller_navigation.enter(0, 0);
        remove_textfield();
    }

    void UI::controller_input(int key, uint32_t now)
    {
        if (!enabled) return;
        auto* element = controller_window();
        if (!element) return;
        auto targets = element->controller_targets();
        controller_navigation.enter(reinterpret_cast<uintptr_t>(element), targets.size());
        if (key == GLFW_KEY_ESCAPE) {
            remove_textfield();
            const auto type = element->get_type();
            if (type == UIElement::CASHSHOP) {
                state->send_key(KeyType::ACTION, KeyAction::ESCAPE, true, true);
                return;
            }
            element->send_key(KeyAction::ESCAPE, true, true);
            // Screens with no keyboard handler still have a consistent Back action.
            if (state->get(type) == element && element->is_active() && type != UIElement::LOGIN &&
                type != UIElement::SERVERSELECT) element->deactivate();
            controller_navigation.enter(0, 0);
            return;
        }
        if (key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_PAGE_DOWN) {
            element->send_scroll(key == GLFW_KEY_PAGE_UP ? 1 : -1);
            controller_navigation.enter(0, 0);
            return;
        }
        if (key == GLFW_KEY_TAB) {
            remove_textfield();
            element->send_key(KeyAction::TAB, true, false);
            controller_navigation.enter(0, 0);
            return;
        }
        if (targets.empty()) {
            if (key == GLFW_KEY_ENTER) element->send_key(KeyAction::RETURN, true, false);
            return;
        }
        if (key == GLFW_KEY_ENTER) {
            if (controller_navigation.press(now, targets.size())) {
                remove_textfield();
                element->controller_activate(targets[controller_navigation.index()]);
#ifdef OPENSTORY_SDL
                if (focusedtextfield) Window::get().show_keyboard();
#endif
                return;
            }
        } else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_UP)
            controller_navigation.step(-1, targets.size());
        else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_DOWN)
            controller_navigation.step(1, targets.size());
        else return;
        remove_textfield();
        const auto& bounds = targets[controller_navigation.index()].bounds;
        const Point<int16_t> center((bounds.left() + bounds.right()) / 2, (bounds.top() + bounds.bottom()) / 2);
        cursor.set_position(center);
        element->controller_hover(targets[controller_navigation.index()]);
    }

    void UI::update_controller_focus()
    {
        auto* element = controller_window();
        if (!element) return;
        const auto targets = element->controller_targets();
        controller_navigation.enter(reinterpret_cast<uintptr_t>(element), targets.size());
        if (targets.empty()) return;
        element->controller_hover(targets[controller_navigation.index()]);
    }
}
