#pragma once
#include <cstdint>
#include <span>

namespace quartz::hal::usb
{
    struct [[gnu::packed]] RWRegister
    {
        std::uint32_t Address;
        std::uint32_t Data;
        std::uint32_t Status;
    };

    enum class EndpointNumber : std::uint8_t
    {
        EP0 = 0,
        EP1 = 1,
        EP2 = 2,
        EP3 = 3,
        EP4 = 4
    };

    constexpr std::uint8_t value(EndpointNumber number) noexcept
    {
        return static_cast<std::uint8_t>(number);
    }

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
        friend class Controller;
        static volatile RWRegister& R_REG;
        static volatile RWRegister& W_REG;
    public:
        static constexpr std::uint8_t MAX_ENDPOINTS = 5;
        const EndpointNumber Number;
        const EndpointDirection Direction;
        const std::uint8_t MemoryOffset;
        const std::uint8_t MaxSize;

        Endpoint() = delete;
        Endpoint(EndpointNumber number, EndpointDirection direction, std::uint8_t offset, std::uint8_t maxSize) noexcept;
        void enable() const noexcept;
        void disable() const noexcept;
        EndpointState getState() const noexcept;
        bool isIdle() const noexcept;
        bool isArmed() const noexcept;
        bool isStalled() const noexcept;
        bool isIn() const noexcept;
        bool isOut() const noexcept;
        void armIn(std::size_t size) const noexcept;
        void armOut() const noexcept;
        void stall() const noexcept;
        std::uint8_t getMemoryOffset() const noexcept;
        std::size_t getMaxSize() const noexcept;
        std::uint32_t read32() const noexcept;
        void write32(std::uint32_t value) const noexcept;
        void readTo(std::span<std::byte> buff) const noexcept;
        void writeFrom(std::span<const std::byte> buff) const noexcept;
        std::uint8_t getReceivedSize() const noexcept;
        static bool isEndpointValid(EndpointNumber number) noexcept;

    private:
        void configure() const noexcept;
        void deconfigure() const noexcept;
        static volatile uint32_t& _getEndpointControl(EndpointNumber number) noexcept;
    };
}