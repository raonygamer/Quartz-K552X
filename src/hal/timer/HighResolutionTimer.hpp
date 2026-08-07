#pragma once
#include <cstdint>
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal {
    class HighResolutionTimer {
        inline static volatile std::uint64_t OverflowCount = 0;
    public:
        static constexpr std::uint32_t TicksPerMicrosecond = 48;
        static constexpr std::uint32_t TicksPerMillisecond = 48'000;

        static void initialize() noexcept;
        static std::uint32_t readHardwareTicks() noexcept;
        static std::uint64_t nowTicks() noexcept;
        static std::uint64_t nowMicroseconds() noexcept;
        static std::uint64_t nowMilliseconds() noexcept;
        static void waitTicks(std::uint64_t ticks) noexcept;
        static void waitMilliseconds(std::uint64_t milliseconds) noexcept;
        static void waitMicroseconds(std::uint64_t microseconds) noexcept;
        static void handleOverflow() noexcept;
        static std::uint64_t getOverflowCount() noexcept {
            return OverflowCount;
        }
    };
}