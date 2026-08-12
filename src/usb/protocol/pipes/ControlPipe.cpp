#include "usb/protocol/pipes/ControlPipe.hpp"
#include "debug/Panic.hpp"
#include "hal/usb/Controller.hpp"

namespace quartz::usb::proto
{
    InTransferState ControlPipe::InState = {};
    OutTransferState ControlPipe::OutState = {};
    StageDirection ControlPipe::Direction = StageDirection::None;
    ControlStage ControlPipe::Stage = ControlStage::Idle;

    void ControlPipe::reset() noexcept
    {
        InState.reset();
        OutState.reset();
        Direction = StageDirection::None;
        Stage = ControlStage::Idle;
    }

    bool ControlPipe::isTransferring() noexcept
    {
        return Stage != ControlStage::Idle;
    }

    void ControlPipe::startTransferOut(const std::span<std::byte> buff) noexcept
    {
        HARD_ASSERTC(Stage == ControlStage::Setup, PanicReason::CTRL_INVALID_STAGE);
        HARD_ASSERTC(buff.data() != nullptr, PanicReason::TRANS_COUT_NUL_BUF);
        HARD_ASSERTC(buff.size() != 0, PanicReason::TRANS_COUT_BUF_ZLEN);
        HARD_ASSERTC(!OutState.Active, PanicReason::TRANS_COUT_ACTIVE);
        OutState.Data = buff;
        OutState.Offset = 0;
        OutState.ExpectedLength = buff.size();
        OutState.Active = true;
        Direction = StageDirection::Out;
        Stage = ControlStage::Data;
        hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0).armOut();
    }

    void ControlPipe::startTransferIn(const std::span<const std::byte> buff, const bool terminateWithZlp) noexcept
    {
        HARD_ASSERTC(Stage == ControlStage::Setup, PanicReason::CTRL_INVALID_STAGE);
        HARD_ASSERTC(buff.data() != nullptr, PanicReason::TRANS_CIN_NUL_BUF);
        HARD_ASSERTC(buff.size() != 0, PanicReason::TRANS_CIN_BUF_ZLEN);
        HARD_ASSERTC(!InState.Active, PanicReason::TRANS_CIN_ACTIVE);
        InState.Data = buff;
        InState.Offset = 0;
        InState.InFlight = 0;
        InState.Active = true;
        InState.ShouldSendZeroLength = terminateWithZlp;
        Direction = StageDirection::In;
        Stage = ControlStage::Data;
        _transmitNextImmediate();
    }

    void ControlPipe::startStatusIn() noexcept
    {
        InState.reset();
        OutState.reset();
        Stage = ControlStage::Status;
        Direction = StageDirection::In;
        hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0).armIn(0);
    }

    void ControlPipe::startStatusOut() noexcept
    {
        InState.reset();
        OutState.reset();
        Stage = ControlStage::Status;
        Direction = StageDirection::Out;
        hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0).armOut();
    }

    payloads::SetupPayload ControlPipe::beginSetup() noexcept
    {
        reset();
        Stage = ControlStage::Setup;
        Direction = StageDirection::None;
        payloads::SetupPayload payload = {};
        const auto bytes = std::as_writable_bytes(std::span{ &payload, 1 });
        static_assert(sizeof(payload) == 8, "Size of SetupPayload must be exactly 8 bytes!");
        const auto& endpoint = hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0);
        endpoint.readTo(bytes);
        return payload;
    }

    std::pair<ControlEvent, hal::usb::Interrupt> ControlPipe::handleInterrupt(const hal::usb::Interrupt status) noexcept
    {
        using hal::usb::Interrupt;
        if (hasInterrupt(status, Interrupt::EP0Setup))
            return { ControlEvent::Setup, hal::usb::Interrupt::EP0Setup };

        if (hasInterrupt(status, Interrupt::EP0Out))
        {
            hal::usb::Controller::clearInterruptStatus(Interrupt::EP0Out);
            return { _handleEndpointOut(), Interrupt::EP0Out };
        }

        if (hasInterrupt(status, Interrupt::EP0In))
        {
            hal::usb::Controller::clearInterruptStatus(Interrupt::EP0In);
            _handleEndpointIn();
            return { Stage == ControlStage::Idle ? ControlEvent::TransferComplete : ControlEvent::None, Interrupt::EP0In };
        }

        return { ControlEvent::None, Interrupt::None };
    }

    void ControlPipe::_transmitNextImmediate() noexcept
    {
        if (!InState.Active)
            return;

        const auto& endpoint = hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0);
        if (InState.Offset < InState.Data.size())
        {
            const std::size_t remaining = InState.Data.size() - InState.Offset;
            const std::size_t packetLength = std::min(remaining, endpoint.getMaxSize());
            endpoint.writeFrom(InState.Data.subspan(InState.Offset, packetLength));
            InState.InFlight = packetLength;
            endpoint.armIn(packetLength);
            return;
        }

        if (InState.ShouldSendZeroLength)
        {
            InState.InFlight = 0;
            InState.ShouldSendZeroLength = false;
            endpoint.armIn(0);
        }
    }

    ControlEvent ControlPipe::_handleEndpointOut() noexcept
    {
        const auto& endpoint = hal::usb::Controller::getEndpoint(hal::usb::EndpointNumber::EP0);
        if (Stage == ControlStage::Status)
        {
            HARD_ASSERTC(Direction == StageDirection::Out, PanicReason::CTRL_INVALID_DIREC);
            HARD_ASSERTC(endpoint.getReceivedSize() == 0, PanicReason::CTRL_INVALID_RECSZ);
            reset();
            return ControlEvent::TransferComplete;
        }

        HARD_ASSERTC(Stage == ControlStage::Data, PanicReason::CTRL_INVALID_STAGE);
        HARD_ASSERTC(Direction == StageDirection::Out, PanicReason::CTRL_INVALID_DIREC);
        if (!OutState.Active)
            return ControlEvent::None;

        const std::size_t packetLength = endpoint.getReceivedSize();
        const std::size_t remaining = OutState.Data.size() - OutState.Offset;
        HARD_ASSERTC(packetLength <= remaining, PanicReason::CTRL_PLLESS_REM);
        endpoint.readTo(OutState.Data.subspan(OutState.Offset, packetLength));
        OutState.Offset += packetLength;
        if (OutState.Offset >= OutState.ExpectedLength)
        {
            OutState.Active = false;
            return ControlEvent::DataOutComplete;
        }

        endpoint.armOut();
        return ControlEvent::None;
    }

    void ControlPipe::_handleEndpointIn() noexcept
    {
        if (Stage == ControlStage::Status)
        {
            HARD_ASSERTC(Direction == StageDirection::In, PanicReason::CTRL_INVALID_DIREC);
            reset();
            return;
        }

        HARD_ASSERTC(Stage == ControlStage::Data, PanicReason::CTRL_INVALID_STAGE);
        HARD_ASSERTC(Direction == StageDirection::In, PanicReason::CTRL_INVALID_DIREC);
        if (!InState.Active)
            return;

        InState.Offset += InState.InFlight;
        InState.InFlight = 0;
        if (InState.Offset < InState.Data.size() || InState.ShouldSendZeroLength)
        {
            _transmitNextImmediate();
            return;
        }

        startStatusOut();
    }
}
