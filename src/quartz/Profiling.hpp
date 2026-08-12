#pragma once
#include <cstdint>

namespace quartz::profiling
{
    constexpr std::uint32_t RawTickMask = 0x00FFFFFFu;
    inline std::uint32_t BeginScanTicks = 0;
    inline std::uint32_t ScanTicks = 0;
    inline std::uint32_t EndScanTicks = 0;
    inline std::uint32_t StateUpdateTicks = 0;
    inline std::uint32_t HIDTicks = 0;
    inline std::uint32_t RGBTicks = 0;
    inline std::uint32_t AverageScanPeriodTicks = 0;
    inline std::uint32_t RGBSlotMaxTicks = 0;
}