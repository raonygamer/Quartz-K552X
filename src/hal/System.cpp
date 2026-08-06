#include "System.hpp"
#include "timer/Timer.hpp"
#include "usb/USB.hpp"
#include "../usb/Interrupt.hpp"
#include "../usb/Device.hpp"

namespace quartz::hal {
    void System::initializeSystemTick() noexcept
    {
        SysTick->CTRL = 0; // Disable SysTick
        SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
        SysTick->VAL = 0; // Clear current value
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick with interrupt
    }

    void System::toBootloader() noexcept
    {
        hal::USB::prepareForBootloader();

        SysTick->CTRL = 0u;
        SysTick->LOAD = 0u;
        SysTick->VAL  = 0u;
        NVIC_ClearPendingIRQ(SysTick_IRQn);

        SN_SYS0->IVTM = 0;

        __asm volatile(
            "cpsid i\n"
            "dsb\n"
            "isb\n"
            "ldr r0, =0x1FFF0301\n"
            "bx r0\n"
        );

        __builtin_unreachable();
    }
}

extern "C" void SysTick_Handler()
{
    quartz::hal::Timer::tick();
}

extern "C" void USB_IRQHandler()
{
    quartz::usb::Device::handleInterrupt();
}