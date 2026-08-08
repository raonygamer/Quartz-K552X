#pragma once
#include <cstdint>

namespace quartz::usb::hid {
    enum class Protocol : std::uint8_t {
        Boot = 0,
        Report = 1
    };
}