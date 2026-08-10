#include "RPC.hpp"
#include <cstring>
#include "packets/PacketHeader.hpp"
#include "packets/PerfStatisticsPacket.hpp"
#include "usb/Device.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "quartz/Profiling.hpp"
#include "utils/Color32.hpp"

namespace quartz::rpc
{
    namespace
    {
        constexpr std::size_t MaxPacketSize = 128;
        std::uint8_t TxBuffer[MaxPacketSize]{};
    }

    void RPC::handlePacket(const std::uint8_t* data, const std::size_t size) noexcept
    {
        if (size < sizeof(PacketHeader))
            return;

        PacketHeader header{};
        std::memcpy(&header, data, sizeof(header));

        if (header.Magic != Magic)
            return;

        if (header.Direction != PacketDirection::HostToDevice)
            return;

        if (header.Version != 1)
            return;

        if (static_cast<std::size_t>(header.PayloadLength) + sizeof(PacketHeader) != size)
            return;

        const std::uint8_t* payload = data + sizeof(PacketHeader);

        switch (header.Type) {
            case PacketType::Ping:
                handlePingPacket(header);
                break;

            case PacketType::GetPerfStatistics:
                handleGetPerfStatisticsPacket(header);
                break;

            case PacketType::SetRGBMatrix:
                handleSetRGBMatrixPacket(header, payload);
                break;

            default:
                break;
        }
    }

    void RPC::handlePingPacket(const PacketHeader& header) noexcept
    {
        if (header.PayloadLength != 0)
            return;
        send(PacketType::Pong, nullptr, 0);
    }

    void RPC::handleGetPerfStatisticsPacket(const PacketHeader& header) noexcept
    {
        if (header.PayloadLength != 0)
            return;
        sendPerfStatisticsPacket();
    }

    void RPC::handleSetRGBMatrixPacket(const PacketHeader& header, const std::uint8_t* payload) noexcept
    {
        if (header.PayloadLength != kb::MatrixDefinitions::Size * sizeof(utils::Color32))
            return;

        const utils::Color32* colors = reinterpret_cast<const utils::Color32*>(payload);
        kb::rgb::RGBMatrix::setFramebuffer(colors, kb::MatrixDefinitions::Size);
    }

    void RPC::sendPerfStatisticsPacket() noexcept
    {
        PerfStatisticsPacket packet{
            .CoreClock = utils::Time::SystemCoreClock,
            .BeginScanTicks = quartz::profiling::BeginScanTicks,
            .ScanTicks = quartz::profiling::ScanTicks,
            .EndScanTicks = quartz::profiling::EndScanTicks,
            .StateUpdateTicks = quartz::profiling::StateUpdateTicks,
            .HIDTicks = quartz::profiling::HIDTicks,
            .AverageScanPeriodTicks = quartz::profiling::AverageScanPeriodTicks
        };

        send(PacketType::PerfStatistics, reinterpret_cast<const std::uint8_t*>(&packet), sizeof(packet));
    }

    void RPC::send(
        const PacketType type,
        const std::uint8_t* payload,
        const std::size_t payloadSize
    ) noexcept
    {
        if (!usb::Device::isConfigured())
            return;

        if (payloadSize != 0 && payload == nullptr)
            return;

        const std::size_t totalSize = sizeof(PacketHeader) + payloadSize;

        if (totalSize > sizeof(TxBuffer))
            return;

        PacketHeader header{
            .Magic = Magic,
            .Version = 1,
            .Type = type,
            .Direction = PacketDirection::DeviceToHost,
            .PayloadLength = static_cast<std::uint16_t>(payloadSize)
        };

        std::memcpy(TxBuffer, &header, sizeof(header));

        if (payloadSize != 0)
            std::memcpy(TxBuffer + sizeof(header), payload, payloadSize);

        //usb::Device::sendRPCData(TxBuffer, totalSize);
    }
    
}