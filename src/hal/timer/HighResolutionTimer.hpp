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
        [[gnu::always_inline]]
        inline static std::uint32_t rawTicks() noexcept 
        {
            std::uint16_t tc1;
            std::uint16_t tc2;
            std::uint8_t pc;

            do {
                tc1 = static_cast<std::uint16_t>(SN_CT16B0->TC);
                pc = static_cast<std::uint8_t>(SN_CT16B0->PC);
                tc2 = static_cast<std::uint16_t>(SN_CT16B0->TC);
            } while (tc1 != tc2);

            return (static_cast<std::uint32_t>(tc2) << 8u) | pc;
        }
        
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