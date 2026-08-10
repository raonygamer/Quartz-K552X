#include "usb/protocol/pipes/TransferPipe.hpp"
#include "hal/usb/Controller.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace quartz::usb::proto
{
    PipeTransferState TransferPipe::State = {};

    void TransferPipe::reset() noexcept
    {
        State.reset();
    }

    bool TransferPipe::isTransferring(const hal::usb::EndpointNumber num) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        if (hal::usb::Controller::getEndpoint(num).isIn())
            return State.In[value(num)].Active;
        return State.Out[value(num)].Active;
    }

    void TransferPipe::startTransferOut(const hal::usb::EndpointNumber num, const std::span<std::byte> buff, const std::size_t expected) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        HARD_ASSERTC(buff.data() != nullptr, PanicReason::TRANS_COUT_NUL_BUF);
        HARD_ASSERTC(buff.size() != 0, PanicReason::TRANS_COUT_BUF_ZLEN);
        HARD_ASSERT(expected <= buff.size());
        const auto& endpoint = hal::usb::Controller::getEndpoint(num);
        auto& [
            tData,
            tOffset,
            tExpectedLen,
            tActive
        ] = State.Out[value(num)];
        HARD_ASSERTC(endpoint.isOut(), PanicReason::ENDPT_NOUT_CFG);
        HARD_ASSERTC(!tActive, PanicReason::TRANS_COUT_ACTIVE);
        HARD_ASSERTC(endpoint.isIdle(), PanicReason::ENDPT_NOT_IDLE);
        tData = buff;
        tOffset = 0;
        tExpectedLen = expected;
        tActive = true;
        endpoint.armOut();
    }

    void TransferPipe::startTransferIn(const hal::usb::EndpointNumber num, const std::span<const std::byte> buff, const bool terminateWithZlp) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        HARD_ASSERTC(buff.data() != nullptr, PanicReason::TRANS_CIN_NUL_BUF);
        HARD_ASSERTC(buff.size() != 0, PanicReason::TRANS_CIN_BUF_ZLEN);
        const auto& endpoint = hal::usb::Controller::getEndpoint(num);
        auto& [
            tData,
            tOffset,
            tInFlight,
            tActive,
            tFinishZlp
        ] = State.In[value(num)];
        HARD_ASSERTC(endpoint.isIn(), PanicReason::ENDPT_NIN_CFG);
        HARD_ASSERTC(!tActive, PanicReason::TRANS_CIN_ACTIVE);
        HARD_ASSERTC(endpoint.isIdle(), PanicReason::ENDPT_NOT_IDLE);
        tData = buff;
        tOffset = 0;
        tInFlight = 0;
        tActive = true;
        tFinishZlp = terminateWithZlp;
        _transmitNextImmediate(num);
    }

    TransferEvents TransferPipe::handleInterrupt(const hal::usb::Interrupt status) noexcept
    {
        TransferEvents events = {};
        const auto handleEndpoint = [status, &events](const hal::usb::EndpointNumber num, const hal::usb::Interrupt interrupt)
        {
            if (!hasInterrupt(status, interrupt))
                return;

            if (hal::usb::Controller::getEndpoint(num).isIn())
            {
                if (_handleEndpointIn(num))
                {
                    events.InComplete |= 1u << value(num);
                }
            }
            else
            {
                if (_handleEndpointOut(num))
                {
                    events.OutComplete |= 1u << value(num);
                }
            }
        };

        handleEndpoint(hal::usb::EndpointNumber::EP1, hal::usb::Interrupt::EP1Ack);
        handleEndpoint(hal::usb::EndpointNumber::EP2, hal::usb::Interrupt::EP2Ack);
        handleEndpoint(hal::usb::EndpointNumber::EP3, hal::usb::Interrupt::EP3Ack);
        handleEndpoint(hal::usb::EndpointNumber::EP4, hal::usb::Interrupt::EP4Ack);
        return events;
    }

    void TransferPipe::_transmitNextImmediate(const hal::usb::EndpointNumber num) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        const auto& endpoint = hal::usb::Controller::getEndpoint(num);
        auto& transfer = State.In[value(num)];
        if (!transfer.Active)
        {
            return;
        }

        const std::size_t maxPacketSize = endpoint.getMaxSize();
        if (transfer.Offset < transfer.Data.size())
        {
            const std::size_t remaining = transfer.Data.size() - transfer.Offset;
            const std::size_t packetLength = std::min(remaining, maxPacketSize);
            endpoint.writeFrom(transfer.Data.subspan(transfer.Offset, static_cast<std::uint8_t>(packetLength)));
            transfer.InFlight = packetLength;
            endpoint.armIn(static_cast<std::uint8_t>(packetLength));
            return;
        }

        if (transfer.ShouldSendZeroLength)
        {
            transfer.InFlight = 0;
            transfer.ShouldSendZeroLength = false;
            endpoint.armIn(0);
            return;
        }

        transfer.reset();
    }

    bool TransferPipe::_handleEndpointOut(const hal::usb::EndpointNumber num) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        const auto& endpoint = hal::usb::Controller::getEndpoint(num);
        auto& transfer = State.Out[value(num)];
        if (!transfer.Active)
        {
            return false;
        }

        const std::size_t packetLength = endpoint.getReceivedSize();
        const std::size_t remaining = transfer.Data.size() - transfer.Offset;
        HARD_ASSERT(packetLength <= remaining);
        const bool isShortPacket = packetLength < endpoint.getMaxSize();
        endpoint.readTo(transfer.Data.subspan(transfer.Offset, packetLength));
        transfer.Offset += packetLength;
        const bool expectedComplete = transfer.ExpectedLength != 0 && transfer.Offset >= transfer.ExpectedLength;
        const bool full = transfer.Offset >= transfer.Data.size();
        if (isShortPacket || expectedComplete || full)
        {
            transfer.Active = false;
            return true;
        }

        endpoint.armOut();
        return false;
    }

    bool TransferPipe::_handleEndpointIn(const hal::usb::EndpointNumber num) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        auto& transfer = State.In[value(num)];
        if (!transfer.Active)
        {
            return false;
        }

        transfer.Offset += transfer.InFlight;
        transfer.InFlight = 0;
        if (transfer.Offset < transfer.Data.size() || transfer.ShouldSendZeroLength)
        {
            _transmitNextImmediate(num);
            return false;
        }

        transfer.reset();
        return true;
    }

    std::size_t TransferPipe::getTransferredOutSize(const hal::usb::EndpointNumber num) noexcept
    {
        HARD_ASSERTC(hal::usb::Endpoint::isEndpointValid(num), PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERT(num != hal::usb::EndpointNumber::EP0);
        return State.Out[value(num)].Offset;
    }
}