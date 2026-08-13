#pragma once
#include "hal/usb/Interrupt.hpp"
#include "usb/protocol/payloads/SetupPayload.hpp"
#include "usb/protocol/pipes/ControlPipe.hpp"
#include <cstdint>

#include "usb/hid/HIDProtocol.hpp"

namespace quartz::usb
{
    struct DeviceState
    {
        /*
         * Address 0 is the USB default address used before the host assigns
         * the device a non-zero address during enumeration.
         */
        std::uint8_t Address = 0;
        std::uint8_t PendingAddress = 0;
        bool HasPendingAddress = false;

        std::uint8_t Configuration = 0;
        std::uint8_t PendingConfiguration = 0;
        bool HasPendingConfiguration = false;

        payloads::SetupPayload Setup = {};

        hid::HIDProtocol Protocol = hid::HIDProtocol::Report;
        std::uint8_t IdleRate = 0;
        std::array<std::uint8_t, 2> HIDOutputReport = {};
        std::array<std::byte, 64> HIDFeatureBuffer = {};
        hid::ControlOutType ControlOut = hid::ControlOutType::None;

        bool RebootPending = false;
    };

    class Device
    {
        static DeviceState State;

    public:
        static void reset() noexcept;
        static void handleInterrupt(hal::usb::Interrupt status) noexcept;
        static bool isConfigured() noexcept;
        static std::uint8_t getAddress() noexcept;
        static std::uint8_t getConfiguration() noexcept;
        static void sendControlResponse(std::span<const std::uint8_t> buff, std::size_t requestedLength) noexcept;
        static bool waitUntilConfigured(std::uint64_t timeoutMs = 2000) noexcept;
        static bool sendReport(std::span<const std::byte> report) noexcept;
        static hid::HIDProtocol getProtocol() noexcept;

    private:
        static void _handleSetup() noexcept;
        static void _handleControlEvent(proto::ControlEvent event) noexcept;
        static void _handleStandardRequest() noexcept;
        static void _handleGetDescriptor() noexcept;
        static void _handleGetHIDReport() noexcept;
        static void _handleSetHIDReport() noexcept;
        static void _handleClassRequest() noexcept;
        static void _handleVendorRequest() noexcept;
        static bool _handleRebootCommand() noexcept;
        static void _commitPendingState() noexcept;
        static void _cancelPendingState() noexcept;
        static void _setConfiguration(std::uint8_t configuration) noexcept;
        static void _sendCurrentKeyboardReportControl() noexcept;
        static void _stallControl() noexcept;
    };
}
