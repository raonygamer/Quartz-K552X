#pragma once
#include <cstdint>

namespace quartz::kb {
    struct KeyboardLEDState {
        inline static std::uint8_t Raw = 0x0;

        [[nodiscard]]
        static bool numLock() noexcept
        {
            return (Raw & (1u << 0)) != 0;
        }

        [[nodiscard]]
        static bool capsLock() noexcept
        {
            return (Raw & (1u << 1)) != 0;
        }

        [[nodiscard]]
        static bool scrollLock() noexcept
        {
            return (Raw & (1u << 2)) != 0;
        }
    };
}