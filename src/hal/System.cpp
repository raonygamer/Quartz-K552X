#include "hal/System.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "hal/usb/Controller.hpp"
#include "hal/usb/Interrupt.hpp"
#include "kb/Matrix.hpp"
#include "usb/Device.hpp"

namespace quartz::hal
{
    void System::initializeSystemTick() noexcept
    {
        SysTick->CTRL = 0; // Disable SysTick
        SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
        SysTick->VAL = 0; // Clear current value
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
        // Enable SysTick with interrupt
    }

    void System::teardownEverything() noexcept
    {
        quartz::usb::Device::reset();
        usb::Controller::reset();

        SysTick->CTRL = 0u;
        SysTick->LOAD = 0u;
        SysTick->VAL = 0u;
        SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
        SN_GPIO0->MODE = 0u;
        SN_GPIO1->MODE = 0u;
        SN_GPIO2->MODE = 0u;
        SN_GPIO3->MODE = 0u;

        NVIC_DisableIRQ(CT16B0_IRQn);
        NVIC_ClearPendingIRQ(CT16B0_IRQn);

        SN_CT16B0->TMRCTRL = 0u;
        SN_CT16B0->IC = 1u;

        SN_SYS0->IVTM = 0;
    }

    void System::toBootloader() noexcept
    {
        teardownEverything();
        __asm volatile(
            "cpsid i\n"
            "dsb\n"
            "isb\n"
            "ldr r0, =0x1FFF0301\n"
            "bx r0\n"
        );

        __builtin_unreachable();
    }

    void System::reset() noexcept
    {
        HighResolutionTimer::waitMilliseconds(100);
        debug::Panic::blinkDebuggingLeds(25, 10);
        teardownEverything();
        HighResolutionTimer::waitMilliseconds(100);
        debug::Panic::setDebuggingLedState(false);
        NVIC_SystemReset();
        __builtin_unreachable();
    }
}

extern "C" void SysTick_Handler()
{
    // quartz::hal::Timer::tick();
}

extern "C" void USB_IRQHandler()
{
    const auto status = quartz::hal::usb::Controller::getInterruptStatus();
    quartz::usb::Device::handleInterrupt(status);
}

extern "C" void HardFault_Handler()
{
    quartz::debug::Panic::captureState();
    quartz::debug::Panic::incrementPanicCount();
    quartz::hal::HighResolutionTimer::waitMilliseconds(100);
    quartz::debug::Panic::blinkDebuggingLeds(100, 20);
    quartz::debug::Panic::setDebuggingLedState(false);
    quartz::hal::HighResolutionTimer::waitMilliseconds(100);

    quartz::hal::System::teardownEverything();
    NVIC_SystemReset();
    __builtin_unreachable();
}
