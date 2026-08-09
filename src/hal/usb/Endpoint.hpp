#pragma once
#include <cstdint>
#include <type_traits>

#include "debug/Panic.hpp"

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal::usb {
    enum class EndpointDirection : std::uint8_t {
        Out = 0,
        In = 1,
        Both = 2
    };

    enum class EndpointState : std::uint8_t {
        Nak = 0x0,
        Ack = 0x1,
        Stall = 0x2
    };

    class Controller;
    class Endpoint {
        friend class Controller;
    public:
        static constexpr std::uint8_t MaxEndpoints = 5;
        const std::uint8_t EndpointNumber;
        const EndpointDirection Direction;
        const std::uint8_t MemoryOffset;
        const std::uint8_t MaxSize;

        Endpoint() = delete;
        constexpr Endpoint(const std::uint8_t num, const EndpointDirection direction, const std::uint8_t offset, const std::uint8_t maxSize) noexcept;
        EndpointState getState() const noexcept;
        bool isIdle() const noexcept;
        bool isArmed() const noexcept;
        bool isStalled() const noexcept;
        bool isIn() const noexcept;
        bool isOut() const noexcept;
        bool armIn(const std::uint8_t size) noexcept;
        bool armOut() noexcept;
        std::uint8_t getMemoryOffset() const noexcept;
        std::uint8_t getMaxSize() const noexcept;
        void read(std::uint8_t* buffer, const std::uint8_t size) const noexcept;
        void write(const std::uint8_t* buffer, const std::uint8_t size) noexcept;
        std::uint8_t getReceivedSize() const noexcept;

    public:
        inline static bool isEndpointValid(const std::uint8_t endpointNumber) noexcept
        {
            return endpointNumber < MaxEndpoints;
        }

    private:
        inline static volatile uint32_t& _getEndpointControl(const std::uint8_t endpointNumber) noexcept
        {
            HARD_ASSERTM(isEndpointValid(endpointNumber), "Invalid endpoint number");
            return *reinterpret_cast<volatile uint32_t*>(reinterpret_cast<std::uintptr_t>(&SN_USB->EP0CTL) + (endpointNumber * sizeof(uint32_t)));
        }

        inline static volatile void* _getUSBMemoryBase() noexcept
        {
            return reinterpret_cast<volatile void*>(SN_USB_BASE + 0x100);
        }
    };
}