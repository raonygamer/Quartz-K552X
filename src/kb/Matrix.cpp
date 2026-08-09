#include "kb/Matrix.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/Keyboard.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "utils/Time.hpp"
#include "rgb/RGBMatrix.hpp"
#include "quartz/Profiling.hpp"

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::kb {
    namespace {

        // GPIO mapping:
        // hal A = GPIO0
        // hal B = GPIO1
        // hal C = GPIO2
        // hal D = GPIO3

        constexpr std::uint32_t RowMaskC = 0xA000u; // C13, C15
        constexpr std::uint32_t RowMaskD = 0x0F80u; // D7-D11

        constexpr std::uint32_t ColMaskC = 0x1FFBu; // C0,C1,C3-C12
        constexpr std::uint32_t ColMaskB = 0x03C0u; // B6-B9

        //
        // Each GPIO CFG field is two bits:
        //
        //   00 = pull-up
        //   10 = inactive/no pull, Schmitt enabled
        //
        // Convert a 16-bit pin mask into the corresponding 32-bit
        // collection of two-bit CFG fields.
        //
        constexpr std::uint32_t expandCFGMask(const std::uint32_t pinMask) noexcept
        {
            std::uint32_t result = 0;

            for (std::uint32_t pin = 0; pin < 16; ++pin) {
                if ((pinMask & (1u << pin)) != 0) {
                    result |= 0x3u << (pin * 2u);
                }
            }

            return result;
        }

        constexpr std::uint32_t makeCFGValue(const std::uint32_t pinMask, const std::uint32_t value) noexcept
        {
            std::uint32_t result = 0;

            for (std::uint32_t pin = 0; pin < 16; ++pin) {
                if ((pinMask & (1u << pin)) != 0) {
                    result |= (value & 0x3u) << (pin * 2u);
                }
            }

            return result;
        }

        constexpr std::uint32_t ColCFGMaskC = expandCFGMask(ColMaskC);
        constexpr std::uint32_t ColCFGMaskB = expandCFGMask(ColMaskB);
        constexpr std::uint32_t ColCFGInactiveC = makeCFGValue(ColMaskC, 0b10u);
        constexpr std::uint32_t ColCFGInactiveB = makeCFGValue(ColMaskB, 0b10u);

    }

    void Matrix::initialize() noexcept
    {
    }

    void Matrix::setRowPinsMode(const hal::GPIOMode mode) noexcept
    {
        if (mode == hal::GPIOMode::Output) {
            SN_GPIO2->MODE |= RowMaskC;
            SN_GPIO3->MODE |= RowMaskD;
        } else {
            SN_GPIO2->MODE &= ~RowMaskC;
            SN_GPIO3->MODE &= ~RowMaskD;
        }
    }

    void Matrix::setRowPinValue(const std::uint8_t row, const bool high) noexcept
    {
        if (row >= MatrixDefinitions::Rows) {
            return;
        }

        const GPIOPinSet& pinSet = RowPins[row];
        const std::uint32_t mask = pinSet.getMask();

        if (pinSet.Port == hal::GPIOPort::C) {
            if (high) {
                SN_GPIO2->BSET = mask;
            } else {
                SN_GPIO2->BCLR = mask;
            }

            return;
        }

        // Rows only live on C and D.
        if (high) {
            SN_GPIO3->BSET = mask;
        } else {
            SN_GPIO3->BCLR = mask;
        }
    }

    void Matrix::setAllRowPinsValue(const bool high) noexcept
    {
        if (high) {
            SN_GPIO2->BSET = RowMaskC;
            SN_GPIO3->BSET = RowMaskD;
        } else {
            SN_GPIO2->BCLR = RowMaskC;
            SN_GPIO3->BCLR = RowMaskD;
        }
    }

    void Matrix::setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept
    {
        if (mode == hal::GPIOMode::Output) {
            SN_GPIO2->MODE |= ColMaskC;
            SN_GPIO1->MODE |= ColMaskB;
        } else {
            SN_GPIO2->MODE &= ~ColMaskC;
            SN_GPIO1->MODE &= ~ColMaskB;
        }

        //
        // CFG is TWO bits per GPIO.
        //
        // PullUp = 00
        // None   = 10 (inactive, Schmitt enabled)
        //
        if (pull == hal::GPIOPull::PullUp) {
            SN_GPIO2->CFG &= ~ColCFGMaskC;
            SN_GPIO1->CFG &= ~ColCFGMaskB;
        } else {
            SN_GPIO2->CFG =
                (SN_GPIO2->CFG & ~ColCFGMaskC) |
                ColCFGInactiveC;

            SN_GPIO1->CFG =
                (SN_GPIO1->CFG & ~ColCFGMaskB) |
                ColCFGInactiveB;
        }
    }

    std::uint16_t Matrix::readColPins() noexcept
    {
        //
        // Matrix inputs are active-low.
        //
        const std::uint32_t portC = ~SN_GPIO2->DATA & ColMaskC;
        const std::uint32_t portB = ~SN_GPIO1->DATA & ColMaskB;

        //
        // Physical:
        //
        // C0  -> bit 0
        // C1  -> bit 1
        // C3  -> bit 2
        // ...
        // C12 -> bit 11
        //
        // B6  -> bit 12
        // B7  -> bit 13
        // B8  -> bit 14
        // B9  -> bit 15
        //
        return static_cast<std::uint16_t>(
            (portC & 0x0003u) |
            ((portC >> 1u) & 0x0FFCu) |
            ((portB << 6u) & 0xF000u)
        );
    }

    void Matrix::begin() noexcept
    {
        const auto start = hal::HighResolutionTimer::rawTicks();
        setRowPinsMode(hal::GPIOMode::Output);
        setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::PullUp);
        setAllRowPinsValue(true);
        profiling::BeginScanTicks = static_cast<std::uint32_t>(
            hal::HighResolutionTimer::rawTicks() -
            start
        );
    }

    void Matrix::scan() noexcept
    {
        constexpr std::uint32_t SettleTicks = utils::Time::microsecondsToTicks(60);

        // Row 0 is ignored for some reason, so we start scanning from row 1.
        constexpr std::uint8_t StartingRow = 1;

        const auto start = hal::HighResolutionTimer::rawTicks();
        Matrix::begin();
        utils::BitSet<MatrixDefinitions::Size> newKeyStates;
        for (std::uint8_t row = StartingRow; row < MatrixDefinitions::Rows; ++row) {
            setRowPinValue(row, false);
            const std::uint32_t settleStart = hal::HighResolutionTimer::rawTicks();
            while (((hal::HighResolutionTimer::rawTicks() - settleStart) & 0x00FFFFFFu) < SettleTicks)
                __NOP();

            newKeyStates.setUnsafeU16(
                readColPins(),
                static_cast<std::size_t>(row) * 2U
            );
            setRowPinValue(row, true);
        }

        Matrix::end();
        kb::rgb::RGBMatrix::acquire();

        profiling::ScanTicks = static_cast<std::uint32_t>(hal::HighResolutionTimer::rawTicks() - start);

        const auto updateStart = hal::HighResolutionTimer::rawTicks();
        kb::KeyboardState::updateKeyStates(newKeyStates);
        profiling::StateUpdateTicks = static_cast<std::uint32_t>(hal::HighResolutionTimer::rawTicks() - updateStart);
    }

    void Matrix::end() noexcept
    {
        const auto start = hal::HighResolutionTimer::rawTicks();
        setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::None);
        setRowPinsMode(hal::GPIOMode::Input);
        profiling::EndScanTicks = static_cast<std::uint32_t>(
            hal::HighResolutionTimer::rawTicks() -
            start
        );
    }
}