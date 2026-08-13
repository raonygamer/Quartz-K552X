#pragma once
#include "KeyboardUsage.hpp"
#include <array>

namespace quartz::usb::hid
{
    struct [[gnu::packed]] BootKeyboardReport
    {
        static constexpr std::size_t KeySlots = 6;

        std::uint8_t Modifiers{};
        std::uint8_t Reserved{};
        std::array<std::uint8_t, KeySlots> Keys{};

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

            // E0-E7 are represented by bits in byte 0.
            if (value >= static_cast<std::uint8_t>(KeyboardUsage::LeftControl) &&
                value <= static_cast<std::uint8_t>(KeyboardUsage::RightGUI))
            {
                const std::uint8_t bit =
                    value - static_cast<std::uint8_t>(KeyboardUsage::LeftControl);

                Modifiers |= static_cast<std::uint8_t>(1u << bit);
                return true;
            }

            // Don't insert duplicates.
            for (const auto key : Keys)
            {
                if (key == value)
                    return true;
            }

            for (auto& key : Keys)
            {
                if (key == 0)
                {
                    key = value;
                    return true;
                }
            }

            // Standard 6KRO overflow indication.
            Keys.fill(
                static_cast<std::uint8_t>(KeyboardUsage::ErrorRollOver)
            );

            return false;
        }
    };

    static_assert(sizeof(BootKeyboardReport) == 8);
}