#pragma once
#include <cstdint>
#include "PacketType.hpp"

namespace quartz::rpc {
    struct [[gnu::packed]] PacketHeader
    {
        std::uint32_t Magic;
        std::uint8_t Version;
        PacketType Type;
        PacketDirection Direction;
        std::uint16_t PayloadLength;
    };

    static_assert(sizeof(PacketHeader) == 10, "PacketHeader must be 10 bytes");
}