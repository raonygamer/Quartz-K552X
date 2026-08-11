#pragma once
#include "quartz/rpc/PacketHeader.hpp"
#include <cstdint>

namespace quartz::rpc
{
    class RPC
    {
    public:
        static constexpr std::uint32_t Magic = 0x51525043u; // "QRPC"
        static void initialize() noexcept;
        static void handleReceiveComplete() noexcept;
        static void handleTransmitComplete() noexcept;
        static void send(PacketType type, std::span<const std::byte> payload) noexcept;

    private:
        static void _handlePacket(std::span<const std::byte> buff) noexcept;
        static void _handlePingPacket(const PacketHeader& header) noexcept;
        static void _handleGetPerfStatisticsPacket(const PacketHeader& header) noexcept;
        static void _handleSetRGBMatrixPacket(const PacketHeader& header) noexcept;
        static void _sendPerfStatisticsPacket() noexcept;
        static bool _waitUntilTransmitIdle(std::uint64_t timeoutMs) noexcept;
    };
}
