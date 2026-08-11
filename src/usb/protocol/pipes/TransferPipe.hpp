#pragma once
#include "debug/Panic.hpp"
#include "hal/usb/Endpoint.hpp"
#include "hal/usb/Interrupt.hpp"
#include "usb/protocol/PipeTransferState.hpp"

namespace quartz::usb::proto
{
    struct TransferEvents
    {
        std::uint8_t InComplete = 0;
        std::uint8_t OutComplete = 0;

        constexpr bool inComplete(const hal::usb::EndpointNumber endpoint) const noexcept
        {
            return InComplete & (1u << value(endpoint));
        }

        constexpr bool outComplete(const hal::usb::EndpointNumber endpoint) const noexcept
        {
            return OutComplete & (1u << value(endpoint));
        }
    };

    class TransferPipe
    {
        static PipeTransferState State;

    public:
        static void reset() noexcept;
        static bool isTransferring(hal::usb::EndpointNumber num) noexcept;
        static void startTransferOut(hal::usb::EndpointNumber num, std::span<std::byte> buff, std::size_t expected = 0) noexcept;
        static void startTransferIn(hal::usb::EndpointNumber num, std::span<const std::byte> buff, bool terminateWithZlp = false) noexcept;
        static TransferEvents handleInterrupt(hal::usb::Interrupt status) noexcept;
        static std::size_t getTransferredOutSize(hal::usb::EndpointNumber num) noexcept;

    private:
        static void _transmitNextImmediate(hal::usb::EndpointNumber num) noexcept;
        static bool _handleEndpointOut(hal::usb::EndpointNumber num) noexcept;
        static bool _handleEndpointIn(hal::usb::EndpointNumber num) noexcept;
    };
}