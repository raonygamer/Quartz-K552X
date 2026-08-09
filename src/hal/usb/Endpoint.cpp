#include "Endpoint.hpp"

#include "rt/Concurrency.hpp"

namespace quartz::hal::usb
{
    namespace
    {
        constexpr std::uint32_t ENDPOINT_ENABLE = 1u << 31;
        constexpr std::uint32_t ENDPOINT_ACK = 1u << 29;
        constexpr std::uint32_t ENDPOINT_READY = ENDPOINT_ENABLE | ENDPOINT_ACK;
        constexpr std::uint32_t FIFO_WRITE_BUSY = 1u << 0;
        constexpr std::uint32_t FIFO_READ_BUSY = 1u << 1;
        constexpr std::uint32_t ENDPOINT_STALL = ENDPOINT_ENABLE | (0x3u << 29);
        constexpr std::uint32_t ENDPOINT_STATE_MASK = 0x60000000u;
    }

    volatile RWRegister& Endpoint::R_REG = *reinterpret_cast<volatile RWRegister*>(&SN_USB->RWADDR);
    volatile RWRegister& Endpoint::W_REG = *reinterpret_cast<volatile RWRegister*>(&SN_USB->RWADDR2);

    constexpr Endpoint::Endpoint(const std::uint8_t num, const EndpointDirection direction, const std::uint8_t offset, const std::uint8_t maxSize) noexcept :
        EndpointNumber(num),
        Direction(direction),
        MemoryOffset(offset),
        MaxSize(maxSize)
    {
        HARD_ASSERTC((MemoryOffset & 0x3u) == 0u, PanicReason::ENDPT_OFF_NALIGN);
        HARD_ASSERTC((MaxSize & 0x3u) == 0u, PanicReason::ENDPT_SZ_NALIGN);
        HARD_ASSERTC(EndpointNumber <= 4, PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERTC((MemoryOffset & 0x3u) == 0u, PanicReason::ENDPT_OFF_NALIGN);
        HARD_ASSERTC((MaxSize & 0x3u) == 0u, PanicReason::ENDPT_SZ_NALIGN);
        HARD_ASSERTC(MaxSize <= 64u, PanicReason::ENDPT_SZ_EXCEED);
        HARD_ASSERTC(static_cast<std::uint16_t>(MemoryOffset) + MaxSize <= 256u, PanicReason::ENDPT_SRAM_EXCEED);

        if (EndpointNumber == 0)
            HARD_ASSERTC(MemoryOffset == 0, PanicReason::ENDPT_EP0_OFFSET);

        if (num != 0 && direction == EndpointDirection::Both)
        {
            HARD_ASSERTC(false, PanicReason::ENDPT_NON0_DIR_BOTH);
        }
    }

    void Endpoint::enable() noexcept
    {
        _getEndpointControl(EndpointNumber) = ENDPOINT_ENABLE;
    }

    void Endpoint::disable() noexcept
    {
        _getEndpointControl(EndpointNumber) = 0;
    }

    EndpointState Endpoint::getState() const noexcept
    {
        const std::uint32_t state = _getEndpointControl(EndpointNumber) & ENDPOINT_STATE_MASK;
        if ((state & 0x40000000u) != 0u)
            return EndpointState::Stall;
        return static_cast<EndpointState>(state);
    }

    bool Endpoint::isIdle() const noexcept
    {
        return getState() == EndpointState::Nak;
    }

    bool Endpoint::isArmed() const noexcept
    {
        return getState() == EndpointState::Ack;
    }

    bool Endpoint::isStalled() const noexcept
    {
        return getState() == EndpointState::Stall;
    }

    bool Endpoint::isIn() const noexcept
    {
        return Direction == EndpointDirection::In || Direction == EndpointDirection::Both;
    }

    bool Endpoint::isOut() const noexcept
    {
        return Direction == EndpointDirection::Out || Direction == EndpointDirection::Both;
    }

    void Endpoint::armIn(const std::uint8_t size) noexcept
    {
        HARD_ASSERTC(isIn(), PanicReason::ENDPT_NIN_CFG);
        HARD_ASSERTC(size <= MaxSize, PanicReason::ENDPT_INARM_SZ_EXCEED);
        // Bits 6:0 == Size of data to be transmitted (in bytes)
        _getEndpointControl(EndpointNumber) = ENDPOINT_READY | (size & 0x7Fu);
    }

    void Endpoint::armOut() noexcept
    {
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        _getEndpointControl(EndpointNumber) = ENDPOINT_READY;
    }

    void Endpoint::stall() noexcept
    {
        _getEndpointControl(EndpointNumber) = ENDPOINT_STALL;
    }

    std::uint8_t Endpoint::getMemoryOffset() const noexcept
    {
        return MemoryOffset;
    }

    std::uint8_t Endpoint::getMaxSize() const noexcept
    {
        return MaxSize;
    }

    std::uint32_t Endpoint::read32() const noexcept
    {
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        std::uint32_t value;
        rt::Concurrency::executeInCriticalSection([this, &value]()
        {
            HARD_ASSERTC((R_REG.Status & FIFO_READ_BUSY) == 0u, PanicReason::USB_RREG_BUSY);
            R_REG.Address = MemoryOffset;
            R_REG.Status = FIFO_READ_BUSY;
            while ((R_REG.Status & FIFO_READ_BUSY) != 0u);
            value = R_REG.Data;
        });

        return value;
    }

    void Endpoint::write32(const std::uint32_t value) noexcept
    {
        HARD_ASSERTC(isIn(), PanicReason::ENDPT_NIN_CFG);
        rt::Concurrency::executeInCriticalSection([this, value]()
        {
            HARD_ASSERTC((W_REG.Status & FIFO_WRITE_BUSY) == 0u, PanicReason::USB_WREG_BUSY);
            W_REG.Address = MemoryOffset;
            W_REG.Data = value;
            W_REG.Status = FIFO_WRITE_BUSY;
            while ((W_REG.Status & FIFO_WRITE_BUSY) != 0u);
        });
    }

    void Endpoint::readTo(void* buffer, const std::uint8_t size) const noexcept
    {
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        HARD_ASSERTC(buffer != nullptr || size == 0, PanicReason::ENDPT_READ_NUL_BUF);
        HARD_ASSERTC(size <= MaxSize, PanicReason::ENDPT_READ_SZ_EXCEED);

        rt::Concurrency::executeInCriticalSection([this, buffer, size]()
        {
            HARD_ASSERTC((R_REG.Status & FIFO_READ_BUSY) == 0u, PanicReason::USB_RREG_BUSY);

            auto* destination = static_cast<std::uint8_t*>(buffer);
            const std::uint8_t words = size / sizeof(std::uint32_t);
            const std::uint8_t remainingBytes = size % sizeof(std::uint32_t);

            for (std::uint8_t i = 0; i < words; ++i)
            {
                R_REG.Address = MemoryOffset + i * sizeof(std::uint32_t);
                R_REG.Status = FIFO_READ_BUSY;
                while ((R_REG.Status & FIFO_READ_BUSY) != 0u);
                const std::uint32_t word = R_REG.Data;
                std::memcpy(destination + i * sizeof(std::uint32_t), &word, sizeof(word));
            }

            if (remainingBytes != 0)
            {
                R_REG.Address = MemoryOffset + words * sizeof(std::uint32_t);
                R_REG.Status = FIFO_READ_BUSY;
                while ((R_REG.Status & FIFO_READ_BUSY) != 0u);
                const std::uint32_t word = R_REG.Data;
                std::memcpy(destination + words * sizeof(std::uint32_t), &word, remainingBytes);
            }
        });
    }

    void Endpoint::writeFrom(const void* buffer, const std::uint8_t size) noexcept
    {
        HARD_ASSERTC(isIn(), PanicReason::ENDPT_NIN_CFG);
        HARD_ASSERTC(buffer != nullptr || size == 0, PanicReason::ENDPT_WRITE_NUL_BUF);
        HARD_ASSERTC(size <= MaxSize, PanicReason::ENDPT_WRITE_SZ_EXCEED);
        rt::Concurrency::executeInCriticalSection([this, buffer, size]()
        {
            HARD_ASSERTC((W_REG.Status & FIFO_WRITE_BUSY) == 0u, PanicReason::USB_WREG_BUSY);

            const auto* source = static_cast<const std::uint8_t*>(buffer);
            const std::uint8_t words = size / sizeof(std::uint32_t);
            const std::uint8_t remainingBytes = size % sizeof(std::uint32_t);

            for (std::uint8_t i = 0; i < words; ++i)
            {
                std::uint32_t word;
                std::memcpy(&word, source + i * sizeof(std::uint32_t), sizeof(word));

                W_REG.Address = MemoryOffset + i * sizeof(std::uint32_t);
                W_REG.Data = word;
                W_REG.Status = FIFO_WRITE_BUSY;
                while ((W_REG.Status & FIFO_WRITE_BUSY) != 0u);
            }

            if (remainingBytes != 0)
            {
                std::uint32_t word = 0;
                std::memcpy(&word, source + words * sizeof(std::uint32_t), remainingBytes);

                W_REG.Address = MemoryOffset + words * sizeof(std::uint32_t);
                W_REG.Data = word;
                W_REG.Status = FIFO_WRITE_BUSY;
                while ((W_REG.Status & FIFO_WRITE_BUSY) != 0u);
            }
        });
    }

    std::uint8_t Endpoint::getReceivedSize() const noexcept
    {
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        return static_cast<std::uint8_t>(_getEndpointControl(EndpointNumber) & 0x7Fu);
    }

    inline bool Endpoint::isEndpointValid(const std::uint8_t endpointNumber) noexcept
    {
        return endpointNumber < MaxEndpoints;
    }

    inline volatile uint32_t& Endpoint::_getEndpointControl(const std::uint8_t endpointNumber) noexcept
    {
        HARD_ASSERTC(isEndpointValid(endpointNumber), PanicReason::ENDPT_INVALID);
        return *reinterpret_cast<volatile uint32_t*>(reinterpret_cast<std::uintptr_t>(&SN_USB->EP0CTL) + (endpointNumber * sizeof(uint32_t)));
    }

    void Endpoint::configure() noexcept
    {
        if (EndpointNumber != 0) 
        {
            const std::uint32_t directionBit = 1u << (EndpointNumber + 1u);
            // Even tho we have EndpointDirection::Both, it's unreachable for EndpointNumber != 0.
            // So we can just do a simple else and not worry about the Both case.
            // Assert just in case.
            HARD_ASSERTC(Direction != EndpointDirection::Both, PanicReason::ENDPT_NON0_DIR_BOTH);
            if (Direction == EndpointDirection::Out)
                SN_USB->CFG |= directionBit;
            else
                SN_USB->CFG &= ~directionBit;
            enable();
            if (Direction == EndpointDirection::Out) armOut();
        }
    }

    void Endpoint::deconfigure() noexcept
    {
        if (EndpointNumber != 0) 
        {
            disable();
            const std::uint32_t directionBit = 1u << (EndpointNumber + 1u);
            SN_USB->CFG &= ~directionBit;
        }
    }
}