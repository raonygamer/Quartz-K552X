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
    };
}