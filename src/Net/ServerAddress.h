#pragma once
#include <asio/ip/address_v4.hpp>
#include <charconv>
#include <string>

namespace ms
{
    inline bool valid_server_address(const std::string& host, const std::string& port)
    {
        asio::error_code error;
        const auto address = asio::ip::make_address_v4(host, error);
        unsigned number = 0;
        const auto parsed = std::from_chars(port.data(), port.data() + port.size(), number);
        return !error && !address.is_unspecified() && !address.is_multicast()
            && address.to_uint() != 0xffffffffu && parsed.ec == std::errc{}
            && parsed.ptr == port.data() + port.size() && number > 0 && number <= 65535;
    }
}
