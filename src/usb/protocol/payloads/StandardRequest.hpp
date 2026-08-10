#pragma once
#include <cstdint>

namespace quartz::usb::payloads
{
    struct StandardRequest
    {
        static constexpr std::uint8_t SET_ADDRESS = 0x05;
        static constexpr std::uint8_t GET_DESCRIPTOR = 0x06;
        static constexpr std::uint8_t GET_CONFIGURATION = 0x08;
        static constexpr std::uint8_t SET_CONFIGURATION = 0x09;
    };
}