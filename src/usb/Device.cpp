#include "Device.hpp"

#include <algorithm>
#include <cstdint>

#include "Descriptors.hpp"
#include "Interrupt.hpp"
#include "hal/usb/USB.hpp"
#include "hal/gpio/GPIO.hpp"
#include "debug/DebugEndpoint.hpp"
#include "hal/timer/HighResolutionTimer.hpp"

namespace quartz::usb {
    namespace {
        constexpr std::uint8_t RequestGetDescriptor    = 0x06;
        constexpr std::uint8_t RequestSetAddress       = 0x05;
        constexpr std::uint8_t RequestSetConfiguration = 0x09;
        constexpr std::uint8_t RequestGetConfiguration = 0x08;

        constexpr std::uint8_t DescriptorDevice        = 0x01;
        constexpr std::uint8_t DescriptorConfiguration = 0x02;
        constexpr std::uint8_t DescriptorString = 0x03;
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

    void Device::handleInterrupt() noexcept
    {
        const std::uint32_t status =
            hal::USB::getInterruptStatus();

        if (hasInterrupt(status, Interrupt::BusReset)) {
            hal::GPIO::setPinHigh(
                hal::GPIOPort::B,
                hal::GPIOPin::PIN14
            );

            handleBusReset();
        }

        if (hasInterrupt(status, Interrupt::EP0Setup)) {
            hal::GPIO::setPinHigh(
                hal::GPIOPort::B,
                hal::GPIOPin::PIN15
            );

            handleSetup();
        }

        if (hasInterrupt(status, Interrupt::EP0In)) {
            handleEndpointIn(0);
        }

        if (hasInterrupt(status, Interrupt::EP0Out)) {
            handleEndpointOut(0);
        }

        if (hasInterrupt(status, Interrupt::EP4Ack)) {
            hal::USB::clearInterruptStatus(
                value(Interrupt::EP4Ack)
            );

            handleEndpointIn(4);
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
            case 0x00:
                handleStandardRequest(setup);
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
                sendEndpoint(
                    0,
                    &configuration,
                    std::min(
                        sizeof(configuration),
                        static_cast<std::size_t>(setup.length)
                    )
                );
                return;

            default:
                stallEndpoint(0);
                return;
        }
    }

    void Device::handleGetDescriptor(const SetupPacket& setup) noexcept
    {
        const std::uint8_t descriptorType =
            static_cast<std::uint8_t>(setup.value >> 8);

        const std::uint8_t descriptorIndex =
            static_cast<std::uint8_t>(setup.value);

        switch (descriptorType) {
            case DescriptorDevice:
                sendEndpoint(
                    0,
                    descriptors::Device.data(),
                    std::min(
                        descriptors::Device.size(),
                        static_cast<std::size_t>(setup.length)
                    )
                );
                return;

            case DescriptorConfiguration:
                sendEndpoint(
                    0,  
                    descriptors::Configuration.data(),
                    std::min(
                        descriptors::Configuration.size(),
                        static_cast<std::size_t>(setup.length)
                    )
                );
                return;

            case DescriptorString:
                switch (descriptorIndex) {
                    case 0:
                        sendEndpoint(
                            0,
                            descriptors::Language.data(),
                            std::min(
                                descriptors::Language.size(),
                                static_cast<std::size_t>(setup.length)
                            )
                        );
                        return;

                    case 1:
                        sendEndpoint(
                            0,
                            descriptors::Manufacturer.data(),
                            std::min(
                                descriptors::Manufacturer.size(),
                                static_cast<std::size_t>(setup.length)
                            )
                        );
                        return;

                    case 2:
                        sendEndpoint(
                            0,
                            descriptors::Product.data(),
                            std::min(
                                descriptors::Product.size(),
                                static_cast<std::size_t>(setup.length)
                            )
                        );
                        return;

                    case 3:
                        sendEndpoint(
                            0,
                            descriptors::SerialNumber.data(),
                            std::min(
                                descriptors::SerialNumber.size(),
                                static_cast<std::size_t>(setup.length)
                            )
                        );
                        return;

                default:
                    stallEndpoint(0);
                    return;
            }

            default:
                stallEndpoint(0);
                return;
        }
    }

    void Device::sendEndpoint(const std::uint8_t endpoint, const std::uint8_t* data, std::size_t length) noexcept
    {
        if (length > 64u) {
            length = 64u;
        }

        hal::USB::writeFifo(
            endpoint,
            data,
            length
        );

        endpointStates[endpoint] = EndpointState::Busy;
        if (endpoint == 0) {
            ep0State = ControlState::DataIn;
        }
        hal::USB::armInEndpoint(
            endpoint,
            length
        );
    }

    void Device::sendEndpointZeroLengthPacket(const std::uint8_t endpoint) noexcept
    {
        endpointStates[endpoint] = EndpointState::Busy;
        if (endpoint == 0) {
            ep0State = ControlState::StatusIn;
        }
        hal::USB::armInEndpoint(endpoint, 0);
    }

    void Device::handleEP0In() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::EP0In)
        );

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

        if (ep0State == ControlState::StatusOut) {
            ep0State = ControlState::Idle;
            endpointStates[0] = EndpointState::Idle;
            hal::USB::enableEndpoint(0);
            return;
        }

        // OUT data stages will be handled here later.
        hal::USB::enableEndpoint(0);
        hal::GPIO::setPinLow(
            hal::GPIOPort::B,
            hal::GPIOPin::PIN15
        );
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

    void Device::applyConfiguration(const std::uint8_t value) noexcept
    {
        if (value == 0u) {
            debug::DebugEndpoint::setConfigured(false);
            hal::USB::disableEndpoint(4);
            return;
        }

        hal::USB::setEndpointDirection(4, false); // IN
        hal::USB::enableEndpoint(4);

        debug::DebugEndpoint::setConfigured(true);

        debug::DebugEndpoint::writeString(
            "Quartz debug endpoint configured\n"
        );
    }

    void Device::handleEndpointIn(const std::uint8_t endpoint) noexcept
    {
        endpointStates[endpoint] = EndpointState::Idle;
        endpointInHandlers[endpoint]();
    }

    void Device::handleEndpointOut(const std::uint8_t endpoint) noexcept
    {
        endpointStates[endpoint] = EndpointState::Idle;
        endpointOutHandlers[endpoint]();
    }

    void Device::stallEndpoint(const std::uint8_t endpoint) noexcept
    {
        hal::USB::stallEndpoint(endpoint);
    }
}