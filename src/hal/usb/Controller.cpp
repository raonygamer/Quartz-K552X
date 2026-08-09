#include "hal/usb/Controller.hpp"

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal::usb {
    // USB_SRAM is 256 bytes shared between the 5 endpoints.
    Controller::EndpointArray Controller::Endpoints = {
        Endpoint(0, EndpointDirection::Both, 0x00, 0x08),
        Endpoint(1, EndpointDirection::In,   0x08, 0x18),
        Endpoint(2, EndpointDirection::In,   0x20, 0x08),
        Endpoint(3, EndpointDirection::Out,  0x28, 0x40),
        Endpoint(4, EndpointDirection::In,   0x68, 0x40)
    };

    
}