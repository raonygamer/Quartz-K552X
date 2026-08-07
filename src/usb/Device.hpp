#pragma once
#include <cstddef>
#include <cstdint>
#include <array>

#include "packets/SetupPacket.hpp"
#include "hal/usb/USB.hpp"

namespace quartz::usb {
    enum class EndpointState : std::uint8_t {
        Idle,
        Busy
    };

    enum class ControlState : std::uint8_t {
        Idle,
        DataIn,
        StatusOut,
        StatusIn
    };

    class Device {
    public:
        static void initialize() noexcept;
        static void handleInterrupt() noexcept;
        static bool isEndpointBusy(std::uint8_t endpoint) noexcept;
        static bool isConfigured() noexcept;
        static bool waitUntilConfigured(const std::uint64_t timeoutMilliseconds = 200) noexcept;

    private:
        static inline std::uint8_t pendingAddress = 0;
        static inline bool addressPending = false;
        static inline std::uint8_t pendingConfiguration = 0;
        static inline bool configurationPending = false;
        static inline std::uint8_t configuration = 0;
        static inline ControlState ep0State = ControlState::Idle;
        static inline std::array<EndpointState, hal::USB::MaxEndpoint + 1> endpointStates = {
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle
        };

        static std::array<void(*)(), hal::USB::MaxEndpoint + 1> endpointInHandlers;
        static std::array<void(*)(), hal::USB::MaxEndpoint + 1> endpointOutHandlers;

        static void handleBusReset() noexcept;
        static void handleSetup() noexcept;
        static void handleEndpointIn(const std::uint8_t endpoint) noexcept;
        static void handleEndpointOut(const std::uint8_t endpoint) noexcept;

        static void applyConfiguration(const std::uint8_t value) noexcept;

        static void handleStandardRequest(const SetupPacket& setup) noexcept;
        static void handleGetDescriptor(const SetupPacket& setup) noexcept;
        static SetupPacket readSetupPacket() noexcept;

        static void sendEndpoint(const std::uint8_t endpoint, const std::uint8_t* data, std::size_t length) noexcept;
        static void sendEndpointZeroLengthPacket(const std::uint8_t endpoint) noexcept;
        static void stallEndpoint(const std::uint8_t endpoint) noexcept;

        static void handleEP0In() noexcept;
        static void handleEP1In() noexcept;
        static void handleEP2In() noexcept;
        static void handleEP3In() noexcept;
        static void handleEP4In() noexcept;

        static void handleEP0Out() noexcept;
        static void handleEP1Out() noexcept;
        static void handleEP2Out() noexcept;
        static void handleEP3Out() noexcept;
        static void handleEP4Out() noexcept;
    };
}