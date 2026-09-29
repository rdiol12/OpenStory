#include "IO/ControllerNavigation.h"
#include "Net/ServerAddress.h"
#include <cassert>
#include <iostream>

int main()
{
    ms::ControllerNavigation focus;
    focus.enter(1, 3);
    assert(focus.index() == 0);
    for (auto now : {1000u, 1100u, 5000u})
        assert((focus.press(now, 3) && focus.index() == 0) &&
               "Cross must activate the current control without moving focus, regardless of timing");
    focus.enter(2, 4);
    focus.step(1, 4); // Host -> Port.
    focus.step(1, 4); // Port -> Connect.
    focus.enter(2, 4); // The per-frame focus refresh must preserve explicit selection.
    assert(focus.index() == 2);
    assert((focus.press(5000, 4) && focus.index() == 2) &&
           "first Cross after D-pad selection must activate Connect, not advance to Offline");
    assert(focus.press(5100, 4) && focus.index() == 2);
    assert(focus.press(5200, 4) && focus.index() == 2);

    focus.enter(3, 4);
    focus.step(-1, 4);
    focus.step(1, 4); // Select Host with D-pad and open its keyboard.
    assert(focus.press(6000, 4) && focus.index() == 0);
    focus.enter(3, 4); // Returning from the keyboard keeps the same menu/selection.
    assert(focus.index() == 0);
    assert(focus.press(19000, 4) && focus.index() == 0); // Reopen the same field.
    focus.step(1, 4);
    focus.step(1, 4);
    focus.enter(3, 4);
    assert(focus.press(20000, 4) && focus.index() == 2); // Independent of double-tap timeout.

    focus.enter(4, 4);
    assert(focus.press(21000, 4) && focus.index() == 0);
    focus.step(1, 4);
    assert(focus.press(21100, 4) && focus.index() == 1);
    assert(focus.press(21200, 4) && focus.index() == 1);
    focus.enter(5, 4);
    assert(focus.press(21300, 4) && focus.index() == 0);
    focus.step(1, 4);
    focus.enter(6, 4);
    assert(focus.press(21400, 4) && focus.index() == 0);
    focus.step(-1, 4);
    assert(focus.index() == 3);
    focus.enter(6, 1);
    assert(focus.index() == 0);
    assert(focus.press(22000, 1));
    focus.enter(6, 0);
    assert(!focus.press(23000, 0));
    focus.step(-1, 0);
    assert(focus.index() == 0);
    assert(ms::valid_server_address("192.0.2.1", "8484"));
    assert(ms::valid_server_address("127.0.0.1", "65535"));
    for (const char* ip : {"", "server.example", "256.1.2.3", "1.2.3", "1.2.3.4\n", "0.0.0.0", "255.255.255.255", "224.0.0.1"})
        assert(!ms::valid_server_address(ip, "8484"));
    for (const char* port : {"", "0", "65536", "8484x", "-1", " 8484"})
        assert(!ms::valid_server_address("192.0.2.1", port));
    std::cout << "Controller consistent Cross activation, D-pad navigation, keyboard return, menu isolation and server validation passed\n";
}
