#include "kb/Matrix.hpp"
#include "cppmcu.h"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/KeyboardState.hpp"
#include "quartz/Profiling.hpp"
#include "rgb/RGBMatrix.hpp"
#include "rt/Concurrency.hpp"
#include "utils/Time.hpp"

namespace quartz::kb
{
    std::size_t Matrix::getKeyIndex(const uint8_t row, const uint8_t col) noexcept
    {
        return static_cast<std::size_t>(row) * MatrixDefinitions::Cols + col;
    }

    utils::MatrixPosition Matrix::getKeyPosition(const std::size_t index) noexcept
    {
        return utils::MatrixPosition{
            static_cast<std::uint8_t>(index / MatrixDefinitions::Cols),
            static_cast<std::uint8_t>(index % MatrixDefinitions::Cols)
        };
    }

    void Matrix::_setRowPinsMode(const hal::GPIOMode mode) noexcept
    {
        if (mode == hal::GPIOMode::Output)
        {
            SN_GPIO2->MODE |= RowMaskC;
            SN_GPIO3->MODE |= RowMaskD;
        }
        else
        {
            SN_GPIO2->MODE &= ~RowMaskC;
            SN_GPIO3->MODE &= ~RowMaskD;
        }
    }

    void Matrix::_setRowPinValue(const std::uint8_t row, const bool high) noexcept
    {
        if (row >= MatrixDefinitions::Rows)
        {
            return;
        }

        const GPIOPinSet& pinSet = RowPins[row];
        const std::uint32_t mask = pinSet.getMask();
        if (pinSet.Port == hal::GPIOPort::C)
        {
            if (high)
            {
                SN_GPIO2->BSET = mask;
            }
            else
            {
                SN_GPIO2->BCLR = mask;
            }
            return;
        }

        if (high)
        {
            SN_GPIO3->BSET = mask;
        }
        else
        {
            SN_GPIO3->BCLR = mask;
        }
    }

    void Matrix::_setAllRowPinsValue(const bool high) noexcept
    {
        if (high)
        {
            SN_GPIO2->BSET = RowMaskC;
            SN_GPIO3->BSET = RowMaskD;
        }
        else
        {
            SN_GPIO2->BCLR = RowMaskC;
            SN_GPIO3->BCLR = RowMaskD;
        }
    }

    void Matrix::_setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept
    {
        if (mode == hal::GPIOMode::Output)
        {
            SN_GPIO2->MODE |= ColMaskC;
            SN_GPIO1->MODE |= ColMaskB;
        }
        else
        {
            SN_GPIO2->MODE &= ~ColMaskC;
            SN_GPIO1->MODE &= ~ColMaskB;
        }

        if (pull == hal::GPIOPull::PullUp)
        {
            SN_GPIO2->CFG &= ~ColCFGMaskC;
            SN_GPIO1->CFG &= ~ColCFGMaskB;
        }
        else
        {
            SN_GPIO2->CFG =
                (SN_GPIO2->CFG & ~ColCFGMaskC) |
                ColCFGInactiveC;

            SN_GPIO1->CFG =
                (SN_GPIO1->CFG & ~ColCFGMaskB) |
                ColCFGInactiveB;
        }
    }

    std::uint16_t Matrix::_readColPins() noexcept
    {
        // Columns are Input and Pulled-UP
        const std::uint32_t portC = ~SN_GPIO2->DATA & ColMaskC;
        const std::uint32_t portB = ~SN_GPIO1->DATA & ColMaskB;
        return static_cast<std::uint16_t>(
            (portC & 0x0003u) |
            ((portC >> 1u) & 0x0FFCu) |
            ((portB << 6u) & 0xF000u)
        );
    }

    void Matrix::_begin() noexcept
    {
        const auto start = hal::HighResolutionTimer::rawTicks();
        _setRowPinsMode(hal::GPIOMode::Output);
        _setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::PullUp);
        _setAllRowPinsValue(true);
        profiling::BeginScanTicks = (
            hal::HighResolutionTimer::rawTicks() -
            start
        );
    }

    void Matrix::scan() noexcept
    {
        // Row 0 is ignored for some reason, so we start scanning from row 1.
        constexpr std::uint64_t SettleTicks = utils::Time::microsecondsToTicks(70);
        constexpr std::uint8_t StartingRow = 1;
        utils::BitSet<MatrixDefinitions::Size> newKeyStates;
        // Disable USB interrupts to reduce jitter on scanning
        rt::Concurrency::disableInterruptsAndExecute([&newKeyStates]
        {
            _begin();
            const auto start = hal::HighResolutionTimer::rawTicks();
            for (std::uint8_t row = StartingRow; row < MatrixDefinitions::Rows; ++row)
            {
                _setRowPinValue(row, false);
                const auto settleStart = hal::HighResolutionTimer::rawTicks();
                while ((hal::HighResolutionTimer::rawTicks() - settleStart & 0x00FFFFFFu) < SettleTicks) __NOP();
                newKeyStates.setUnsafeU16(
                    _readColPins(),
                    static_cast<std::size_t>(row) * 2U
                );
                _setRowPinValue(row, true);
            }
            profiling::ScanTicks = hal::HighResolutionTimer::rawTicks() - start;
            _end();
            rgb::RGBMatrix::acquire();
            const auto updateStart = hal::HighResolutionTimer::rawTicks();
            KeyboardState::updateKeyStates(newKeyStates);
            profiling::StateUpdateTicks = hal::HighResolutionTimer::rawTicks() - updateStart;
        }, USB_IRQn);
    }

    void Matrix::_end() noexcept
    {
        const auto start = hal::HighResolutionTimer::rawTicks();
        _setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::None);
        _setRowPinsMode(hal::GPIOMode::Input);
        profiling::EndScanTicks = hal::HighResolutionTimer::rawTicks() - start;
    }
}
