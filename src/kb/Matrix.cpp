#include "kb/Matrix.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/hid/BootKeyboardReport.hpp"
#include "utils/Position2D.hpp"
#include "KeyMap.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz::kb {
    utils::BitSet<Matrix::Size> Matrix::LastRawKeyStates {};
    utils::BitSet<Matrix::Size> Matrix::RawKeyStates {};
    utils::BitSet<Matrix::Size> Matrix::LastStableKeyStates {};
    utils::BitSet<Matrix::Size> Matrix::StableKeyStates {};
    std::array<std::uint8_t, Matrix::Size> Matrix::DebounceCounters {};

    void Matrix::initialize() noexcept {
        RawKeyStates.zero();
        LastRawKeyStates.zero();
        StableKeyStates.zero();
        LastStableKeyStates.zero();
        DebounceCounters.fill(0);
    }

    void Matrix::setRowPinsMode(const hal::GPIOMode mode) noexcept
    {
        for (std::uint8_t row = 0; row < Rows; ++row) {
            const GPIOPinSet& pinSet = RowPins[row];
            hal::GPIO::setPinMode(pinSet.Port, pinSet.Pin, mode);
        }
    }

    void Matrix::setRowPinValue(const std::uint8_t row, const bool high) noexcept
    {
        if (row >= Rows) {
            return;
        }

        const GPIOPinSet& pinSet = RowPins[row];
        hal::GPIO::setPinValue(pinSet.Port, pinSet.Pin, high);
    }

    void Matrix::setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept
    {
        for (std::uint8_t col = 0; col < Cols; ++col) {
            const GPIOPinSet& pinSet = ColPins[col];
            hal::GPIO::setPinMode(pinSet.Port, pinSet.Pin, mode);
            hal::GPIO::setPinPull(pinSet.Port, pinSet.Pin, pull);
        }
    }

    Matrix::ColumnBitSet Matrix::readColPins() noexcept
    {
        ColumnBitSet colStates;
        for (std::uint8_t col = 0; col < Cols; ++col) {
            const GPIOPinSet& pinSet = ColPins[col];
            const bool value = hal::GPIO::getPinValue(pinSet.Port, pinSet.Pin);
            if (!value) {
                colStates.set(col);
            } else {
                colStates.clear(col);
            }
        }

        return colStates;
    }

    void Matrix::debounce() noexcept
    {
        for (std::size_t index = 0; index < Size; ++index) {
            const bool raw = RawKeyStates.test(index);
            const bool stable = StableKeyStates.test(index);

            // Electrical state agrees with our accepted state.
            if (raw == stable) {
                DebounceCounters[index] = 0;
                continue;
            }

            // Raw state is trying to change.
            auto& counter = DebounceCounters[index];

            if (++counter < DebounceThreshold)
                continue;

            // It remained changed for Threshold consecutive scans:
            // accept it.
            if (raw)
                StableKeyStates.set(index);
            else
                StableKeyStates.clear(index);

            counter = 0;
        }
    }

    void Matrix::scan() noexcept
    {
        utils::BitSet<Size> newKeyStates;
        setRowPinsMode(hal::GPIOMode::Output);
        setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::PullUp);
        for (std::uint8_t row = 0; row < Rows; ++row) {
            setRowPinValue(row, true);
        }

        for (std::uint8_t row = 0; row < Rows; ++row) {
            setRowPinValue(row, false);
            hal::HighResolutionTimer::waitMicroseconds(30);
            const ColumnBitSet colStates = readColPins();
            for (std::uint8_t col = 0; col < Cols; ++col) {
                const std::size_t index = getKeyIndex(row, col);
                if (colStates.test(col)) {
                    newKeyStates.set(index);
                } else {
                    newKeyStates.clear(index);
                }
            }
            setRowPinValue(row, true);
        }
        RawKeyStates.cloneInto(LastRawKeyStates);
        newKeyStates.cloneInto(RawKeyStates);
        StableKeyStates.cloneInto(LastStableKeyStates);
        debounce();

        if (StableKeyStates != LastStableKeyStates) {
            usb::hid::BootKeyboardReport report {};
            for (std::size_t index = 0; index < Size; ++index) {
                if (!StableKeyStates.test(index))
                    continue;

                const std::uint8_t row = static_cast<std::uint8_t>(index % Rows);
                const std::uint8_t col = static_cast<std::uint8_t>(index / Rows);
                const auto action = KeyMap::Actions[row][col];
                if (action.Type == KeyActionType::HID)
                    report.add(action.Usage);
            }
            usb::hid::updateReport(report);
        }
    }
}