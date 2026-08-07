#include "Device.hpp"

#include <algorithm>
#include <cstdint>

#include "Descriptors.hpp"
#include "Interrupt.hpp"
#include "hal/usb/USB.hpp"
#include "hal/gpio/GPIO.hpp"
#include "debug/DebugEndpoint.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/KeyboardState.hpp"

namespace quartz::usb {
    namespace {
        constexpr std::uint8_t RequestGetDescriptor    = 0x06;
        constexpr std::uint8_t RequestSetAddress       = 0x05;
        constexpr std::uint8_t RequestSetConfiguration = 0x09;
        constexpr std::uint8_t RequestGetConfiguration = 0x08;

        constexpr std::uint8_t DescriptorDevice        = 0x01;
        constexpr std::uint8_t DescriptorConfiguration = 0x02;
        constexpr std::uint8_t DescriptorString        = 0x03;
        constexpr std::uint8_t DescriptorHID           = 0x21;
        constexpr std::uint8_t DescriptorReport        = 0x22;

        constexpr std::uint8_t HIDGetIdle      = 0x02;
        constexpr std::uint8_t HIDGetProtocol  = 0x03;
        constexpr std::uint8_t HIDSetIdle      = 0x0A;
        constexpr std::uint8_t HIDSetProtocol  = 0x0B;

        constexpr std::uint8_t HIDBootProtocol   = 0;

        constexpr std::uint8_t HIDInterface = 0;
        constexpr std::uint8_t HIDSetReport = 0x09;
        constexpr std::uint8_t HIDReportTypeOutput = 0x02;
    }

    std::array<void(*)(), hal::USB::MaxEndpoint + 1> Device::endpointInHandlers {
        &handleEP0In,
        &handleEP1In,
        &handleEP2In,
        &handleEP3In,
        &handleEP4In
    };

    std::array<void(*)(), hal::USB::MaxEndpoint + 1> Device::endpointOutHandlers {
        &handleEP0Out,
        &handleEP1Out,
        &handleEP2Out,
        &handleEP3Out,
        &handleEP4Out
    };

    void Device::initialize() noexcept
    {
        pendingAddress = 0;
        addressPending = false;
        pendingConfiguration = 0;
        configurationPending = false;
        configuration = 0;
        ep0State = ControlState::Idle;
        hidProtocol = HIDReportProtocol;
        hidIdleRate = 0;
        inTransfers = {};
        outTransfers = {};
        endpointStates = {
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle
        };

        debug::DebugEndpoint::reset();
        hal::USB::initialize();
        hal::USB::connect();
    }

    bool Device::isConfigured() noexcept
    {
        return configuration != 0u && pendingConfiguration == 0u && configurationPending == false;
    }

    bool Device::waitUntilConfigured(const std::uint64_t timeoutMilliseconds) noexcept
    {
        const auto start =
            hal::HighResolutionTimer::nowTicks();

        const auto timeout =
            timeoutMilliseconds *
            hal::HighResolutionTimer::TicksPerMillisecond;

        while (!isConfigured())
        {
            if ((hal::HighResolutionTimer::nowTicks() - start) >= timeout)
                return false;

            __WFI();
        }

        return true;
    }

    bool Device::sendKeyboardReport(const hid::BootKeyboardReport &report) noexcept
    {
        constexpr std::uint8_t endpoint = 1;
        if (!isConfigured() ||
            isEndpointBusy(endpoint)) {
            return false;
        }

        sendEndpoint(
            endpoint,
            reinterpret_cast<const std::uint8_t*>(&report),
            sizeof(report)
        );

        return true;
    }

    void Device::handleInterrupt() noexcept
    {
        const std::uint32_t status =
            hal::USB::getInterruptStatus();

        if (hasInterrupt(status, Interrupt::BusReset)) {
            handleBusReset();
        }

        if (hasInterrupt(status, Interrupt::EP0Setup)) {
            handleSetup();
        }

        if (hasInterrupt(status, Interrupt::EP0In)) {
            hal::USB::clearInterruptStatus(
                value(Interrupt::EP0In)
            );

            handleEndpointIn(0);
        }

        if (hasInterrupt(status, Interrupt::EP0Out)) {
            handleEndpointOut(0);
        }

        for (std::uint8_t endpoint = 1; endpoint <= hal::USB::MaxEndpoint; ++endpoint)
        {
            const std::uint32_t ack =
                1u << (7u + endpoint);

            if ((status & ack) == 0)
                continue;

            hal::USB::clearInterruptStatus(ack);
            handleEndpointIn(endpoint);
        }
    }

    bool Device::isEndpointBusy(std::uint8_t endpoint) noexcept
    {
        if (endpoint > hal::USB::MaxEndpoint) {
            return false;
        }

        return endpointStates[endpoint] != EndpointState::Idle;
    }

    void Device::handleBusReset() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::BusReset)
        );

        pendingAddress = 0;
        addressPending = false;
        pendingConfiguration = 0;
        configurationPending = false;
        configuration = 0;
        ep0State = ControlState::Idle;
        hidProtocol = HIDReportProtocol;
        hidIdleRate = 0;
        inTransfers = {};
        outTransfers = {};
        endpointStates = {
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle
        };
        debug::DebugEndpoint::reset();
        hal::USB::resetBus();
    }

    SetupPacket Device::readSetupPacket() noexcept
    {
        const std::uint32_t word0 =
            hal::USB::readFifo32(0);

        const std::uint32_t word1 =
            hal::USB::readFifo32(4);

        return SetupPacket {
            .requestType = static_cast<std::uint8_t>(
                word0
            ),

            .request = static_cast<std::uint8_t>(
                word0 >> 8
            ),

            .value = static_cast<std::uint16_t>(
                word0 >> 16
            ),

            .index = static_cast<std::uint16_t>(
                word1
            ),

            .length = static_cast<std::uint16_t>(
                word1 >> 16
            ),
        };
    }

    void Device::handleSetup() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::EP0PreSetup) |
            value(Interrupt::EP0Setup) |
            value(Interrupt::EP0InStall) |
            value(Interrupt::EP0OutStall)
        );

        inTransfers[0] = {};
        outTransfers[0] = {};

        ep0State = ControlState::Idle;
        endpointStates[0] = EndpointState::Idle;

        const SetupPacket setup = readSetupPacket();

        if (hasInterrupt(
                hal::USB::getInterruptStatus(),
                Interrupt::SetupError)) {
            hal::USB::clearInterruptStatus(
                value(Interrupt::SetupError)
            );

            stallEndpoint(0);
            return;
        }

        switch (setup.type()) {
            case RequestType::Standard: // Standard
                handleStandardRequest(setup);
                return;

            case RequestType::Class: // Class
                handleClassRequest(setup);
                return;

            default:
                stallEndpoint(0);
                return;
        }
    }

    void Device::handleStandardRequest(const SetupPacket& setup) noexcept
    {
        switch (setup.request) {
            case RequestGetDescriptor:
                handleGetDescriptor(setup);
                return;

            case RequestSetAddress:
                if (setup.isDeviceToHost() ||
                    setup.value > 127u ||
                    setup.index != 0u ||
                    setup.length != 0u) {
                    stallEndpoint(0);
                    return;
                }

                pendingAddress =
                    static_cast<std::uint8_t>(setup.value);

                addressPending = true;

                sendEndpointZeroLengthPacket(0);
                return;

            case RequestSetConfiguration:
                if (setup.isDeviceToHost() ||
                    setup.index != 0u ||
                    setup.length != 0u ||
                    setup.value > 1u) {
                    stallEndpoint(0);
                    return;
                }

                pendingConfiguration =
                    static_cast<std::uint8_t>(setup.value);

                configurationPending = true;

                sendEndpointZeroLengthPacket(0);
                return;

            case RequestGetConfiguration:
                sendControlResponse(
                    setup,
                    &configuration,
                    sizeof(configuration)
                );
                return;

            default:
                stallEndpoint(0);
                return;
        }
    }

    void Device::handleClassRequest(const SetupPacket& setup) noexcept
    {
        // HID requests are directed at interface 0.
        if ((setup.index & 0xFFu) != HIDInterface) {
            stallEndpoint(0);
            return;
        }

        switch (setup.request) {
            case HIDSetIdle:
                if (setup.isDeviceToHost() || setup.length != 0u) {
                    stallEndpoint(0);
                    return;
                }

                // High byte = idle duration in 4 ms units.
                hidIdleRate =
                    static_cast<std::uint8_t>(setup.value >> 8);

                sendEndpointZeroLengthPacket(0);
                return;

            case HIDGetIdle:
                if (!setup.isDeviceToHost()) {
                    stallEndpoint(0);
                    return;
                }

                sendControlResponse(
                    setup,
                    &hidIdleRate,
                    sizeof(hidIdleRate)
                );
                return;

            case HIDSetProtocol: {
                if (setup.isDeviceToHost() ||
                    setup.length != 0u ||
                    setup.value > HIDReportProtocol) {
                    stallEndpoint(0);
                    return;
                }

                hidProtocol =
                    static_cast<std::uint8_t>(setup.value);

                sendEndpointZeroLengthPacket(0);
                return;
            }

            case HIDGetProtocol:
                if (!setup.isDeviceToHost()) {
                    stallEndpoint(0);
                    return;
                }

                sendControlResponse(
                    setup,
                    &hidProtocol,
                    sizeof(hidProtocol)
                );
                return;

            case HIDSetReport: {
                if (setup.isDeviceToHost()) {
                    stallEndpoint(0);
                    return;
                }

                HIDReportCount++;

                const std::uint8_t reportType =
                    static_cast<std::uint8_t>(setup.value >> 8);

                const std::uint8_t reportId =
                    static_cast<std::uint8_t>(setup.value);

                if (reportType != HIDReportTypeOutput ||
                    reportId != 0 ||
                    setup.length != 1) {
                    stallEndpoint(0);
                    return;
                }

                if (!beginControlOutTransfer(&kb::KeyboardLEDState::Raw, sizeof(kb::KeyboardLEDState::Raw))) {
                    stallEndpoint(0);
                }

                return;
            }

            default:
                stallEndpoint(0);
                return;
        }
    }

    bool Device::beginControlOutTransfer(
        std::uint8_t* data,
        const std::size_t length
    ) noexcept
    {
        constexpr std::uint8_t Endpoint = 0;

        if (length != 0u && data == nullptr)
            return false;

        auto& transfer = outTransfers[Endpoint];

        transfer.Data = data;
        transfer.Capacity = length;
        transfer.Offset = 0;
        transfer.ExpectedLength = length;
        transfer.Active = true;

        ep0State = ControlState::DataOut;
        endpointStates[Endpoint] = EndpointState::Busy;

        // EP0 has no direction configuration bit;
        // ACK state applies to the next OUT token from the host.
        hal::USB::armOutEndpoint(Endpoint);

        return true;
    }

    void Device::handleGetDescriptor(const SetupPacket& setup) noexcept
    {
        const std::uint8_t descriptorType =
            static_cast<std::uint8_t>(setup.value >> 8);

        const std::uint8_t descriptorIndex =
            static_cast<std::uint8_t>(setup.value);

        switch (descriptorType) {
            case DescriptorDevice:
                sendControlResponse(
                    setup,
                    descriptors::Device.data(),
                    descriptors::Device.size()
                );
                return;

            case DescriptorConfiguration:
                sendControlResponse(
                    setup,
                    descriptors::Configuration.data(),
                    descriptors::Configuration.size()
                );
                return;

            case DescriptorString:
                switch (descriptorIndex) {
                    case 0:
                        sendControlResponse(
                            setup,
                            descriptors::Language.data(),
                            descriptors::Language.size()
                        );
                        return;

                    case 1:
                        sendControlResponse(
                            setup,
                            descriptors::Manufacturer.data(),
                            descriptors::Manufacturer.size()
                        );
                        return;

                    case 2:
                        sendControlResponse(
                            setup,
                            descriptors::Product.data(),
                            descriptors::Product.size()
                        );
                        return;

                    case 3:
                        sendControlResponse(
                            setup,
                            descriptors::SerialNumber.data(),
                            descriptors::SerialNumber.size()
                        );
                        return;

                    default:
                        stallEndpoint(0);
                        return;
                }

            case DescriptorHID:
                sendControlResponse(
                    setup,
                    descriptors::HIDKeyboard::Descriptor.data(),
                    descriptors::HIDKeyboard::Descriptor.size()
                );
                return;

            case DescriptorReport:
                sendControlResponse(
                    setup,
                    descriptors::HIDKeyboard::ReportDescriptor.data(),
                    descriptors::HIDKeyboard::ReportDescriptor.size()
                );
                return;

            default:
                stallEndpoint(0);
                return;
        }
    }

    bool Device::sendEndpoint(
        const std::uint8_t endpoint,
        const std::uint8_t* data,
        const std::size_t length,
        const bool terminateWithZlp
    ) noexcept
    {
        if (endpoint > hal::USB::MaxEndpoint)
            return false;

        if (isEndpointBusy(endpoint))
            return false;

        if (length != 0 && data == nullptr)
            return false;

        if (getEndpointMaxPacketSize(endpoint) == 0)
            return false;

        auto& transfer = inTransfers[endpoint];

        transfer.Data = data;
        transfer.Length = length;
        transfer.Offset = 0;
        transfer.InFlight = 0;
        transfer.Active = true;
        transfer.ZlpPending = terminateWithZlp;

        endpointStates[endpoint] = EndpointState::Busy;

        armNextInPacket(endpoint);

        return true;
    }

    void Device::sendEndpointZeroLengthPacket(const std::uint8_t endpoint) noexcept
    {
        if (endpoint == 0u) {
            ep0State = ControlState::StatusIn;
            endpointStates[0] = EndpointState::Busy;

            // No InTransfer is needed for the control status stage.
            hal::USB::armInEndpoint(0, 0);
            return;
        }

        sendEndpoint(
            endpoint,
            nullptr,
            0,
            true
        );
    }

    void Device::handleEP0In() noexcept
    {
        switch (ep0State) {
            case ControlState::DataIn:
                /*
                * Descriptor/data packet was acknowledged.
                *
                * Now ACK the host's zero-length OUT status packet.
                * Despite the current HAL method name, EP0CTL's ACK state
                * applies according to the token direction sent by the host.
                */
                ep0State = ControlState::StatusOut;
                endpointStates[0] = EndpointState::Busy;
                hal::USB::armInEndpoint(0, 0);
                return;

            case ControlState::StatusIn:
                /*
                * Status stage for SET_ADDRESS, SET_CONFIGURATION, etc.
                */
                if (addressPending) {
                    hal::USB::setAddress(pendingAddress);

                    addressPending = false;
                    pendingAddress = 0;
                }

                if (configurationPending) {
                    applyConfiguration(pendingConfiguration);
                    configuration = pendingConfiguration;
                    configurationPending = false;
                    pendingConfiguration = 0;
                }

                ep0State = ControlState::Idle;
                endpointStates[0] = EndpointState::Idle;
                hal::USB::enableEndpoint(0);
                return;

            default:
                ep0State = ControlState::Idle;
                endpointStates[0] = EndpointState::Idle;
                hal::USB::enableEndpoint(0);
                return;
        }
    }
    
    void Device::handleEP1In() noexcept
    {
    }

    void Device::handleEP2In() noexcept
    {
    }

    void Device::handleEP3In() noexcept
    {
    }

    void Device::handleEP4In() noexcept
    {
        debug::DebugEndpoint::handleTransmitComplete();
    }

    void Device::handleEP0Out() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::EP0Out)
        );

        switch (ep0State) {
            case ControlState::StatusOut:
                ep0State = ControlState::Idle;
                endpointStates[0] = EndpointState::Idle;

                hal::USB::enableEndpoint(0);
                return;

            case ControlState::DataOut: {
                auto& transfer = outTransfers[0];

                if (!transfer.Active) {
                    stallEndpoint(0);
                    return;
                }

                const std::size_t packetLength =
                    hal::USB::getEndpointByteCount(0);

                const std::size_t remaining =
                    transfer.Capacity - transfer.Offset;

                if (packetLength > remaining) {
                    transfer.Active = false;
                    stallEndpoint(0);
                    return;
                }

                hal::USB::readEndpointFifo(
                    0,
                    transfer.Data + transfer.Offset,
                    packetLength
                );

                transfer.Offset += packetLength;

                const bool shortPacket =
                    packetLength <
                    getEndpointMaxPacketSize(0);

                const bool complete =
                    transfer.Offset >=
                    transfer.ExpectedLength;

                if (!complete && !shortPacket) {
                    hal::USB::armOutEndpoint(0);
                    return;
                }

                // Host ended the DATA OUT stage.
                transfer.Active = false;

                const bool validLength =
                    transfer.Offset ==
                    transfer.ExpectedLength;

                if (!validLength) {
                    stallEndpoint(0);
                    return;
                }

                //
                // DATA OUT complete.
                // Now device sends zero-length IN status packet.
                //
                sendEndpointZeroLengthPacket(0);

                handleHIDOutputReport();
                return;
            }

            default:
                hal::USB::enableEndpoint(0);
                return;
        }
    }

    void Device::handleHIDOutputReport() noexcept
    {
        HIDOutputReportCount++;
        hal::GPIO::setPinValue(hal::GPIOPort::B, hal::GPIOPin::PIN14, kb::KeyboardLEDState::capsLock());
        hal::GPIO::setPinValue(hal::GPIOPort::B, hal::GPIOPin::PIN15, kb::KeyboardLEDState::scrollLock());
    }

    void Device::handleEP1Out() noexcept
    {
    }

    void Device::handleEP2Out() noexcept
    {
    }

    void Device::handleEP3Out() noexcept
    {
    }

    void Device::handleEP4Out() noexcept
    {
    }

    void Device::armNextInPacket(const std::uint8_t endpoint) noexcept
    {
        auto& transfer = inTransfers[endpoint];

        if (!transfer.Active)
            return;

        const std::size_t maxPacketSize =
            getEndpointMaxPacketSize(endpoint);

        if (transfer.Offset < transfer.Length) {
            const std::size_t remaining =
                transfer.Length - transfer.Offset;

            const std::size_t packetLength =
                std::min(remaining, maxPacketSize);

            hal::USB::writeEndpointFifo(
                endpoint,
                transfer.Data + transfer.Offset,
                packetLength
            );

            transfer.InFlight = packetLength;

            hal::USB::armInEndpoint(
                endpoint,
                packetLength
            );

            return;
        }

        if (transfer.ZlpPending) {
            transfer.ZlpPending = false;
            transfer.InFlight = 0;

            hal::USB::armInEndpoint(
                endpoint,
                0
            );

            return;
        }

        // Should only really be reachable for malformed state.
        transfer.Active = false;
        transfer.InFlight = 0;
        endpointStates[endpoint] = EndpointState::Idle;
    }

    void Device::handleOutPacket(const std::uint8_t endpoint) noexcept
    {
        auto& transfer = outTransfers[endpoint];
        const std::size_t packetLength =
            hal::USB::getEndpointByteCount(endpoint);

        const std::size_t remaining =
            transfer.Capacity - transfer.Offset;

        const std::size_t copyLength =
            std::min(packetLength, remaining);

        hal::USB::readEndpointFifo(
            endpoint,
            transfer.Data + transfer.Offset,
            copyLength
        );

        transfer.Offset += copyLength;

        const bool shortPacket =
            packetLength <
            getEndpointMaxPacketSize(endpoint);

        const bool expectedComplete =
            transfer.ExpectedLength != 0 &&
            transfer.Offset >= transfer.ExpectedLength;

        const bool full =
            transfer.Offset >= transfer.Capacity;

        if (shortPacket || expectedComplete || full) {
            transfer.Active = false;
            endpointStates[endpoint] = EndpointState::Idle;
            endpointOutHandlers[endpoint]();
            return;
        }

        // Tell the controller we're ready for another OUT packet.
        hal::USB::armOutEndpoint(endpoint);
    }

    void Device::applyConfiguration(const std::uint8_t value) noexcept
    {
        if (value == 0u) {
            debug::DebugEndpoint::setConfigured(false);

            hal::USB::disableEndpoint(1);
            hal::USB::disableEndpoint(4);

            endpointStates[1] = EndpointState::Idle;
            endpointStates[4] = EndpointState::Idle;
            return;
        }

        // HID keyboard: EP1 IN interrupt, 8 bytes.
        hal::USB::setEndpointDirection(1, false);
        hal::USB::enableEndpoint(1);

        // Quartz debug: EP4 IN bulk, 32 bytes.
        hal::USB::setEndpointDirection(4, false);
        hal::USB::enableEndpoint(4);

        debug::DebugEndpoint::setConfigured(true);

        debug::DebugEndpoint::writeString(
            "Quartz composite HID/debug configured\n"
        );
    }

    void Device::sendControlResponse(
        const SetupPacket& setup,
        const std::uint8_t* data,
        const std::size_t availableLength
    ) noexcept
    {
        constexpr std::size_t MaxPacketSize = 64;

        const std::size_t requestedLength =
            static_cast<std::size_t>(setup.length);

        const std::size_t transferLength =
            std::min(
                availableLength,
                requestedLength
            );

        /*
        * For a control IN transfer:
        *
        * If we have less data than the host requested, but the amount
        * happens to end exactly on a max-packet boundary, a ZLP is
        * needed to tell the host "that was all of it".
        */
        const bool needsZlp =
            transferLength < requestedLength &&
            (transferLength % MaxPacketSize) == 0;

        ep0State = ControlState::DataIn;

        if (!sendEndpoint(
                0,
                data,
                transferLength,
                needsZlp))
        {
            stallEndpoint(0);
        }
    }

    void Device::handleEndpointIn(const std::uint8_t endpoint) noexcept
    {
        auto& transfer = inTransfers[endpoint];

        if (!transfer.Active) {
            endpointStates[endpoint] = EndpointState::Idle;
            endpointInHandlers[endpoint]();
            return;
        }

        // The packet currently in flight was ACKed by the host.
        transfer.Offset += transfer.InFlight;
        transfer.InFlight = 0;

        if (transfer.Offset < transfer.Length ||
            transfer.ZlpPending)
        {
            armNextInPacket(endpoint);
            return;
        }

        // Entire logical transfer is complete.
        transfer.Active = false;
        transfer.Data = nullptr;
        transfer.Length = 0;
        transfer.Offset = 0;

        endpointStates[endpoint] = EndpointState::Idle;

        endpointInHandlers[endpoint]();
    }

    void Device::handleEndpointOut(const std::uint8_t endpoint) noexcept
    {
        if (endpoint == 0u) {
            handleEP0Out();
            return;
        }

        if (outTransfers[endpoint].Active) {
            handleOutPacket(endpoint);
            return;
        }

        endpointStates[endpoint] = EndpointState::Idle;
        endpointOutHandlers[endpoint]();
    }

    void Device::stallEndpoint(const std::uint8_t endpoint) noexcept
    {
        hal::USB::stallEndpoint(endpoint);
    }
}