#pragma once
#include <cstdint>

namespace quartz::usb {
    enum class RequestType : std::uint8_t {
        Standard = 0,
        Class    = 1,
        Vendor   = 2,
        Reserved = 3,
    };

    struct SetupPacket {
        std::uint8_t requestType;
        std::uint8_t request;
        std::uint16_t value;
        std::uint16_t index;
        std::uint16_t length;

        [[nodiscard]]
        constexpr RequestType type() const noexcept
        {
            return static_cast<RequestType>(
                (requestType >> 5u) & 0x03u
            );
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