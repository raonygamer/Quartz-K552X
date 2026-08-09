#pragma once
#include <cstdint>
#include <cstddef>
#include <array>

#include "hal/usb/Endpoint.hpp"

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal::usb {
    class Controller {
    public:
        using EndpointArray = std::array<Endpoint, Endpoint::MaxEndpoints>;
        static EndpointArray Endpoints;

        static void initialize() noexcept;
        static void connect() noexcept;
        static void disconnect() noexcept;
        static void reset() noexcept;
        static void handleInterrupt() noexcept;
        static void handleBusReset() noexcept;
        static void handleBusSuspend() noexcept;
        static void handleBusResume() noexcept;
        static void configure() noexcept;
        static void deconfigure() noexcept;

        static Endpoint& getEndpoint(const std::uint8_t endpointNumber) noexcept;
        static std::uint32_t getInterruptStatus() noexcept;
    private:
        static void _clearInterruptStatus(const std::uint32_t mask) noexcept;
    };
}