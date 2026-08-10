#pragma once
#include "debug/Panic.hpp"
#include "hal/usb/Endpoint.hpp"
#include "hal/usb/Interrupt.hpp"
#include "usb/protocol/PipeTransferState.hpp"

namespace quartz::usb::proto {
    class TransferPipe {
        static PipeTransferState State;
    public:
        static void reset() noexcept;
        static bool isTransferring(hal::usb::EndpointNumber num) noexcept;
        static void startTransferOut(hal::usb::EndpointNumber num, std::span<std::byte> buff, std::size_t expected = 0) noexcept;
        static void startTransferIn(hal::usb::EndpointNumber num, std::span<const std::byte> buff, bool terminateWithZlp = false) noexcept;
        static void handleInterrupt(hal::usb::Interrupt status) noexcept;

    private:
        static void _transmitNextImmediate(hal::usb::EndpointNumber num) noexcept;
        static void _handleEndpointOut(hal::usb::EndpointNumber num) noexcept;
        static void _handleEndpointIn(hal::usb::EndpointNumber num) noexcept;
    };
}