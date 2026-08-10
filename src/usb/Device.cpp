#include "usb/Device.hpp"

#include "hal/timer/HighResolutionTimer.hpp"
#include "hal/usb/Controller.hpp"
#include "usb/Descriptors.hpp"
#include "usb/protocol/pipes/TransferPipe.hpp"
#include "usb/protocol/payloads/StandardRequest.hpp"
#include "usb/Descriptors.hpp"

namespace quartz::usb
{
    DeviceState Device::State = {};

    void Device::reset() noexcept
    {
        State = {};
        proto::ControlPipe::reset();
        proto::TransferPipe::reset();
    }

    void Device::handleInterrupt(const hal::usb::Interrupt status) noexcept
    {
        if (hasInterrupt(status, hal::usb::Interrupt::BusReset))
        {
            reset();
            hal::usb::Controller::reset();
            return;
        }

        const auto event = proto::ControlPipe::handleInterrupt(status);
        if (event == proto::ControlEvent::Setup)
        {
            _cancelPendingState();
            State.Setup = proto::ControlPipe::beginSetup();
            _handleSetup();
            return;
        }

        if (event != proto::ControlEvent::None)
            _handleControlEvent(event);
        proto::TransferPipe::handleInterrupt(status);
    }

    bool Device::isConfigured() noexcept
    {
        return State.Configuration != 0;
    }

    std::uint8_t Device::getAddress() noexcept
    {
        return State.Address;
    }

    std::uint8_t Device::getConfiguration() noexcept
    {
        return State.Configuration;
    }

    void Device::sendControlResponse(const std::span<const std::uint8_t> buff, const std::size_t requestedLength) noexcept
    {
        const std::size_t transferLength = std::min(buff.size(), requestedLength);
        const std::size_t maxPacketSize = hal::usb::Controller::getControlEndpoint().getMaxSize();
        const bool needsZlp = transferLength < requestedLength && transferLength % maxPacketSize == 0;
        proto::ControlPipe::startTransferIn(std::as_bytes(buff.first(transferLength)), needsZlp);
    }

    bool Device::waitUntilConfigured(const std::uint64_t timeoutMs) noexcept
    {
        const auto start = hal::HighResolutionTimer::nowTicks();
        const auto timeout = timeoutMs * hal::HighResolutionTimer::TicksPerMillisecond;
        while (!isConfigured())
        {
            if (hal::HighResolutionTimer::nowTicks() - start >= timeout)
                return false;
            __NOP();
        }

        return true;
    }

    void Device::_handleSetup() noexcept
    {
        switch (State.Setup.type())
        {
        case payloads::RequestType::Standard:
            _handleStandardRequest();
            return;
        case payloads::RequestType::Class:
            _handleClassRequest();
            return;
        case payloads::RequestType::Vendor:
            _handleVendorRequest();
            return;
        default:
            _stallControl();
            return;
        }
    }

    void Device::_handleControlEvent(proto::ControlEvent event) noexcept
    {
        switch (event)
        {
        case proto::ControlEvent::DataOutComplete:
            return;
        case proto::ControlEvent::TransferComplete:
            _commitPendingState();
            return;
        default:
            return;
        }
    }

    void Device::_handleStandardRequest() noexcept
    {
        const auto& setup = State.Setup;
        switch (State.Setup.request)
        {
        case payloads::StandardRequest::GET_DESCRIPTOR:
            _handleGetDescriptor();
            return;
        case payloads::StandardRequest::SET_ADDRESS:
            if (setup.isDeviceToHost() || setup.value > 127 || setup.index != 0 || setup.length != 0)
            {
                _stallControl();
                return;
            }
            State.PendingAddress = setup.value;
            State.HasPendingAddress = true;
            proto::ControlPipe::startStatusIn();
            return;
        case payloads::StandardRequest::GET_CONFIGURATION:
            if (!setup.isDeviceToHost() || setup.value != 0 || setup.index != 0 || setup.length != 1)
            {
                _stallControl();
                return;
            }
            proto::ControlPipe::startTransferIn(std::as_bytes(std::span{ &State.Configuration, 1 }));
            return;
        case payloads::StandardRequest::SET_CONFIGURATION:
            if (setup.isDeviceToHost() || setup.value > 1 || setup.index != 0 || setup.length != 0)
            {
                _stallControl();
                return;
            }
            State.PendingConfiguration = setup.value;
            State.HasPendingConfiguration = true;
            proto::ControlPipe::startStatusIn();
            return;
        default:
            _stallControl();
            return;
        }
    }

    void Device::_handleGetDescriptor() noexcept
    {
        const auto type = static_cast<std::uint8_t>(State.Setup.value >> 8);
        const auto index = static_cast<std::uint8_t>(State.Setup.value);
        const std::uint16_t length = State.Setup.length;
        switch (type)
        {
        case Descriptor::DEVICE:
            sendControlResponse(std::span{ Descriptor::Device.data(), Descriptor::Device.size() }, length);
            return;
        case Descriptor::CONFIGURATION:
            sendControlResponse(std::span{ Descriptor::Configuration.data(), Descriptor::Configuration.size() }, length);
            return;
        case Descriptor::STRING:
            if (index >= Descriptor::Strings.size())
            {
                _stallControl();
                return;
            }

            sendControlResponse(Descriptor::Strings[index], length);
            return;
        case Descriptor::HID:
            sendControlResponse(std::span{ HIDKeyboard::Descriptor.data(), HIDKeyboard::Descriptor.size() }, length);
            return;
        case Descriptor::REPORT:
            sendControlResponse(std::span{ HIDKeyboard::ReportDescriptor.data(), HIDKeyboard::ReportDescriptor.size() }, length);
            return;
        default:
            _stallControl();
            return;
        }
    }

    void Device::_handleClassRequest() noexcept
    {
        _stallControl();
    }

    void Device::_handleVendorRequest() noexcept
    {
        _stallControl();
    }

    void Device::_commitPendingState() noexcept
    {
        if (State.HasPendingAddress)
        {
            State.HasPendingAddress = false;
            State.Address = State.PendingAddress;
            hal::usb::Controller::setAddress(State.Address);
        }

        if (State.HasPendingConfiguration)
        {
            _setConfiguration(State.PendingConfiguration);
            State.HasPendingConfiguration = false;
            State.Configuration = State.PendingConfiguration;
        }
    }

    void Device::_cancelPendingState() noexcept
    {
        State.HasPendingAddress = false;
        State.HasPendingConfiguration = false;
    }

    void Device::_setConfiguration(std::uint8_t configuration) noexcept
    {
        if (configuration == 0)
            hal::usb::Controller::deconfigure();
        else
            hal::usb::Controller::configure();
    }

    void Device::_stallControl() noexcept
    {
        hal::usb::Controller::getControlEndpoint().stall();
    }
}
