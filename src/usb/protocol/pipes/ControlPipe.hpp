#pragma once
#include "hal/usb/Endpoint.hpp"
#include "hal/usb/Interrupt.hpp"
#include "usb/protocol/InTransferState.hpp"
#include "usb/protocol/OutTransferState.hpp"
#include "usb/protocol/payloads/SetupPayload.hpp"

namespace quartz::usb::proto {
    enum class ControlStage : std::uint8_t
    {
        Idle,
        Setup,
        Data,
        Status
    };

    enum class StageDirection : std::uint8_t
    {
        None,
        In,
        Out
    };

    enum class ControlEvent : std::uint8_t
    {
        None,
        Setup,
        DataOutComplete,
        TransferComplete
    };

    class ControlPipe {
        static InTransferState InState;
        static OutTransferState OutState;
        static StageDirection Direction;
        static ControlStage Stage;
    public:
        static void reset() noexcept;
        static bool isTransferring() noexcept;
        static void startTransferOut(std::span<std::byte> buff) noexcept;
        static void startTransferIn(std::span<const std::byte> buff, bool terminateWithZlp = false) noexcept;
        static void startStatusIn() noexcept;
        static void startStatusOut() noexcept;
        static payloads::SetupPayload beginSetup() noexcept;
        static ControlEvent handleInterrupt(hal::usb::Interrupt status) noexcept;

    private:
        static void _transmitNextImmediate() noexcept;
        static ControlEvent _handleEndpointOut() noexcept;
        static void _handleEndpointIn() noexcept;
    };
}
