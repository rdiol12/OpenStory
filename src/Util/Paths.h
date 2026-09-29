#pragma once
#include <string>

namespace ms
{
    // Native titles use explicit sandbox paths; changing cwd faults on the PS5.
    inline std::string data_path(const std::string& name)
    {
#ifdef PLATFORM_PS5
        if (name.empty() || name.front() != '/') return "/download0/openstory/" + name;
#endif
        return name;
    }
}
