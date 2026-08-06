#pragma once
#include <cstddef>
#include <cstdint>

#include "packets/SetupPacket.hpp"

namespace quartz::usb {
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

    private:
        static inline std::uint8_t pendingAddress = 0;
        static inline bool addressPending = false;
        static inline std::uint8_t pendingConfiguration = 0;
        static inline bool configurationPending = false;
        static inline std::uint8_t configuration = 0;
        static inline ControlState controlState = ControlState::Idle;

        static void handleBusReset() noexcept;
        static void handleSetup() noexcept;
        static void handleEP0In() noexcept;
        static void handleEP0Out() noexcept;

        static void applyConfiguration(const std::uint8_t value) noexcept;

        static void handleStandardRequest(const SetupPacket& setup) noexcept;
        static void handleGetDescriptor(const SetupPacket& setup) noexcept;
        static SetupPacket readSetupPacket() noexcept;
        static void sendEP0(const std::uint8_t* data, std::uint16_t availableLength, std::uint16_t requestedLength) noexcept;

        static void sendEP0ZeroLengthPacket() noexcept;
        static void stallEP0() noexcept;
    };
}