#pragma once
#include <array>
#include <cstdint>
#include <type_traits>

#include "debug/Panic.hpp"

extern "C"
{
#include "SN32F240B.h"
}

namespace quartz::hal::usb
{
    struct [[gnu::packed]] RWRegister
    {
        std::uint32_t Address;
        std::uint32_t Data;
        std::uint32_t Status;
    };

    enum class EndpointDirection : std::uint8_t
    {
        Out  = 0,
        In   = 1,
        Both = 2
    };

    enum class EndpointState : std::uint8_t
    {
        Nak   = 0x0,
        Ack   = 0x1,
        Stall = 0x2
    };

    class Controller;
    class Endpoint
    {
    private:
        friend class Controller;

        static volatile RWRegister& R_REG;
        static volatile RWRegister& W_REG;

    public:
        static constexpr std::uint8_t MaxEndpoints = 5;
        const std::uint8_t EndpointNumber;
        const EndpointDirection Direction;
        const std::uint8_t MemoryOffset;
        const std::uint8_t MaxSize;

        Endpoint() = delete;
        constexpr Endpoint(const std::uint8_t num, const EndpointDirection direction, const std::uint8_t offset, const std::uint8_t maxSize) noexcept;
        void enable() noexcept;
        void disable() noexcept;
        EndpointState getState() const noexcept;
        bool isIdle() const noexcept;
        bool isArmed() const noexcept;
        bool isStalled() const noexcept;
        bool isIn() const noexcept;
        bool isOut() const noexcept;
        void armIn(const std::uint8_t size) noexcept;
        void armOut() noexcept;
        void stall() noexcept;
        std::uint8_t getMemoryOffset() const noexcept;
        std::uint8_t getMaxSize() const noexcept;
        std::uint32_t read32() const noexcept;
        void write32(const std::uint32_t value) noexcept;
        void readTo(void* buffer, const std::uint8_t size) const noexcept;
        void writeFrom(const void* buffer, const std::uint8_t size) noexcept;
        std::uint8_t getReceivedSize() const noexcept;

    private:
        void configure() noexcept;
        void deconfigure() noexcept;    

    public:
        [[gnu::always_inline]]
        inline static bool isEndpointValid(const std::uint8_t endpointNumber) noexcept;

    private:
        [[gnu::always_inline]]
        inline static volatile uint32_t& _getEndpointControl(const std::uint8_t endpointNumber) noexcept;

    };
}