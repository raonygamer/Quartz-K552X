#pragma once
#include <cstdint>

namespace quartz::rpc {
    enum class PacketType : std::uint16_t {
        Invalid = 0,
        Ping = 1,
        Pong = 2,
        GetPerfStatistics = 3,
        PerfStatistics = 4,
        SetRGBMatrix = 5,
        Error = 0xFFFFu
    };

    enum class PacketDirection : std::uint8_t
    {
        DeviceToHost = 0,
        HostToDevice = 1
    };
}