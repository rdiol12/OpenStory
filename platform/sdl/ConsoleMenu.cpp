#include "ConsoleMenu.h"
#include "Configuration.h"
#include "Constants.h"
#include "IO/UI.h"
#include "IO/Components/AreaButton.h"
#include "Net/ServerAddress.h"

namespace ms
{
    static void label(const std::string& value, Point<int16_t> p, bool title = false)
    {
        Text(title ? Text::A18M : Text::A13M, Text::LEFT, Color::Name::WHITE, value).draw(p);
    }

    UIServerSelect::UIServerSelect()
    {
        position = {(int16_t)((Constants::Constants::get().get_viewwidth()-640)/2),
                    (int16_t)((Constants::Constants::get().get_viewheight()-360)/2)};
        dimension = {640, 360};
        const auto ip = position + Point<int16_t>(32, 106);
        const auto service = position + Point<int16_t>(440, 106);
        host = Textfield(Text::A18M, Text::LEFT, Color::Name::WHITE, {ip, ip + Point<int16_t>(370, 42)}, 15);
        port = Textfield(Text::A18M, Text::LEFT, Color::Name::WHITE, {service, service + Point<int16_t>(155, 42)}, 5);
        if (Setting<ServerConfigured>::get().load()) host.change_text(Setting<ServerIP>::get().load());
        port.change_text(Setting<ServerPort>::get().load());
        host.set_enter_callback([this](const std::string&) { host.set_state(Textfield::NORMAL); });
        port.set_enter_callback([this](const std::string&) { port.set_state(Textfield::NORMAL); });
        host.set_key_callback(KeyAction::TAB, [this] { port.set_state(Textfield::FOCUSED); });
        port.set_key_callback(KeyAction::TAB, [this] { host.set_state(Textfield::FOCUSED); });
        buttons[0] = std::make_unique<AreaButton>(Point<int16_t>(32, 190), Point<int16_t>(270, 48));
        buttons[1] = std::make_unique<AreaButton>(Point<int16_t>(326, 190), Point<int16_t>(282, 48));
        status = Text(Text::A13M, Text::LEFT, Color::Name::WHITE,
            "Enter your server's IPv4 address, or explore the login screen offline.");
        update();
    }

    void UIServerSelect::draw(float) const
    {
        auto& g = GraphicsGL::get();
        g.drawrectangle(0, 0, Constants::Constants::get().get_viewwidth(), Constants::Constants::get().get_viewheight(), .035f,.035f,.035f,1);
        g.drawrectangle(position.x(), position.y(), 640, 360, .09f,.09f,.09f,1);
        label("Connect to your MapleStory server", position + Point<int16_t>(32, 28), true);
        label("Server IP", position + Point<int16_t>(32, 80));
        label("Port", position + Point<int16_t>(440, 80));
        for (const auto& bounds : {host.get_bounds(), port.get_bounds()})
            g.drawrectangle(bounds.left()-6, bounds.top()-4, bounds.width()+12, bounds.height()+8, .19f,.19f,.19f,1);
        host.draw({0,0}); port.draw({0,0});
        const float connect = buttons.at(0)->get_state() == Button::MOUSEOVER ? .16f : 0.f;
        const float offline = buttons.at(1)->get_state() == Button::MOUSEOVER ? .38f : .22f;
        g.drawrectangle(position.x()+32, position.y()+190, 270, 48, .46f+connect,.23f+connect,.06f,1);
        g.drawrectangle(position.x()+326, position.y()+190, 282, 48, offline,offline,offline,1);
        label("Connect", position + Point<int16_t>(52, 205), true);
        label("Offline preview", position + Point<int16_t>(346, 205), true);
        status.draw(position + Point<int16_t>(32, 263));
        label("D-pad: select    Cross: choose    Circle: offline", position + Point<int16_t>(32, 308));
        label("Triangle: open keyboard", position + Point<int16_t>(32, 333));
    }

    void UIServerSelect::controller_hover(const ControllerTarget& target)
    {
        if (target.button < 0) {
            remove_cursor();
            const auto center = Point<int16_t>((target.bounds.left()+target.bounds.right())/2,
                (target.bounds.top()+target.bounds.bottom())/2);
            auto& field = host.get_bounds().contains(center) ? host : port;
            if (!UI::get().has_textfield()) field.set_state(Textfield::FOCUSED);
        } else UIElement::controller_hover(target);
    }
    void UIServerSelect::update() { host.update({0,0}); port.update({0,0}); }
    Cursor::State UIServerSelect::send_cursor(bool pressed, Point<int16_t> point)
    {
        if (auto state = host.send_cursor(point, pressed)) return state;
        if (auto state = port.send_cursor(point, pressed)) return state;
        return UIElement::send_cursor(pressed, point);
    }
    void UIServerSelect::send_key(int32_t, bool pressed, bool escape) { if (pressed && escape) choice = OFFLINE; }
    std::vector<UIElement::ControllerTarget> UIServerSelect::controller_targets() const
    {
        std::vector<ControllerTarget> result{{host.get_bounds()}, {port.get_bounds()}};
        auto controls = UIElement::controller_targets();
        result.insert(result.end(), controls.begin(), controls.end());
        return result;
    }
    Button::State UIServerSelect::button_pressed(uint16_t id)
    {
        if (id == 1) choice = OFFLINE;
        else if (valid_server_address(address(), service())) choice = CONNECT;
        else message("Enter a valid IPv4 address and a port from 1 to 65535.");
        return Button::NORMAL;
    }

    static const char* menu_names[] = {"Inventory", "Equipment", "Skills", "Stats", "Quests", "World map", "Friends", "Guild", "Chat", "Settings"};
    static const KeyAction::Id menu_actions[] = {KeyAction::ITEMS, KeyAction::EQUIPMENT, KeyAction::SKILLS,
        KeyAction::STATS, KeyAction::QUESTLOG, KeyAction::WORLDMAP, KeyAction::FRIENDS, KeyAction::GUILD, KeyAction::SAY, KeyAction::MAINMENU};
    UIControllerMenu::UIControllerMenu()
    {
        position = {(int16_t)((Constants::Constants::get().get_viewwidth()-380)/2), 75};
        dimension = {380, 475};
        for (int i = 0; i < 10; ++i)
            buttons[i] = std::make_unique<AreaButton>(Point<int16_t>(22, 64+i*36), Point<int16_t>(336, 30));
    }
    void UIControllerMenu::draw(float) const
    {
        GraphicsGL::get().drawrectangle(position.x(), position.y(), 380, 475, .07f,.07f,.07f,.98f);
        label("Game menu", position + Point<int16_t>(22, 24), true);
        for (int i = 0; i < 10; ++i) {
            if (buttons.at(i)->get_state() == Button::MOUSEOVER)
                GraphicsGL::get().drawrectangle(position.x()+22, position.y()+64+i*36, 336, 30, .25f,.25f,.25f,1);
            label(menu_names[i], position + Point<int16_t>(32, 71+i*36));
        }
        label("R1: switch open menus    Circle: close", position + Point<int16_t>(22, 443));
    }
    Button::State UIControllerMenu::button_pressed(uint16_t id)
    {
        if (id < 10) {
            deactivate();
            UI::get().controller_open(menu_actions[id]);
        }
        return Button::NORMAL;
    }
}
