#pragma once
#include <array>
#include <cstdint>

namespace quartz::hal
{
    class System
    {
    public:
        static constexpr std::array<std::uint8_t, 8> SONIX_REBOOT_MAGIC = {
            0xAA,
            0x55,
            0xA5,
            0x5A,
            0xFF,
            0x00,
            0x33,
            0xCC,
        };

        static void initializeSystemTick() noexcept;
        static void teardownEverything() noexcept;
        [[noreturn]]
        static void toBootloader(bool teardown = true) noexcept;
        [[noreturn]]
        static void reset() noexcept;
    };
}

extern "C" void SysTick_Handler();
extern "C" void USB_IRQHandler();