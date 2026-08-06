#pragma once
#include <cstdint>

namespace quartz::usb {
    enum class Interrupt : std::uint32_t {
        None         = 0u,

        BusReset     = 1u << 31,
        BusSuspend   = 1u << 30,
        BusResume    = 1u << 29,

        StartOfFrame = 1u << 26,
        BusWakeup    = 1u << 25,

        EP0PreSetup  = 1u << 24,
        EP0Setup     = 1u << 23,
        EP0In        = 1u << 22,
        EP0Out       = 1u << 21,

        EP0InStall   = 1u << 20,
        EP0OutStall  = 1u << 19,

        SetupError   = 1u << 18,
        TimeoutError = 1u << 17,

        EP1Ack       = 1u << 8,
        EP2Ack       = 1u << 9,
        EP3Ack       = 1u << 10,
        EP4Ack       = 1u << 11,

        EP1Nak       = 1u << 0,
        EP2Nak       = 1u << 1,
        EP3Nak       = 1u << 2,
        EP4Nak       = 1u << 3,
    };

    enum class InterruptEnable : std::uint32_t {
        Bus         = 1u << 31,
        StartOfFrame = 1u << 30,
        USBEvents   = 1u << 29,
        BusWakeup   = 1u << 28,
        EndpointAck = 1u << 4,

        EP4Nak      = 1u << 3,
        EP3Nak      = 1u << 2,
        EP2Nak      = 1u << 1,
        EP1Nak      = 1u << 0,
    };

    constexpr std::uint32_t value(const Interrupt interrupt) noexcept
    {
        return static_cast<std::uint32_t>(interrupt);
    }

    constexpr bool hasInterrupt(const std::uint32_t status, const Interrupt interrupt) noexcept
    {
        return (status & value(interrupt)) != 0u;
    }
}