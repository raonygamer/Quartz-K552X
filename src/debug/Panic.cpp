#include "Panic.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/timer/HighResolutionTimer.hpp"

namespace quartz::debug {
    void Panic::captureState() noexcept
    {
        state.MagicNumber = State::Magic;
        state.ProgramCounter = reinterpret_cast<std::uint32_t>(__builtin_return_address(0));
        state.LinkRegister = reinterpret_cast<std::uint32_t>(__builtin_return_address(1));
        state.StackPointer = reinterpret_cast<std::uint32_t>(__builtin_frame_address(0));
    }

    void Panic::blinkDebuggingLeds(std::uint16_t delayMs, std::uint8_t count) noexcept
    {
        setDebuggingLedState(false);
        for (std::uint8_t i = 0; i < count; ++i) {
            hal::GPIO::togglePin(hal::GPIOPort::B, hal::GPIOPin::PIN14);
            hal::GPIO::togglePin(hal::GPIOPort::B, hal::GPIOPin::PIN15);
            hal::HighResolutionTimer::waitMilliseconds(delayMs);
        }
    }

    void Panic::setDebuggingLedState(bool on) noexcept
    {
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);

        if (on) {
            hal::GPIO::setPinHigh(hal::GPIOPort::B, hal::GPIOPin::PIN14);
            hal::GPIO::setPinHigh(hal::GPIOPort::B, hal::GPIOPin::PIN15);
        } else {
            hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
            hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);
        }
    }

    void Panic::triggerHardFault() noexcept
    {
        __asm volatile("udf #0");
    }
}