#pragma once
#include <cstdint>
#include "packets/PacketHeader.hpp"

namespace quartz::rpc {
    class RPC {
    public:
        static constexpr std::uint32_t Magic = 0x51525043u; // "QRPC"
        static void handlePacket(const std::uint8_t* data, std::size_t size) noexcept;
        static void handlePingPacket(const PacketHeader& header) noexcept;
        static void handleGetPerfStatisticsPacket(const PacketHeader& header) noexcept;
        static void handleSetRGBMatrixPacket(const PacketHeader& header, const std::uint8_t* payload) noexcept;
        static void sendPerfStatisticsPacket() noexcept;
        static void send(PacketType type, const std::uint8_t* payload, std::size_t payloadSize) noexcept;
    };
}