#include "KeyboardState.hpp"
#include "KeyMap.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz::kb {
    LEDState KeyboardState::CurrentLEDState = {};
    utils::BitSet<Matrix::Size> KeyboardState::CurrentKeyStates = {};
    utils::BitSet<Matrix::Size> KeyboardState::LastKeyStates = {};

    void KeyboardState::debounce(const utils::BitSet<Matrix::Size>& rawStates) noexcept
    {
        static constexpr std::uint8_t DebounceThreshold = 3;
        static std::array<std::uint8_t, Matrix::Size> DebounceCounters {};

        auto& states = CurrentKeyStates;
        for (std::size_t index = 0; index < Matrix::Size; ++index) {
            const bool raw = rawStates.test(index);
            const bool stable = states.test(index);

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
                states.set(index);
            else
                states.clear(index);

            counter = 0;
        }
    }

    void KeyboardState::updateKeyStates(const utils::BitSet<Matrix::Size>& rawStates) noexcept
    {
        CurrentKeyStates.cloneInto(LastKeyStates);
        debounce(rawStates);
    }

    bool KeyboardState::anyKeyChanged() noexcept
    {
        return !CurrentKeyStates.equals(LastKeyStates);
    }

    usb::hid::BootKeyboardReport KeyboardState::buildBootReport() noexcept
    {
        usb::hid::BootKeyboardReport report = {};
        for (std::size_t index = 0; index < Matrix::Size; ++index) {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::Actions[row][col];
            if (action.Type == KeyActionType::HID)
                report.add(action.Usage);
        }
        return report;
    }

    usb::hid::NKROKeyboardReport KeyboardState::buildNKROReport() noexcept
    {
        usb::hid::NKROKeyboardReport report;
        for (std::size_t index = 0; index < Matrix::Size; ++index) {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::Actions[row][col];
            if (action.Type == KeyActionType::HID)
                report.add(action.Usage);
        }
        return report;
    }
}