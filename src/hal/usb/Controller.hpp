#pragma once
#include "hal/usb/Interrupt.hpp"
#include "hal/usb/Endpoint.hpp"
#include <array>

namespace quartz::hal::usb {
    class Controller {
    public:
        using EndpointArray = std::array<Endpoint, Endpoint::MAX_ENDPOINTS>;
        static EndpointArray Endpoints;

        static void initialize() noexcept;
        static void connect() noexcept;
        static void disconnect() noexcept;
        static void reset() noexcept;
        static void configure() noexcept;
        static void deconfigure() noexcept;
        static void setAddress(std::uint8_t address) noexcept;
        static bool isEndpointAvailable(EndpointNumber number) noexcept;

        static Endpoint& getEndpoint(EndpointNumber number) noexcept;
        static Interrupt getInterruptStatus() noexcept;
        static void clearInterruptStatus(Interrupt mask) noexcept;
        static Endpoint& getControlEndpoint() noexcept;
    };
}
