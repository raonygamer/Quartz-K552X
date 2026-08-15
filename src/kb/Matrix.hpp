#pragma once
#include "MatrixDefinitions.hpp"
#include "hal/gpio/GPIO.hpp"
#include "utils/BitSet.hpp"
#include <cstdint>

namespace quartz::kb
{
    struct GPIOPinSet
    {
        hal::GPIOPort Port;
        hal::GPIOPin Pin;

        std::uint32_t getMask() const noexcept
        {
            return (1u << static_cast<std::uint32_t>(Pin));
        }
    };

    class Matrix
    {
        friend class MatrixTimingProbe;

        static constexpr std::uint32_t RowMaskC = 0xA000u; // C13, C15
        static constexpr std::uint32_t RowMaskD = 0x0F80u; // D7-D11
        static constexpr std::uint32_t ColMaskC = 0x1FFBu; // C0,C1,C3-C12
        static constexpr std::uint32_t ColMaskB = 0x03C0u; // B6-B9
        constexpr static GPIOPinSet RowPins[MatrixDefinitions::Rows] = {
            GPIOPinSet(hal::GPIOPort::C, hal::GPIOPin::PIN13),
            GPIOPinSet(hal::GPIOPort::C, hal::GPIOPin::PIN15),
            GPIOPinSet(hal::GPIOPort::D, hal::GPIOPin::PIN7),
            GPIOPinSet(hal::GPIOPort::D, hal::GPIOPin::PIN8),
            GPIOPinSet(hal::GPIOPort::D, hal::GPIOPin::PIN9),
            GPIOPinSet(hal::GPIOPort::D, hal::GPIOPin::PIN10),
            GPIOPinSet(hal::GPIOPort::D, hal::GPIOPin::PIN11)
        };
    public:
        static void scan() noexcept;

    private:
        static void _begin() noexcept;
        static void _end() noexcept;
        static void _setRowPinsMode(hal::GPIOMode mode) noexcept;
        static void _setRowPinValue(std::uint8_t row, bool high) noexcept;
        static void _setAllRowPinsValue(bool high) noexcept;
        static void _setColPinsMode(hal::GPIOMode mode, hal::GPIOPull pull) noexcept;
        static std::uint16_t _readColPins() noexcept;
        static constexpr std::uint32_t _expandCFGMask(std::uint32_t pinMask) noexcept;
        static constexpr std::uint32_t _makeCFGValue(std::uint32_t pinMask, std::uint32_t value) noexcept;

        inline static std::uint32_t ColCFGMaskC = _expandCFGMask(ColMaskC);
        inline static std::uint32_t ColCFGMaskB = _expandCFGMask(ColMaskB);
        inline static std::uint32_t ColCFGInactiveC = _makeCFGValue(ColMaskC, 0b10u);
        inline static std::uint32_t ColCFGInactiveB = _makeCFGValue(ColMaskB, 0b10u);
    };

    constexpr std::uint32_t Matrix::_expandCFGMask(const std::uint32_t pinMask) noexcept
    {
        std::uint32_t result = 0;

        for (std::uint32_t pin = 0; pin < 16; ++pin)
        {
            if ((pinMask & (1u << pin)) != 0)
            {
                result |= 0x3u << (pin * 2u);
            }
        }

        return result;
    }

    constexpr std::uint32_t Matrix::_makeCFGValue(const std::uint32_t pinMask, const std::uint32_t value) noexcept
    {
        std::uint32_t result = 0;

        for (std::uint32_t pin = 0; pin < 16; ++pin)
        {
            if ((pinMask & (1u << pin)) != 0)
            {
                result |= (value & 0x3u) << (pin * 2u);
            }
        }

        return result;
    }
}
