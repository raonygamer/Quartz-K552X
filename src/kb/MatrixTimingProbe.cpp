#include "kb/MatrixTimingProbe.hpp"
#include "kb/Matrix.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "utils/Time.hpp"
#include "cppmcu.h"

#include <limits>

namespace quartz::kb
{
    void MatrixTimingProbe::_waitRaw(const std::uint32_t ticks) noexcept
    {
        const std::uint32_t start = hal::HighResolutionTimer::rawTicks();
        while (((hal::HighResolutionTimer::rawTicks() - start) & RawTickMask) < ticks)
            __NOP();
    }

    std::uint16_t MatrixTimingProbe::_scanRow(const std::uint8_t row, const std::uint32_t settleTicks) noexcept
    {
        Matrix::_setRowPinValue(row, false);
        _waitRaw(settleTicks);
        const std::uint16_t states = Matrix::_readColPins();
        Matrix::_setRowPinValue(row, true);
        return states;
    }

    bool MatrixTimingProbe::_anyKeyDown(const std::uint32_t settleTicks) noexcept
    {
        for (std::uint8_t row = StartingRow; row < MatrixDefinitions::Rows; ++row)
        {
            if (_scanRow(row, settleTicks) != 0u)
                return true;
        }
        return false;
    }

    std::uint8_t MatrixTimingProbe::_firstPressedColumn(const std::uint16_t states) noexcept
    {
        for (std::uint8_t column = 0; column < MatrixDefinitions::Cols; ++column)
        {
            if (states & (1u << column))
                return column;
        }
        return NoColumn;
    }

    MatrixTimingProbe::ColumnProbe MatrixTimingProbe::_columnProbe(const std::uint8_t column) noexcept
    {
        // Columns: C0-C1, C3-C12, B6-B9.
        if (column < 2u)
            return { &SN_GPIO2->DATA, 1u << column };
        if (column < 12u)
            return { &SN_GPIO2->DATA, 1u << (column + 1u) };
        if (column < MatrixDefinitions::Cols)
            return { &SN_GPIO1->DATA, 1u << (column - 6u) };
        return {};
    }

    void MatrixTimingProbe::_measureRow(const std::uint8_t row, rpc::payloads::MatrixTimingProbeRowResult& result, const std::uint32_t timeoutTicks) noexcept
    {
        const ColumnProbe probe = _columnProbe(result.Column);
        if (!probe.Data)
            return;

        // With the row HIGH the held key's column must also be HIGH.
        // If it isn't, the key/net isn't in a usable state for this trial.
        if ((*probe.Data & probe.Mask) == 0u)
        {
            if (result.Timeouts != std::numeric_limits<std::uint8_t>::max())
                ++result.Timeouts;
            return;
        }

        // Deliberately make this tiny measurement window exclusive.
        // Otherwise an unrelated ISR landing here would become "electrical settling time".
        const std::uint32_t primask = __get_PRIMASK();
        __disable_irq();

        const std::uint32_t start = hal::HighResolutionTimer::rawTicks();
        Matrix::_setRowPinValue(row, false);

        bool detected = false;
        std::uint32_t elapsed = 0;

        for (;;)
        {
            if ((*probe.Data & probe.Mask) == 0u)
            {
                elapsed = (hal::HighResolutionTimer::rawTicks() - start) & RawTickMask;
                detected = true;
                break;
            }

            elapsed = (hal::HighResolutionTimer::rawTicks() - start) & RawTickMask;
            if (elapsed >= timeoutTicks)
                break;

            __NOP();
        }

        Matrix::_setRowPinValue(row, true);
        __set_PRIMASK(primask);

        if (!detected)
        {
            if (result.Timeouts != std::numeric_limits<std::uint8_t>::max())
                ++result.Timeouts;
            return;
        }

        const auto ticks = static_cast<std::uint16_t>(elapsed);
        if (result.Samples == 0u)
        {
            result.MinTicks = ticks;
            result.MaxTicks = ticks;
        }
        else
        {
            if (ticks < result.MinTicks)
                result.MinTicks = ticks;
            if (ticks > result.MaxTicks)
                result.MaxTicks = ticks;
        }

        if (result.Samples != std::numeric_limits<std::uint16_t>::max())
            ++result.Samples;
    }

    SizedMatrixTimingProbeResult MatrixTimingProbe::run(const std::uint32_t captureMs, const std::uint32_t durationMs) noexcept
    {
        constexpr std::uint32_t CaptureSettleTicks = utils::Time::microsecondsToTicks(40u);
        constexpr std::uint32_t RecoveryTicks = utils::Time::microsecondsToTicks(100u);
        constexpr std::uint32_t TimeoutTicks = utils::Time::microsecondsToTicks(100u);

        SizedMatrixTimingProbeResult result {};
        result.CoreClock = SystemCoreClock;

        for (auto& row : result.Rows)
            row.Column = NoColumn;

        Matrix::_begin();

        // First wait until the trigger chord has actually been released.
        // Requiring several clean scans also gives release bounce nowhere to hide.
        const std::uint64_t releaseDeadline = hal::HighResolutionTimer::nowTicks() + 2000ULL * hal::HighResolutionTimer::TicksPerMillisecond;
        std::uint8_t cleanScans = 0;

        while (hal::HighResolutionTimer::nowTicks() < releaseDeadline && cleanScans < 16u)
        {
            if (_anyKeyDown(CaptureSettleTicks))
                cleanScans = 0;
            else
                ++cleanScans;

            _waitRaw(RecoveryTicks);
        }

        // Capture one held key per physical row.
        // Once a row has a key, keep that key held until the probe finishes.
        const std::uint64_t captureDeadline = hal::HighResolutionTimer::nowTicks() + static_cast<std::uint64_t>(captureMs) * hal::HighResolutionTimer::TicksPerMillisecond;

        while (hal::HighResolutionTimer::nowTicks() < captureDeadline)
        {
            std::uint8_t captured = 0;

            for (std::uint8_t row = StartingRow; row < MatrixDefinitions::Rows; ++row)
            {
                auto& rowResult = result.Rows[row - StartingRow];
                if (rowResult.Column != NoColumn)
                {
                    ++captured;
                    continue;
                }

                const std::uint16_t states = _scanRow(row, CaptureSettleTicks);
                const std::uint8_t column = _firstPressedColumn(states);
                if (column == NoColumn)
                    continue;

                rowResult.Column = column;
                ++captured;
            }

            if (captured == result.Rows.size())
                break;

            _waitRaw(RecoveryTicks);
        }

        // Now repeatedly measure only the row/column pairs captured above.
        const std::uint64_t measurementDeadline = hal::HighResolutionTimer::nowTicks() + static_cast<std::uint64_t>(durationMs) * hal::HighResolutionTimer::TicksPerMillisecond;

        while (hal::HighResolutionTimer::nowTicks() < measurementDeadline)
        {
            bool measured = false;

            for (std::uint8_t row = StartingRow; row < MatrixDefinitions::Rows; ++row)
            {
                auto& rowResult = result.Rows[row - StartingRow];
                if (rowResult.Column == NoColumn)
                    continue;

                measured = true;
                _measureRow(row, rowResult, TimeoutTicks);
                _waitRaw(RecoveryTicks);
            }

            if (!measured)
                break;
        }

        Matrix::_setAllRowPinsValue(true);
        Matrix::_end();
        return result;
    }
}