#include "RPC.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "quartz/Profiling.hpp"
#include "quartz/rpc/handlers/FramebufferSetPacketHandler.hpp"
#include "quartz/rpc/handlers/PerformanceRequestPacketHandler.hpp"
#include "quartz/rpc/handlers/PingPacketHandler.hpp"
#include "quartz/rpc/payloads/FramebufferSetPayload.hpp"
#include "quartz/rpc/payloads/PerformancePayload.hpp"
#include "quartz/utils/Alignment.hpp"
#include "usb/Device.hpp"
#include "usb/protocol/pipes/TransferPipe.hpp"
#include "utils/Time.hpp"
#include <cstring>

namespace quartz::rpc
{
    PROTOCOL_ALIGNED std::uint8_t RPC::TxBuffer[TxBufferSize];
    PROTOCOL_ALIGNED std::uint8_t RPC::RxBuffer[RxBufferSize];

    void RPC::_handlePacket(const std::span<const std::byte> buff) noexcept
    {
        const auto header = PacketHeader::asPtr(buff);
        if (!header || header->Direction != PacketDirection::HostToDevice || header->Version != 1)
            return;
        if (header->PayloadLength + sizeof(PacketHeader) != buff.size())
            return;
        switch (header->Type)
        {
        case PacketType::Ping:
            return handlers::PingPacketHandler::handle(*header);
        case PacketType::PerformanceRequest:
            return handlers::PerformanceRequestPacketHandler::handle(*header);
        case PacketType::FramebufferSet:
            return handlers::FramebufferSetPacketHandler::handle(*header);
        default:
            break;
        }
    }

    void RPC::send(const PacketType type, const std::uint32_t responseFor, const std::span<const std::byte> payload) noexcept
    {
        if (!usb::Device::isConfigured())
            return;
        if (!_waitUntilTransmitIdle(100))
            return;
        const std::size_t totalSize = sizeof(PacketHeader) + payload.size();
        if (totalSize > sizeof(TxBuffer))
            return;

        new (TxBuffer) PacketHeader(
            1,
            type,
            PacketDirection::DeviceToHost,
            responseFor,
            payload.size()
        );

        if (payload.data() && payload.size() > 0)
            std::memcpy(TxBuffer + sizeof(PacketHeader), payload.data(), payload.size());

        usb::proto::TransferPipe::startTransferIn(
            hal::usb::EndpointNumber::EP4,
            std::as_bytes(std::span{ TxBuffer, totalSize })
        );
    }

    void RPC::initialize() noexcept
    {
        _armToReceive();
    }

    void RPC::handleReceiveComplete() noexcept
    {
        const std::size_t size = usb::proto::TransferPipe::getTransferredOutSize(hal::usb::EndpointNumber::EP3);
        _handlePacket(std::as_bytes(std::span{ RxBuffer, size }));
        _armToReceive();
    }

    bool RPC::_waitUntilTransmitIdle(const std::uint64_t timeoutMs) noexcept
    {
        const auto start = hal::HighResolutionTimer::nowTicks();
        const auto timeout = timeoutMs * hal::HighResolutionTimer::TicksPerMillisecond;
        while (usb::proto::TransferPipe::isTransferring(TxEndpoint))
        {
            if (hal::HighResolutionTimer::nowTicks() - start >= timeout)
                return false;

            __WFI();
        }

        return true;
    }

    void RPC::_armToReceive() noexcept
    {
        usb::proto::TransferPipe::startTransferOut(
            RxEndpoint,
            std::as_writable_bytes(std::span{ RxBuffer }),
            0
        );
    }
}
