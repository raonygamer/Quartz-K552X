#include "quartz/rpc/senders/PerformanceResponsePacketSender.hpp"

#include "quartz/Profiling.hpp"
#include "quartz/rpc/RPC.hpp"
#include "quartz/rpc/payloads/PerformancePayload.hpp"
#include "utils/Time.hpp"

namespace quartz::rpc::senders
{
    void PerformanceResponsePacketSender::send(const std::uint32_t responseFor)
    {
        const payloads::PerformancePayload packet = {
            .CoreClock = utils::Time::SystemCoreClock,
            .BeginScanTicks = profiling::BeginScanTicks,
            .ScanTicks = profiling::ScanTicks,
            .EndScanTicks = profiling::EndScanTicks,
            .StateUpdateTicks = profiling::StateUpdateTicks,
            .HIDTicks = profiling::HIDTicks,
            .RGBTicks = profiling::RGBTicks,
            .AverageScanPeriodTicks = profiling::AverageScanPeriodTicks,
            .RGBSlotMaxTicks = profiling::RGBSlotMaxTicks,
        };
        profiling::RGBSlotMaxTicks = 0;
        RPC::send(PacketType::PerformanceResponse, responseFor, std::as_bytes(std::span{ &packet, 1 }));
    }
}