#pragma once
#include <cstdint>
#include "KeyboardUsage.hpp"
#include <array>

namespace quartz::usb::hid {
    struct __attribute__((packed)) NKROKeyboardReport {
        static constexpr auto MaximumUsage = static_cast<std::uint8_t>(KeyboardUsage::International1);
        static constexpr std::size_t KeyBytes = (static_cast<std::size_t>(MaximumUsage) + 8u) / 8u;

        std::uint8_t Modifiers {};
        std::uint8_t Reserved {};
        std::array<std::uint8_t, KeyBytes> Keys {};

        constexpr void clear() noexcept
        {
            Modifiers = 0;
            Reserved = 0;
            Keys.fill(0);
        }

        constexpr bool add(const KeyboardUsage usage) noexcept
        {
            const auto value = static_cast<std::uint8_t>(usage);

            if (usage == KeyboardUsage::None)
                return true;

            if (value >= static_cast<std::uint8_t>(KeyboardUsage::LeftControl) &&
                value <= static_cast<std::uint8_t>(KeyboardUsage::RightGUI))
            {
                const auto bit = static_cast<std::uint8_t>(
                    value -
                    static_cast<std::uint8_t>(KeyboardUsage::LeftControl)
                );

                Modifiers |= static_cast<std::uint8_t>(1u << bit);
                return true;
            }

            if (value > MaximumUsage)
                return false;

            Keys[value / 8u] |=
                static_cast<std::uint8_t>(1u << (value % 8u));

            return true;
        }
    };

    static_assert(NKROKeyboardReport::KeyBytes == 17);
    static_assert(sizeof(NKROKeyboardReport) == 19);
}