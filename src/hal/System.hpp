#pragma once
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal {
    class System {
    public:
        static void initializeSystemTick() noexcept;
        static void teardownEverything() noexcept;
        [[noreturn]]
        static void toBootloader() noexcept;
        [[noreturn]]
        static void reset() noexcept;
    };
}

extern "C" void SysTick_Handler();
extern "C" void USB_IRQHandler();