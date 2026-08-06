#pragma once
#include <cstdint>

namespace quartz::usb {
    struct SetupPacket {
        std::uint8_t requestType;
        std::uint8_t request;
        std::uint16_t value;
        std::uint16_t index;
        std::uint16_t length;

        [[nodiscard]]
        constexpr std::uint8_t type() const noexcept
        {
            return requestType & 0x60u;
        }

        [[nodiscard]]
        constexpr std::uint8_t recipient() const noexcept
        {
            return requestType & 0x1Fu;
        }

        [[nodiscard]]
        constexpr bool isDeviceToHost() const noexcept
        {
            return (requestType & 0x80u) != 0u;
        }
    };

    static_assert(sizeof(SetupPacket) == 8);
}