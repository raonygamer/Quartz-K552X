#pragma once
#include "hal/usb/Endpoint.hpp"
#include "quartz/rpc/PacketHeader.hpp"
#include <cstdint>

namespace quartz::rpc
{
    class RPC
    {
        static constexpr auto TxEndpoint = hal::usb::EndpointNumber::EP4;
        static constexpr auto RxEndpoint = hal::usb::EndpointNumber::EP3;
        static constexpr std::size_t TxBufferSize = 512;
        static constexpr std::size_t RxBufferSize = 512;
        static std::uint8_t TxBuffer[TxBufferSize];
        static std::uint8_t RxBuffer[RxBufferSize];
    public:
        static constexpr std::uint32_t Magic = 0x51525043u; // "QRPC"
        static void initialize() noexcept;
        static void handleReceiveComplete() noexcept;
        static void send(PacketType type, std::uint32_t responseFor = 0, std::span<const std::byte> payload = {}) noexcept;

    private:
        static void _handlePacket(std::span<const std::byte> buff) noexcept;
        static bool _waitUntilTransmitIdle(std::uint64_t timeoutMs) noexcept;
        static void _armToReceive() noexcept;
    };
}
