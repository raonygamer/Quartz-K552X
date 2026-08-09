#include "Endpoint.hpp"

namespace quartz::hal::usb {
    namespace {
        constexpr std::uint32_t ENDPOINT_ENABLE = 1u << 31;
        constexpr std::uint32_t ENDPOINT_ACK    = 1u << 29;
        constexpr std::uint32_t ENDPOINT_READY  = ENDPOINT_ENABLE | ENDPOINT_ACK;
    }

    constexpr Endpoint::Endpoint(const std::uint8_t num, const EndpointDirection direction, const std::uint8_t offset, const std::uint8_t maxSize) noexcept : 
        EndpointNumber(num), 
        Direction(direction), 
        MemoryOffset(offset), 
        MaxSize(maxSize)
    {
        if (num != 0 && direction == EndpointDirection::Both) {
            if (std::is_constant_evaluated())
                throw "Only endpoint 0 can be bidirectional!";

            HARD_ASSERT(false);
        }
    }

    EndpointState Endpoint::getState() const noexcept
    {
        return static_cast<EndpointState>(_getEndpointControl(EndpointNumber) & 0x60000000u);
    }

    bool Endpoint::isIdle() const noexcept {
        return getState() == EndpointState::Nak;
    }

    bool Endpoint::isArmed() const noexcept {
        return getState() == EndpointState::Ack;
    }

    bool Endpoint::isStalled() const noexcept {
        return getState() == EndpointState::Stall;
    }

    bool Endpoint::isIn() const noexcept {
        return Direction == EndpointDirection::In || Direction == EndpointDirection::Both;
    }

    bool Endpoint::isOut() const noexcept {
        return Direction == EndpointDirection::Out || Direction == EndpointDirection::Both;
    }

    bool Endpoint::armIn(const std::uint8_t size) noexcept
    {
        HARD_ASSERTM(isIn(), "Endpoint is not configured for IN transfers");
        HARD_ASSERTM(size <= MaxSize, "IN transfer exceeds endpoint maximum size");
        if (!isIdle())
            return false;
        // Bits 6:0 == Size of data to be transmitted (in bytes)
        _getEndpointControl(EndpointNumber) = ENDPOINT_READY | (size & 0x7Fu);
        return true;
    }

    bool Endpoint::armOut() noexcept
    {
        HARD_ASSERTM(isOut(), "Endpoint is not configured for OUT transfers");
        if (!isIdle())
            return false;
        _getEndpointControl(EndpointNumber) = ENDPOINT_READY;
        return true;
    }

    std::uint8_t Endpoint::getMemoryOffset() const noexcept {
        return MemoryOffset;
    }

    std::uint8_t Endpoint::getMaxSize() const noexcept {
        return MaxSize;
    }

    void Endpoint::read(std::uint8_t* buffer, const std::uint8_t size) const noexcept
    {
        HARD_ASSERTM(isOut(), "Endpoint is not configured for OUT transfers");
        HARD_ASSERTM(size <= MaxSize, "Read size exceeds endpoint maximum size");
        const auto offset = static_cast<std::uintptr_t>(MemoryOffset);
        const auto source = reinterpret_cast<volatile const std::uint8_t*>(_getUSBMemoryBase()) + offset;
        for (std::uint8_t i = 0; i < size; ++i)
            buffer[i] = source[i];
    }

    void Endpoint::write(const std::uint8_t* buffer, const std::uint8_t size) noexcept
    {
        HARD_ASSERTM(isIn(), "Endpoint is not configured for IN transfers");
        HARD_ASSERTM(size <= MaxSize, "Write size exceeds endpoint maximum size");
        const auto offset = static_cast<std::uintptr_t>(MemoryOffset);
        const auto target = reinterpret_cast<volatile std::uint8_t*>(_getUSBMemoryBase()) + offset;
        for (std::uint8_t i = 0; i < size; ++i)
            target[i] = buffer[i];
    }

    std::uint8_t Endpoint::getReceivedSize() const noexcept
    {
        HARD_ASSERTM(isOut(), "Endpoint is not configured for OUT transfers");
        return static_cast<std::uint8_t>(_getEndpointControl(EndpointNumber) & 0x7Fu);
    }
}