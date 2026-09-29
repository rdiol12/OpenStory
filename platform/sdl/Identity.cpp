#include "Configuration.h"
#include "Util/Paths.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <filesystem>
#ifdef PLATFORM_PS5
#include <stdlib.h>
#else
#include <random>
#endif

// The login protocol expects a Windows-style identifier. Keep one per install.
bool init_sdl_identity()
{
    std::string id;
    const auto identity_file = ms::data_path("ClientID.txt");
    if (std::filesystem::exists(identity_file)) {
        std::ifstream input(identity_file);
        input >> id;
    } else {
        unsigned char bytes[10];
#ifdef PLATFORM_PS5
        arc4random_buf(bytes, sizeof(bytes));
#else
        std::random_device random;
        for (auto& byte : bytes) byte = static_cast<unsigned char>(random());
#endif
        const char* hex = "0123456789ABCDEF";
        for (auto byte : bytes) { id += hex[byte >> 4]; id += hex[byte & 15]; }
        std::ofstream output(identity_file);
        output << id << '\n';
        output.close();
        if (!output) { std::cerr << "Cannot save ClientID.txt\n"; return false; }
    }
    if (id.size() != 20 || id.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        std::cerr << "Invalid ClientID.txt (expected 20 hexadecimal characters)\n";
        return false;
    }
    std::string hardware = id.substr(0, 12), serial = id.substr(12);
    std::string mac;
    for (int i = 0; i < 12; i += 2) { if (i) mac += '-'; mac += id.substr(i, 2); }
    ms::Configuration::get().set_hwid(hardware.data(), serial.data());
    ms::Configuration::get().set_macs(mac.data());
    return true;
}
