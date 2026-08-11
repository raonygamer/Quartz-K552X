#include "RPC.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "quartz/Profiling.hpp"
#include "quartz/rpc/payloads/LEDFramebufferSetPayload.hpp"
#include "quartz/rpc/payloads/PerformancePayload.hpp"
#include "usb/Device.hpp"
#include "usb/protocol/pipes/TransferPipe.hpp"
#include "utils/Time.hpp"
#include <cstring>

namespace quartz::rpc
{
    namespace
    {
        constexpr std::size_t TxBufferSize = 512;
        constexpr std::size_t RxBufferSize = 512;

        std::uint8_t TxBuffer[TxBufferSize]{};
        std::uint8_t RxBuffer[RxBufferSize]{};

        void armReceive() noexcept
        {
            usb::proto::TransferPipe::startTransferOut(
                hal::usb::EndpointNumber::EP3,
                std::as_writable_bytes(std::span{ RxBuffer }),
                0
            );
        }
    }

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
            _handlePingPacket(*header);
            break;
        case PacketType::PerformanceRequest:
            _handleGetPerfStatisticsPacket(*header);
            break;
        case PacketType::LEDFramebufferSet:
            _handleSetRGBMatrixPacket(*header);
            break;
        default:
            break;
        }
    }

    void RPC::_handlePingPacket(const PacketHeader& header) noexcept
    {
        if (header.PayloadLength != 0)
            return;
        send(PacketType::Pong, {});
    }

    void RPC::_handleGetPerfStatisticsPacket(const PacketHeader& header) noexcept
    {
        if (header.PayloadLength != 0)
            return;
        _sendPerfStatisticsPacket();
    }

    void RPC::_handleSetRGBMatrixPacket(const PacketHeader& header) noexcept
    {
        const auto* payload = header.getPayload<payloads::LEDFramebufferSetPayload<kb::MatrixDefinitions::Size>>();
        if (!payload)
            return;
        kb::rgb::RGBMatrix::setFramebuffer(payload->Framebuffer.data(), kb::MatrixDefinitions::Size);
    }

    void RPC::_sendPerfStatisticsPacket() noexcept
    {
        const payloads::PerformancePayload packet = {
            .CoreClock = utils::Time::SystemCoreClock,
            .BeginScanTicks = profiling::BeginScanTicks,
            .ScanTicks = profiling::ScanTicks,
            .EndScanTicks = profiling::EndScanTicks,
            .StateUpdateTicks = profiling::StateUpdateTicks,
            .HIDTicks = profiling::HIDTicks,
            .AverageScanPeriodTicks = profiling::AverageScanPeriodTicks
        };

        send(PacketType::PerformanceResponse, std::as_bytes(std::span{ &packet, 1 }));
    }

    void RPC::send(const PacketType type, const std::span<const std::byte> payload) noexcept
    {
        if (!usb::Device::isConfigured())
            return;
        if (!_waitUntilTransmitIdle(100))
            return;
        const std::size_t totalSize = sizeof(PacketHeader) + payload.size();
        if (totalSize > sizeof(TxBuffer))
            return;
        const PacketHeader header(1, type, PacketDirection::DeviceToHost, payload.size());
        std::memcpy(TxBuffer, &header, sizeof(header));
        if (payload.data() && payload.size() > 0)
            std::memcpy(TxBuffer + sizeof(header), payload.data(), payload.size());

        usb::proto::TransferPipe::startTransferIn(
            hal::usb::EndpointNumber::EP4,
            std::as_bytes(std::span{ TxBuffer, totalSize })
        );
    }

    void RPC::initialize() noexcept
    {
        armReceive();
    }

    void RPC::handleReceiveComplete() noexcept
    {
        const std::size_t size = usb::proto::TransferPipe::getTransferredOutSize(hal::usb::EndpointNumber::EP3);
        _handlePacket(std::as_bytes(std::span{ RxBuffer, size }));
        armReceive();
    }

    void RPC::handleTransmitComplete() noexcept
    {
    }

    bool RPC::_waitUntilTransmitIdle(const std::uint64_t timeoutMs) noexcept
    {
        const auto start = hal::HighResolutionTimer::nowTicks();
        const auto timeout = timeoutMs * hal::HighResolutionTimer::TicksPerMillisecond;
        while (usb::proto::TransferPipe::isTransferring(hal::usb::EndpointNumber::EP4))
        {
            if (hal::HighResolutionTimer::nowTicks() - start >= timeout)
                return false;

            __WFI();
        }

        return true;
    }
}
