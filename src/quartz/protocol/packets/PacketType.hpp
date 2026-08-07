#pragma once
#include <cstdint>

namespace quartz::protocol {
    enum class PacketType : std::uint8_t {
        Invalid = 0x00,
        // 0x01 - 0x0F: Reserved
        DebugMessage = 0x10
    };
}