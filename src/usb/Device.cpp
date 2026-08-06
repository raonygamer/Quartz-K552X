#include "Device.hpp"

#include <algorithm>
#include <cstdint>

#include "Descriptors.hpp"
#include "Interrupt.hpp"
#include "hal/usb/USB.hpp"
#include "hal/gpio/GPIO.hpp"
#include "debug/DebugEndpoint.hpp"

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

    void Device::initialize() noexcept
    {
        pendingAddress = 0;
        addressPending = false;
        pendingConfiguration = 0;
        configurationPending = false;
        configuration = 0;
        controlState = ControlState::Idle;

        debug::DebugEndpoint::reset();
        hal::USB::initialize();
        hal::USB::connect();
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
            handleEP0In();
        }

        if (hasInterrupt(status, Interrupt::EP0Out)) {
            handleEP0Out();
        }

        if (hasInterrupt(status, Interrupt::EP4Ack)) {
            hal::USB::clearInterruptStatus(
                value(Interrupt::EP4Ack)
            );

            debug::DebugEndpoint::handleTransmitComplete();
        }
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
        controlState = ControlState::Idle;
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

            stallEP0();
            return;
        }

        switch (setup.type()) {
            case 0x00:
                handleStandardRequest(setup);
                return;

            default:
                stallEP0();
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
                    stallEP0();
                    return;
                }

                pendingAddress =
                    static_cast<std::uint8_t>(setup.value);

                addressPending = true;

                sendEP0ZeroLengthPacket();
                return;

            case RequestSetConfiguration:
                if (setup.isDeviceToHost() ||
                    setup.index != 0u ||
                    setup.length != 0u ||
                    setup.value > 1u) {
                    stallEP0();
                    return;
                }

                pendingConfiguration =
                    static_cast<std::uint8_t>(setup.value);

                configurationPending = true;

                sendEP0ZeroLengthPacket();
                return;

            case RequestGetConfiguration:
                sendEP0(
                    &configuration,
                    sizeof(configuration),
                    setup.length
                );
                return;

            default:
                stallEP0();
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
                sendEP0(
                    descriptors::Device.data(),
                    descriptors::Device.size(),
                    setup.length
                );
                return;

            case DescriptorConfiguration:
                sendEP0(
                    descriptors::Configuration.data(),
                    descriptors::Configuration.size(),
                    setup.length
                );
                return;

            case DescriptorString:
                switch (descriptorIndex) {
                    case 0:
                        sendEP0(
                            descriptors::Language.data(),
                            descriptors::Language.size(),
                            setup.length
                        );
                        return;

                    case 1:
                        sendEP0(
                            descriptors::Manufacturer.data(),
                            descriptors::Manufacturer.size(),
                            setup.length
                        );
                        return;

                    case 2:
                        sendEP0(
                            descriptors::Product.data(),
                            descriptors::Product.size(),
                            setup.length
                        );
                        return;

                    case 3:
                        sendEP0(
                            descriptors::SerialNumber.data(),
                            descriptors::SerialNumber.size(),
                            setup.length
                        );
                        return;

                default:
                    stallEP0();
                    return;
            }

            default:
                stallEP0();
                return;
        }
    }

    void Device::sendEP0(const std::uint8_t* data, std::uint16_t availableLength, std::uint16_t requestedLength) noexcept
    {
        std::uint16_t length =
        std::min(availableLength, requestedLength);

        if (length > 64u) {
            length = 64u;
        }

        hal::USB::writeFifo(
            0,
            data,
            length
        );

        controlState = ControlState::DataIn;

        hal::USB::armInEndpoint(
            0,
            length
        );
    }

    void Device::sendEP0ZeroLengthPacket() noexcept
    {
        controlState = ControlState::StatusIn;
        hal::USB::armInEndpoint(0, 0);
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

    void Device::handleEP0In() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::EP0In)
        );

        switch (controlState) {
            case ControlState::DataIn:
                /*
                * Descriptor/data packet was acknowledged.
                *
                * Now ACK the host's zero-length OUT status packet.
                * Despite the current HAL method name, EP0CTL's ACK state
                * applies according to the token direction sent by the host.
                */
                controlState = ControlState::StatusOut;
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

                controlState = ControlState::Idle;
                hal::USB::enableEndpoint(0);
                return;

            default:
                controlState = ControlState::Idle;
                hal::USB::enableEndpoint(0);
                return;
        }
    }

    void Device::handleEP0Out() noexcept
    {
        hal::USB::clearInterruptStatus(
            value(Interrupt::EP0Out)
        );

        if (controlState == ControlState::StatusOut) {
            controlState = ControlState::Idle;
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

    void Device::stallEP0() noexcept
    {
        hal::USB::stallEndpoint(0);
    }
}