#include "hal/usb/Endpoint.hpp"
#include "rt/Concurrency.hpp"
#include "debug/Panic.hpp"
#include "cppmcu.h"

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

    Endpoint::Endpoint(
        const EndpointNumber number,
        const EndpointDirection direction,
        const std::uint8_t offset,
        const std::uint8_t maxSize
    ) noexcept :
        Number(number),
        Direction(direction),
        MemoryOffset(offset),
        MaxSize(maxSize)
    {
        HARD_ASSERTC(!(direction == EndpointDirection::Both && Number == EndpointNumber::EP0), PanicReason::ENDPT_INVALID_DIREC);
        HARD_ASSERTC(Number != EndpointNumber::EP0, PanicReason::ENDPT_EP0_NOT_CTRL);
        HARD_ASSERTC(Number <= EndpointNumber::EP4, PanicReason::ENDPT_INVALID_NUM);
        HARD_ASSERTC((MemoryOffset & 0x3u) == 0u, PanicReason::ENDPT_OFF_NALIGN);
        HARD_ASSERTC((MaxSize & 0x3u) == 0u, PanicReason::ENDPT_SZ_NALIGN);
        HARD_ASSERTC(MaxSize <= 64u, PanicReason::ENDPT_SZ_EXCEED);
        HARD_ASSERTC(static_cast<std::uint16_t>(MemoryOffset) + MaxSize <= 256u, PanicReason::ENDPT_SRAM_EXCEED);
    }

    void Endpoint::enable() const noexcept
    {
        _getEndpointControl(Number) = ENDPOINT_ENABLE;
    }

    void Endpoint::disable() const noexcept
    {
        _getEndpointControl(Number) = 0;
    }

    EndpointState Endpoint::getState() const noexcept
    {
        const std::uint32_t state = _getEndpointControl(Number) & ENDPOINT_STATE_MASK;
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

    void Endpoint::armIn(const std::size_t size) const noexcept
    {
        HARD_ASSERTC(isIn(), PanicReason::ENDPT_NIN_CFG);
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
        HARD_ASSERTC(size <= MaxSize, PanicReason::ENDPT_INARM_SZ_EXCEED);
        // Bits 6:0 == Size of data to be transmitted (in bytes)
        _getEndpointControl(Number) = ENDPOINT_READY | (size & 0x7Fu);
    }

    void Endpoint::armOut() const noexcept
    {
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
        _getEndpointControl(Number) = ENDPOINT_READY;
    }

    void Endpoint::stall() const noexcept
    {
        _getEndpointControl(Number) = ENDPOINT_STALL;
    }

    std::uint8_t Endpoint::getMemoryOffset() const noexcept
    {
        return MemoryOffset;
    }

    std::size_t Endpoint::getMaxSize() const noexcept
    {
        return MaxSize;
    }

    std::uint32_t Endpoint::read32() const noexcept
    {
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
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

    void Endpoint::write32(const std::uint32_t value) const noexcept
    {
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
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

    void Endpoint::readTo(const std::span<std::byte> buff) const noexcept
    {
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
        HARD_ASSERTC(isOut(), PanicReason::ENDPT_NOUT_CFG);
        HARD_ASSERTC(buff.data() != nullptr || buff.size() == 0, PanicReason::ENDPT_READ_NUL_BUF);
        HARD_ASSERTC(buff.size() <= MaxSize, PanicReason::ENDPT_READ_SZ_EXCEED);
        rt::Concurrency::executeInCriticalSection([this, buff]()
        {
            HARD_ASSERTC((R_REG.Status & FIFO_READ_BUSY) == 0u, PanicReason::USB_RREG_BUSY);
            auto* destination = reinterpret_cast<std::uint8_t*>(buff.data());
            const std::uint8_t words = buff.size() / sizeof(std::uint32_t);
            const std::uint8_t remainingBytes = buff.size() % sizeof(std::uint32_t);
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

    void Endpoint::writeFrom(const std::span<const std::byte> buff) const noexcept
    {
        HARD_ASSERTC(isIdle(), PanicReason::ENDPT_NOT_IDLE);
        HARD_ASSERTC(isIn(), PanicReason::ENDPT_NIN_CFG);
        HARD_ASSERTC(buff.data() != nullptr || buff.size() == 0, PanicReason::ENDPT_WRITE_NUL_BUF);
        HARD_ASSERTC(buff.size() <= MaxSize, PanicReason::ENDPT_WRITE_SZ_EXCEED);
        rt::Concurrency::executeInCriticalSection([this, buff]()
        {
            HARD_ASSERTC((W_REG.Status & FIFO_WRITE_BUSY) == 0u, PanicReason::USB_WREG_BUSY);
            const auto* source = reinterpret_cast<const std::uint8_t*>(buff.data());
            const std::uint8_t words = buff.size() / sizeof(std::uint32_t);
            const std::uint8_t remainingBytes = buff.size() % sizeof(std::uint32_t);
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
        return static_cast<std::uint8_t>(_getEndpointControl(Number) & 0x7Fu);
    }

    inline bool Endpoint::isEndpointValid(const EndpointNumber number) noexcept
    {
        return value(number) < MAX_ENDPOINTS;
    }

    inline volatile std::uint32_t& Endpoint::_getEndpointControl(const EndpointNumber number) noexcept
    {
        HARD_ASSERTC(isEndpointValid(number), PanicReason::ENDPT_INVALID);
        return (&SN_USB->EP0CTL)[value(number)];
    }

    void Endpoint::configure() const noexcept
    {
        if (Number == EndpointNumber::EP0)
            return;
        const std::uint32_t directionBit = 1u << (value(Number) + 1u);
        if (Direction == EndpointDirection::Out)
            SN_USB->CFG |= directionBit;
        else
            SN_USB->CFG &= ~directionBit;
        enable();
    }

    void Endpoint::deconfigure() const noexcept
    {
        if (Number == EndpointNumber::EP0)
            return;
        disable();
        const std::uint32_t directionBit = 1u << (value(Number) + 1u);
        SN_USB->CFG &= ~directionBit;
    }
}
