#pragma once
#include <cstddef>
#include <cstdint>
#include <array>

#include "packets/SetupPacket.hpp"
#include "hal/usb/USB.hpp"
#include "Descriptors.hpp"
#include "hid/HIDProtocol.hpp"

namespace quartz::usb {
    namespace hid {
        struct BootKeyboardReport;
        struct NKROKeyboardReport;
    }

    enum class EndpointState : std::uint8_t {
        Idle,
        Busy
    };

    enum class ControlState : std::uint8_t {
        Idle,
        DataIn,
        DataOut,
        StatusOut,
        StatusIn
    };

    enum class ControlOutType : std::uint8_t {
        None,
        HIDOutputReport,
        HIDFeatureReport,
    };

    namespace {
        constexpr std::uint8_t HIDReportProtocol = 1;
    };

    class Device {
    public:
        static constexpr std::array<std::size_t, 5> EndpointMaxPacket = {
            64, // EP0
            32, // EP1 HID
            64, // EP2 unused
            32, // EP3 rpc
            32, // EP4 rpc
        };

        static void initialize() noexcept;
        static void handleInterrupt() noexcept;
        static bool isEndpointBusy(std::uint8_t endpoint) noexcept;
        static bool isConfigured() noexcept;
        static bool waitUntilConfigured(const std::uint64_t timeoutMilliseconds = 200) noexcept;
        static hid::Protocol getHIDProtocol() noexcept;
        static bool sendBootKeyboard(const hid::BootKeyboardReport& report) noexcept;
        static bool sendReportKeyboard(const hid::NKROKeyboardReport& report) noexcept;
        static bool sendRPCData(const std::uint8_t* data, const std::size_t length) noexcept;

    private:
        struct InTransfer {
            const std::uint8_t* Data = nullptr;

            std::size_t Length = 0;
            std::size_t Offset = 0;

            // Number of bytes currently sitting in the hardware FIFO
            // waiting for host ACK.
            std::size_t InFlight = 0;

            bool Active = false;

            // Send a zero-length packet after all data.
            bool ZlpPending = false;
            constexpr InTransfer() noexcept :
                Data(nullptr),
                Length(0),
                Offset(0),
                InFlight(0),
                Active(false),
                ZlpPending(false)
            {}
        };

        struct OutTransfer {
            std::uint8_t* Data = nullptr;

            std::size_t Capacity = 0;
            std::size_t Offset = 0;

            // 0 can mean "finish on short packet"
            std::size_t ExpectedLength = 0;

            bool Active = false;
            constexpr OutTransfer() noexcept :
                Data(nullptr),
                Capacity(0),
                Offset(0),
                ExpectedLength(0),
                Active(false)
            {}
        };

        static inline std::array<InTransfer, hal::USB::MaxEndpoint + 1> inTransfers;
        static inline std::array<OutTransfer, hal::USB::MaxEndpoint + 1> outTransfers;

        static inline std::uint8_t pendingAddress = 0;
        static inline bool addressPending = false;
        static inline std::uint8_t pendingConfiguration = 0;
        static inline bool configurationPending = false;
        static inline std::uint8_t configuration = 0;
        static inline ControlState ep0State = ControlState::Idle;
        static inline std::uint8_t hidIdleRate = 0;
        static inline hid::Protocol hidProtocol = hid::Protocol::Boot;
        static inline std::array<EndpointState, hal::USB::MaxEndpoint + 1> endpointStates = {
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle,
            EndpointState::Idle
        };

        inline static ControlOutType controlOutType = ControlOutType::None;
        inline static std::array<std::uint8_t, 64> featureReportBuffer {};

        static std::array<void(*)(), hal::USB::MaxEndpoint + 1> endpointInHandlers;
        static std::array<void(*)(), hal::USB::MaxEndpoint + 1> endpointOutHandlers;

        static void handleBusReset() noexcept;
        static void handleSetup() noexcept;
        static void handleEndpointIn(const std::uint8_t endpoint) noexcept;
        static void handleEndpointOut(const std::uint8_t endpoint) noexcept;
        static bool beginControlOutTransfer(
            std::uint8_t* data,
            const std::size_t length
        ) noexcept;

        static void applyConfiguration(const std::uint8_t value) noexcept;

        static bool handleSonixRebootCommand(const std::uint8_t* data, const std::size_t length) noexcept;
        static void handleVendorCommand(const std::uint8_t* data, const std::size_t length) noexcept;
        static void handleStandardRequest(const SetupPacket& setup) noexcept;
        static void handleClassRequest(const SetupPacket& setup) noexcept;
        static void handleGetDescriptor(const SetupPacket& setup) noexcept;
        static void handleHIDOutputReport() noexcept;
        static SetupPacket readSetupPacket() noexcept;

        static bool sendEndpoint(
            const std::uint8_t endpoint,
            const std::uint8_t* data,
            const std::size_t length,
            const bool terminateWithZlp = false
        ) noexcept;
        static void sendControlResponse(
            const SetupPacket& setup,
            const std::uint8_t* data,
            const std::size_t availableLength
        ) noexcept;
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
        static void armNextInPacket(const std::uint8_t endpoint) noexcept;
        static void handleOutPacket(const std::uint8_t endpoint) noexcept;

    public:
        static inline std::size_t HIDReportCount = 0;
        static inline std::size_t HIDOutputReportCount = 0;
        static constexpr std::size_t getEndpointMaxPacketSize(const std::uint8_t endpoint) noexcept
        {
            switch (endpoint) {
                case 0:
                    return 64;

                case 1:
                    return descriptors::HIDKeyboard::MaxPacketSize; // 8

                case 3:
                case 4:
                    return descriptors::RPC::MaxPacketSize;

                default:
                    return 0;
            }
        }
    };
}