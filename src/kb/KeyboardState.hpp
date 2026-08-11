#pragma once
#include <cstdint>
#include <span>

#include "Matrix.hpp"
#include "MatrixDefinitions.hpp"
#include "usb/hid/BootKeyboardReport.hpp"
#include "usb/hid/HIDProtocol.hpp"
#include "usb/hid/NKROKeyboardReport.hpp"
#include "utils/BitSet.hpp"

namespace quartz::kb
{
    struct LEDState
    {
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

        void updateLeds() const noexcept
        {
            hal::GPIO::setPinValue(hal::GPIOPort::B, hal::GPIOPin::PIN14, capsLock());
            hal::GPIO::setPinValue(hal::GPIOPort::B, hal::GPIOPin::PIN15, scrollLock());
        }
    };

    class KeyboardState
    {
    public:
        static LEDState CurrentLEDState;
        static utils::BitSet<MatrixDefinitions::Size> CurrentKeyStates;
        static utils::BitSet<MatrixDefinitions::Size> LastKeyStates;

        static void debounce(const utils::BitSet<MatrixDefinitions::Size>& rawStates) noexcept;
        static void updateKeyStates(const utils::BitSet<MatrixDefinitions::Size>& rawStates) noexcept;
        static bool anyKeyChanged() noexcept;
        static bool isKeyDown(const quartz::usb::hid::KeyboardUsage usage);
        static bool isFunctionPressed() noexcept;
        static usb::hid::BootKeyboardReport buildBootReport() noexcept;
        static usb::hid::NKROKeyboardReport buildNKROReport() noexcept;
    };
}
