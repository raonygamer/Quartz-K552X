#include "HighResolutionTimer.hpp"

namespace quartz::hal
{
    void HighResolutionTimer::initialize() noexcept
    {
        SN_SYS1->AHBCLKEN |= (1u << 6);

        SN_CT16B0->TMRCTRL = 0u;
        SN_CT16B0->PRE = 255u;

        // Start with match IRQ disabled.
        SN_CT16B0->MCTRL = 0u;

        SN_CT16B0->TMRCTRL = (1u << 1);
        while (SN_CT16B0->TMRCTRL & (1u << 1)) {
        }

        OverflowCount = 0;

        SN_CT16B0->TMRCTRL = 1u;

        // Get away from the initial TC == 0 so that MR0=0
        // represents the NEXT natural rollover.
        while (SN_CT16B0->TC == 0u) {
        }

        SN_CT16B0->MR0 = 0u;
        SN_CT16B0->IC = 1u;

        // MR0IE = 1
        // MR0RST = 0
        // MR0STOP = 0
        SN_CT16B0->MCTRL = 1u;

        NVIC_ClearPendingIRQ(CT16B0_IRQn);
        NVIC_EnableIRQ(CT16B0_IRQn);
    }

    std::uint32_t HighResolutionTimer::readHardwareTicks() noexcept
    {
        std::uint16_t tc1;
        std::uint16_t tc2;
        std::uint8_t pc;

        do {
            tc1 = static_cast<std::uint16_t>(SN_CT16B0->TC);
            pc  = static_cast<std::uint8_t>(SN_CT16B0->PC);
            tc2 = static_cast<std::uint16_t>(SN_CT16B0->TC);
        } while (tc1 != tc2);

        return
            (static_cast<std::uint32_t>(tc2) << 8) |
            pc;
    }

    std::uint64_t HighResolutionTimer::nowTicks() noexcept
    {
        const std::uint32_t primask = __get_PRIMASK();
        __disable_irq();

        if (SN_CT16B0->RIS & 1u) {
            OverflowCount = OverflowCount + 1u;
            SN_CT16B0->IC = 1u;
        }

        std::uint64_t high = OverflowCount;
        std::uint32_t low = readHardwareTicks();

        // Catch a rollover that occurred while taking the snapshot.
        if (SN_CT16B0->RIS & 1u) {
            OverflowCount = OverflowCount + 1u;
            SN_CT16B0->IC = 1u;

            high = OverflowCount;
            low = readHardwareTicks();
        }

        if (!primask) {
            __enable_irq();
        }

        return (high << 24) | low;
    }

    std::uint64_t HighResolutionTimer::nowMicroseconds() noexcept
    {
        return nowTicks() / 48u;
    }

    std::uint64_t HighResolutionTimer::nowMilliseconds() noexcept
    {
        return nowTicks() / 48'000u;
    }

    void HighResolutionTimer::waitTicks(const std::uint64_t ticks) noexcept
    {
        std::uint64_t elapsed = 0;
        std::uint32_t previous = readHardwareTicks();

        while (elapsed < ticks) {
            const std::uint32_t current = readHardwareTicks();

            elapsed +=
                (current - previous) & 0x00FFFFFFu;

            previous = current;

            __NOP();
        }
    }

    void HighResolutionTimer::waitMicroseconds(const std::uint64_t microseconds) noexcept
    {
        constexpr std::uint64_t TicksPerMicrosecond = 48u;
        waitTicks(microseconds * TicksPerMicrosecond);
    }

    void HighResolutionTimer::waitMilliseconds(const std::uint64_t milliseconds) noexcept
    {
        constexpr std::uint64_t TicksPerMillisecond = 48'000u;
        waitTicks(milliseconds * TicksPerMillisecond);
    }

    void HighResolutionTimer::handleOverflow() noexcept
    {
        if (SN_CT16B0->RIS & 1u)
        {
            OverflowCount = OverflowCount + 1u;
            SN_CT16B0->IC = 1u;
        }
    }
}

extern "C" void CT16B0_IRQHandler()
{
    quartz::hal::HighResolutionTimer::handleOverflow();
}