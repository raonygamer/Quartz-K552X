#include "Timer.hpp"

namespace quartz::hal
{
    volatile std::uint32_t Timer::CurrentTick = 0;

    void Timer::tick() noexcept
    {
        CurrentTick = CurrentTick + 1u;
    }

    void Timer::wait(std::uint32_t ms) noexcept
    {
        const std::uint32_t startTick = CurrentTick;
        while ((CurrentTick - startTick) < ms)
        {
            __NOP(); // Wait for interrupt to save power while waiting
        }
    }
}