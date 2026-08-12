#pragma once
#include <cstdint>

namespace quartz::utils
{
    struct Time
    {
        constexpr static std::uint64_t SystemCoreClock = 48'000'000ull;

        [[gnu::always_inline]]
        static constexpr std::uint64_t ticksToMicroseconds(const std::uint64_t ticks) noexcept
        {
            return static_cast<std::uint64_t>(ticks) * 1'000'000ULL / SystemCoreClock;
        }

        [[gnu::always_inline]]
        static constexpr std::uint64_t microsecondsToTicks(const std::uint64_t microseconds) noexcept
        {
            return static_cast<std::uint64_t>(microseconds) * SystemCoreClock / 1'000'000ULL;
        }

        [[gnu::always_inline]]
        static constexpr std::uint64_t millisecondsToTicks(const std::uint64_t milliseconds) noexcept
        {
            return static_cast<std::uint64_t>(milliseconds) * SystemCoreClock / 1'000ULL;
        }
    };
}