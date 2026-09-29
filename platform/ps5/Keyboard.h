#pragma once
#include <cstdint>
namespace ms::native_keyboard {
    bool open();
    bool busy();
    // Returns true if the platform dialog failed and the fallback is needed.
    bool poll(bool activation_held, uint32_t now);
    void shutdown();
}
