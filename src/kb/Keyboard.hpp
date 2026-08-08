#pragma once
#include <cstdint>

namespace quartz::kb {
    class Keyboard {
    public:
        inline static std::uint64_t LastScanTicks = 0;
        inline static std::uint32_t ScanRate = 0;
        inline static std::uint32_t CPUScanUsage = 0;
        inline static std::uint32_t StateUpdateTicks = 0;
        inline static std::uint32_t HIDTicks = 0;
        inline static std::uint32_t ScanDurationTicks = 0;
        inline static std::uint32_t ScanPeriodTicks = 0;

        static void scanAndSend();
    };
}