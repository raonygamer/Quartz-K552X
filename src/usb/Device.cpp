#include "usb/Device.hpp"

#include "hal/System.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "hal/usb/Controller.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "quartz/rpc/RPC.hpp"
#include "usb/Descriptors.hpp"
#include "usb/hid/KeyboardReporter.hpp"
#include "usb/protocol/payloads/StandardRequest.hpp"
#include "usb/protocol/pipes/TransferPipe.hpp"

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
            hal::usb::Controller::clearInterruptStatus(
                hal::usb::Interrupt::EP0PreSetup |
                hal::usb::Interrupt::EP0Setup |
                hal::usb::Interrupt::EP0In |
                hal::usb::Interrupt::EP0Out |
                hal::usb::Interrupt::EP0InStall |
                hal::usb::Interrupt::EP0OutStall
            );

            _cancelPendingState();
            State.Setup = proto::ControlPipe::beginSetup();
            const auto current = hal::usb::Controller::getInterruptStatus();
            if (hal::usb::hasInterrupt(current, hal::usb::Interrupt::EP0PreSetup) || hal::usb::hasInterrupt(
                                                                                         current, hal::usb::Interrupt::EP0Setup
                                                                                     ))
            {
                proto::ControlPipe::reset();
                return;
            }
            _handleSetup();
            return;
        }

        if (event != proto::ControlEvent::None)
            _handleControlEvent(event);

        const auto pipeEvents = proto::TransferPipe::handleInterrupt(status);
        if (pipeEvents.outComplete(hal::usb::EndpointNumber::EP3))
            rpc::RPC::handleReceiveComplete();
        if (pipeEvents.inComplete(hal::usb::EndpointNumber::EP4))
            rpc::RPC::handleTransmitComplete();

        if (hasInterrupt(status, hal::usb::Interrupt::EP0In))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP0In);
        if (hasInterrupt(status, hal::usb::Interrupt::EP0Out))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP0Out);
        if (hasInterrupt(status, hal::usb::Interrupt::EP1Ack))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP1Ack);
        if (hasInterrupt(status, hal::usb::Interrupt::EP2Ack))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP2Ack);
        if (hasInterrupt(status, hal::usb::Interrupt::EP3Ack))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP3Ack);
        if (hasInterrupt(status, hal::usb::Interrupt::EP4Ack))
            hal::usb::Controller::clearInterruptStatus(hal::usb::Interrupt::EP4Ack);
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

    hid::HIDProtocol Device::getProtocol() noexcept
    {
        return State.Protocol;
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
            break;
        }
        _stallControl();
    }

    void Device::_handleControlEvent(const proto::ControlEvent event) noexcept
    {
        switch (event)
        {
        case proto::ControlEvent::DataOutComplete:
            switch (State.ControlOut)
            {
            case hid::ControlOutType::HIDOutputReport:
                kb::KeyboardState::CurrentLEDState.Raw = State.HIDOutputReport;
                kb::KeyboardState::CurrentLEDState.updateLeds();
                State.ControlOut = hid::ControlOutType::None;
                proto::ControlPipe::startStatusIn();
                return;
            case hid::ControlOutType::HIDFeatureReport:
                if (_handleRebootCommand())
                    return;
                return;
            default:
                _stallControl();
                return;
            }

        case proto::ControlEvent::TransferComplete:
            _commitPendingState();
        default:
            break;
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
            break;
        }
        _stallControl();
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
            break;
        }
        _stallControl();
    }

    void Device::_handleGetHIDReport() noexcept
    {
        const auto& setup = State.Setup;
        const auto reportType = static_cast<hid::HIDReportType>(setup.value >> 8);
        const auto reportId = static_cast<std::uint8_t>(setup.value);
        if (!setup.isDeviceToHost() || reportId != 0)
        {
            _stallControl();
            return;
        }

        switch (reportType)
        {
        case hid::HIDReportType::Output:
            sendControlResponse(std::span{ &kb::KeyboardState::CurrentLEDState.Raw, 1 }, setup.length);
            return;
        case hid::HIDReportType::Input:
            _sendCurrentKeyboardReportControl();
            return;
        default:
            break;
        }

        _stallControl();
    }

    void Device::_handleSetHIDReport() noexcept
    {
        const auto& setup = State.Setup;
        const auto reportType = static_cast<hid::HIDReportType>(setup.value >> 8);
        const auto reportId = static_cast<std::uint8_t>(setup.value);
        if (setup.isDeviceToHost() || reportId != 0)
        {
            _stallControl();
            return;
        }

        switch (reportType)
        {
        case hid::HIDReportType::Output:
            State.ControlOut = hid::ControlOutType::HIDOutputReport;
            proto::ControlPipe::startTransferOut(std::as_writable_bytes(std::span{ &State.HIDOutputReport, 1 }));
            return;
        case hid::HIDReportType::Feature:
            State.ControlOut = hid::ControlOutType::HIDFeatureReport;
            proto::ControlPipe::startTransferOut(
                std::as_writable_bytes(std::span{ &State.HIDFeatureBuffer, setup.length })
            );
            return;
        default:
            break;
        }

        _stallControl();
    }

    void Device::_handleClassRequest() noexcept
    {
        constexpr std::uint16_t HIDInterface = 0;
        const auto& setup = State.Setup;
        if (setup.index != HIDInterface)
        {
            _stallControl();
            return;
        }

        switch (setup.request)
        {
        case hid::HIDRequest::SET_IDLE: {
            const auto reportId = static_cast<std::uint8_t>(setup.value);
            if (setup.isDeviceToHost() || reportId != 0 || setup.length != 0)
            {
                _stallControl();
                return;
            }

            State.IdleRate = static_cast<std::uint8_t>(setup.value >> 8);
            proto::ControlPipe::startStatusIn();
            return;
        }
        case hid::HIDRequest::GET_IDLE: {
            const auto reportId = static_cast<std::uint8_t>(setup.value);
            if (!setup.isDeviceToHost() || (setup.value >> 8) != 0 || reportId != 0 || setup.length != 1)
            {
                _stallControl();
                return;
            }

            proto::ControlPipe::startTransferIn(std::as_bytes(std::span{ &State.IdleRate, 1 }));
            return;
        }
        case hid::HIDRequest::SET_PROTOCOL:
            if (setup.isDeviceToHost() || setup.value > 1 || setup.length != 0)
            {
                _stallControl();
                return;
            }

            State.Protocol = static_cast<hid::HIDProtocol>(setup.value);
            proto::ControlPipe::startStatusIn();
            return;
        case hid::HIDRequest::GET_PROTOCOL:
            if (!setup.isDeviceToHost() || setup.value != 0 || setup.length != 1)
            {
                _stallControl();
                return;
            }

            proto::ControlPipe::startTransferIn(std::as_bytes(std::span{ &State.Protocol, 1 }));
            return;
        case hid::HIDRequest::SET_REPORT:
            _handleSetHIDReport();
            return;
        case hid::HIDRequest::GET_REPORT:
            _handleGetHIDReport();
            return;
        default:
            break;
        }
        _stallControl();
    }

    void Device::_handleVendorRequest() noexcept
    {
        _stallControl();
    }

    bool Device::_handleRebootCommand() noexcept
    {
        const auto& data = State.HIDFeatureBuffer;
        if constexpr (data.size() != hal::System::SONIX_REBOOT_MAGIC.size())
            return false;
        for (std::size_t i = 0; i < hal::System::SONIX_REBOOT_MAGIC.size(); ++i)
        {
            if (data[i] != static_cast<std::byte>(hal::System::SONIX_REBOOT_MAGIC[i]))
                return false;
        }
        State.RebootPending = true;
        return true;
    }

    void Device::_commitPendingState() noexcept
    {
        if (State.RebootPending)
        {
            State.RebootPending = false;
            debug::Panic::setNextRebootIsBootloader(true);
            hal::System::reset();
        }

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
        State.ControlOut = hid::ControlOutType::None;
    }

    void Device::_setConfiguration(std::uint8_t configuration) noexcept
    {
        if (configuration == 0)
        {
            hal::usb::Controller::deconfigure();
            return;
        }
        hal::usb::Controller::configure();
        rpc::RPC::initialize();
    }

    bool Device::sendKeyboard(const std::span<const std::byte> report) noexcept
    {
        if (!isConfigured() || proto::TransferPipe::isTransferring(hal::usb::EndpointNumber::EP1))
            return false;
        proto::TransferPipe::startTransferIn(hal::usb::EndpointNumber::EP1, report);
        return true;
    }

    void Device::_sendCurrentKeyboardReportControl() noexcept
    {
        const auto report = hid::rawCurrentKeyboardReport();
        if (report.data() == nullptr || report.size() == 0)
            return;
        sendControlResponse(std::span(reinterpret_cast<const uint8_t*>(report.data()), report.size()), State.Setup.length);
    }

    void Device::_stallControl() noexcept
    {
        hal::usb::Controller::getControlEndpoint().stall();
    }
}
