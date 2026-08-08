#pragma once
#include <cstdint>
#include "utils/BitSet.hpp"
#include "Matrix.hpp"
#include "usb/hid/BootKeyboardReport.hpp"
#include "usb/hid/NKROKeyboardReport.hpp"

namespace quartz::kb {
    struct LEDState {
        std::uint8_t Raw = 0x0;

        [[nodiscard]]
        bool numLock() const noexcept
        {
            return (Raw & (1u << 0)) != 0;
        }

        [[nodiscard]]
        bool capsLock() const noexcept
        {
            return (Raw & (1u << 1)) != 0;
        }

        [[nodiscard]]
        bool scrollLock() const noexcept
        {
            return (Raw & (1u << 2)) != 0;
        }
    };

    class KeyboardState {
    public:
        static LEDState CurrentLEDState;
        static utils::BitSet<Matrix::Size> CurrentKeyStates;
        static utils::BitSet<Matrix::Size> LastKeyStates;

        static void debounce(const utils::BitSet<Matrix::Size>& rawStates) noexcept;
        static void updateKeyStates(const utils::BitSet<Matrix::Size>& rawStates) noexcept;
        static bool anyKeyChanged() noexcept;
        static usb::hid::BootKeyboardReport buildBootReport() noexcept;
        static usb::hid::NKROKeyboardReport buildNKROReport() noexcept;
    };
}