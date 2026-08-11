#include "GPIO.hpp"

namespace quartz::hal
{
    namespace
    {
        constexpr std::uint32_t ConfigurationMask = 0b11u;

        constexpr std::uint32_t ConfigurationPullUp = 0b00u;
        constexpr std::uint32_t ConfigurationDigitalInput = 0b10u;
        constexpr std::uint32_t ConfigurationInputBufferDisabled = 0b11u;

        [[nodiscard]]
        constexpr std::uint32_t getConfigurationShift(const GPIOPin pin) noexcept
        {
            return static_cast<std::uint32_t>(pin) * 2u;
        }

        void setPinConfiguration(volatile SN_GPIO0_Type* const gpio, const GPIOPin pin, const std::uint32_t configuration) noexcept
        {
            const std::uint32_t shift =
                getConfigurationShift(pin);

            const std::uint32_t mask =
                ConfigurationMask << shift;

            gpio->CFG =
                (gpio->CFG & ~mask) |
                ((configuration & ConfigurationMask) << shift);
        }

        [[nodiscard]]
        std::uint32_t getPinConfiguration(volatile SN_GPIO0_Type* const gpio, const GPIOPin pin) noexcept
        {
            return (
                       gpio->CFG >> getConfigurationShift(pin)
                   ) &
                   ConfigurationMask;
        }
    }

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

    void GPIO::setPortMode(GPIOPort port, std::uint32_t mask, GPIOMode mode) noexcept
    {
        auto* gpioPort = getPort(port);
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

    void GPIO::setPortValue(GPIOPort port, std::uint32_t mask, bool high) noexcept
    {
        auto* gpioPort = getPort(port);
        if (high)
            gpioPort->BSET = mask;
        else
            gpioPort->BCLR = mask;
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

    uint32_t GPIO::getPortValue(GPIOPort port) noexcept
    {
        auto* gpio = getPort(port);
        return gpio->DATA;
    }

    void GPIO::setPinPull(GPIOPort port, GPIOPin pin, GPIOPull pull) noexcept
    {
        auto* gpio = getPort(port);
        switch (pull)
        {
        case GPIOPull::PullUp:
            setPinConfiguration(
                gpio,
                pin,
                ConfigurationPullUp
            );
            return;

        case GPIOPull::None: {
            /*
             * Preserve the disabled input buffer state when one
             * has already been selected.
             */
            const std::uint32_t current =
                getPinConfiguration(gpio, pin);

            setPinConfiguration(
                gpio,
                pin,
                current == ConfigurationInputBufferDisabled
                    ? ConfigurationInputBufferDisabled
                    : ConfigurationDigitalInput
            );

            return;
        }
        }
    }

    void GPIO::setPinInputBuffer(GPIOPort port, GPIOPin pin, bool enabled) noexcept
    {
        auto* gpio = getPort(port);
        if (enabled)
        {
            const std::uint32_t current =
                getPinConfiguration(gpio, pin);

            /*
             * Preserve a currently enabled pull-up; otherwise use
             * the ordinary floating digital-input configuration.
             */
            setPinConfiguration(
                gpio,
                pin,
                current == ConfigurationPullUp
                    ? ConfigurationPullUp
                    : ConfigurationDigitalInput
            );

            return;
        }

        /*
         * The manual explicitly requires DATA to remain low when
         * the Schmitt trigger/input buffer is disabled.
         */
        gpio->BCLR =
            1u << static_cast<std::uint32_t>(pin);

        setPinConfiguration(
            gpio,
            pin,
            ConfigurationInputBufferDisabled
        );
    }

    void GPIO::setPinAnalog(GPIOPort port, GPIOPin pin, bool enabled) noexcept
    {
        /*
         * AIN0–AIN15 are P2.0–P2.15. With your port numbering,
         * P2 is GPIOPort::C.
         */
        if (port != GPIOPort::C)
        {
            return;
        }

        setPinMode(
            port,
            pin,
            GPIOMode::Input
        );

        if (enabled)
        {
            setPinLow(port, pin);

            setPinConfiguration(
                getPort(port),
                pin,
                ConfigurationInputBufferDisabled
            );

            return;
        }

        /*
         * Restore it as an ordinary floating digital input.
         * This does not disable the ADC peripheral itself.
         */
        setPinConfiguration(
            getPort(port),
            pin,
            ConfigurationDigitalInput
        );
    }
}