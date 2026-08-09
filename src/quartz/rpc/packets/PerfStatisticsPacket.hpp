#pragma once
#include <cstdint>
#include "utils/Time.hpp"

namespace quartz::rpc {
    struct [[gnu::packed]] PerfStatisticsPacket
    {
        std::uint32_t CoreClock = utils::Time::SystemCoreClock;
        std::uint32_t BeginScanTicks;
        std::uint32_t ScanTicks;
        std::uint32_t EndScanTicks;
        std::uint32_t StateUpdateTicks;
        std::uint32_t HIDTicks;
        std::uint32_t AverageScanPeriodTicks;
    };

    static_assert(sizeof(PerfStatisticsPacket) == 28);
}