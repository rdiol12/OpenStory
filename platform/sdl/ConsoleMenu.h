#pragma once
#include "IO/UIElement.h"
#include "IO/Components/Textfield.h"

namespace ms
{
    class UIServerSelect : public UIElement
    {
    public:
        static constexpr Type TYPE = SERVERSELECT;
        static constexpr bool FOCUSED = true, TOGGLED = false;
        enum Choice { WAIT, CONNECT, OFFLINE };
        UIServerSelect();
        void draw(float alpha) const override;
        void update() override;
        Cursor::State send_cursor(bool pressed, Point<int16_t> point) override;
        void send_key(int32_t key, bool pressed, bool escape) override;
        std::vector<ControllerTarget> controller_targets() const override;
        void controller_hover(const ControllerTarget& target) override;
        Type get_type() const override { return TYPE; }
        Choice take_choice() { auto value = choice; choice = WAIT; return value; }
        std::string address() const { return host.get_text(); }
        std::string service() const { return port.get_text(); }
        void message(const std::string& text) { status.change_text(text); }
    protected:
        Button::State button_pressed(uint16_t id) override;
    private:
        Textfield host, port;
        Text status;
        Choice choice = WAIT;
    };

    class UIControllerMenu : public UIElement
    {
    public:
        static constexpr Type TYPE = CONTROLLERMENU;
        static constexpr bool FOCUSED = false, TOGGLED = true;
        UIControllerMenu();
        void draw(float alpha) const override;
        Type get_type() const override { return TYPE; }
        void send_key(int32_t, bool pressed, bool escape) override { if (pressed && escape) deactivate(); }
    protected:
        Button::State button_pressed(uint16_t id) override;
    };
}
