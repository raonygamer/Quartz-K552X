#include "GPIO.hpp"


namespace quartz::hal {
    volatile SN_GPIO0_Type* GPIO::getPort(GPIOPort port) noexcept
    {
        const auto portIndex = static_cast<std::uintptr_t>(port);
        const auto address = static_cast<std::uintptr_t>(SN_GPIO0_BASE) + portIndex * 0x2000u;
        return reinterpret_cast<volatile SN_GPIO0_Type*>(address);
    }

    void GPIO::setPinMode(GPIOPort port, GPIOPin pin, GPIOMode mode) noexcept
    {
        auto* gpioPort = getPort(port);
        const uint32_t mask = 1u << static_cast<std::uint8_t>(pin);

        if (mode == GPIOMode::Output)
            gpioPort->MODE |= mask;
        else
            gpioPort->MODE &= ~mask;
    }

    void GPIO::setPinValue(GPIOPort port, GPIOPin pin, bool high) noexcept
    {
        if (high)
            setPinHigh(port, pin);
        else
            setPinLow(port, pin);
    }

    void GPIO::setPinHigh(GPIOPort port, GPIOPin pin) noexcept
    {
        auto* gpio = getPort(port);
        const uint32_t mask = 1u << static_cast<std::uint8_t>(pin);
        gpio->BSET = mask;
    }

    void GPIO::setPinLow(GPIOPort port, GPIOPin pin) noexcept
    {
        auto* gpio = getPort(port);
        const uint32_t mask = 1u << static_cast<std::uint8_t>(pin);
        gpio->BCLR = mask;
    }

    void GPIO::togglePin(GPIOPort port, GPIOPin pin) noexcept
    {
        setPinValue(port, pin, !getPinValue(port, pin));
    }

    bool GPIO::getPinValue(GPIOPort port, GPIOPin pin) noexcept
    {
        auto* gpio = getPort(port);
        return (gpio->DATA & (1u << static_cast<std::uint8_t>(pin))) != 0;
    }
}