#pragma once
#include <cstdint>

namespace quartz::profiling {
    inline std::uint32_t BeginScanTicks = 0;
    inline std::uint32_t ScanTicks = 0;
    inline std::uint32_t EndScanTicks = 0;
    inline std::uint32_t StateUpdateTicks = 0;
    inline std::uint32_t HIDTicks = 0;
    inline std::uint32_t AverageScanPeriodTicks = 0;
}