#include "USB.hpp"
#include "../../usb/Interrupt.hpp"
#include "../timer/HighResolutionTimer.hpp"

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal {
    namespace {
        constexpr std::uint32_t USBClockEnable = 1u << 4;

        constexpr std::uint32_t USBConfig =
              (1u << 31) // VREG33
            | (1u << 30) // PHY
            | (1u << 28) // SIE
            | (1u << 27); // ESD

        constexpr std::uint32_t USBDPPU = 1u << 29; // D+ pull-up

        constexpr std::uint32_t PHYParameter1 = 0x80000000u;
        constexpr std::uint32_t PHYParameter2 = 0x00004004u;

        constexpr std::uint32_t EndpointEnable = 0x80000000u;
        constexpr std::uint32_t EndpointAck  =   0xA0000000u;
        constexpr std::uint32_t EndpointStall  = 0xE0000000u;

        constexpr std::uint32_t FifoWriteBusy = 1u << 0;
        constexpr std::uint32_t FifoReadBusy  = 1u << 1;

        constexpr std::uint32_t BusInterruptEnable = 1u << 31;
        constexpr std::uint32_t USBEventInterruptEnable = 1u << 29;
        constexpr std::uint32_t EndpointInterruptEnable = 1u << 4;

        constexpr std::uint32_t InitialInterruptMask =
            BusInterruptEnable |
            USBEventInterruptEnable |
            EndpointInterruptEnable;
            
    }

    volatile std::uint32_t& USB::endpointControl(const std::uint8_t endpoint) noexcept
    {
        return reinterpret_cast<volatile std::uint32_t*>(
            &SN_USB->EP0CTL
        )[endpoint];
    }

    void USB::initialize() noexcept
    {
        SN_SYS1->AHBCLKEN |= USBClockEnable;

        SN_USB->INTEN = InitialInterruptMask;
        SN_USB->SGCTL = 0u;

        SN_USB->PHYPRM  = PHYParameter1;
        SN_USB->PHYPRM2 = PHYParameter2;

        SN_USB->EP1BUFOS = 0x40;
        SN_USB->EP2BUFOS = 0x80;
        SN_USB->EP3BUFOS = 0xC0;
        SN_USB->EP4BUFOS = 0xE0;

        SN_USB->CFG = USBConfig;

        resetBus();

        NVIC_ClearPendingIRQ(USB_IRQn);
        NVIC_EnableIRQ(USB_IRQn);
    }

    void USB::connect() noexcept
    {
        SN_USB->CFG |= USBDPPU;
    }

    void USB::disconnect() noexcept
    {
        SN_USB->CFG &= ~USBDPPU;
    }

    void USB::resetBus() noexcept
    {
        SN_USB->INSTSC = 0xFFFFFFFFu;
        SN_USB->ADDR = 0u;

        enableEndpoint(0);

        for (std::uint8_t endpoint = 1; endpoint <= MaxEndpoint; ++endpoint) {
            disableEndpoint(endpoint);
        }
    }

    std::uint32_t USB::getInterruptStatus() noexcept
    {
        return SN_USB->INSTS;
    }

    void USB::clearInterruptStatus(const std::uint32_t mask) noexcept
    {
        SN_USB->INSTSC = mask;
    }

    std::uint32_t USB::readFifo32(const std::uint16_t offset) noexcept
    {
        SN_USB->RWADDR = offset;
        SN_USB->RWSTATUS = FifoReadBusy;

        while ((SN_USB->RWSTATUS & FifoReadBusy) != 0u) {
        }

        return SN_USB->RWDATA;
    }

    void USB::writeFifo32(const std::uint16_t offset, const std::uint32_t value) noexcept
    {
        SN_USB->RWADDR = offset;
        SN_USB->RWDATA = value;
        SN_USB->RWSTATUS = FifoWriteBusy;

        while ((SN_USB->RWSTATUS & FifoWriteBusy) != 0u) {
        }
    }

    std::uint16_t USB::getEndpointBufferOffset(const std::uint8_t endpoint) noexcept
    {
        switch (endpoint) {
            case 0:
                return 0x00;

            case 1:
                return static_cast<std::uint16_t>(
                    SN_USB->EP1BUFOS & 0xFCu
                );

            case 2:
                return static_cast<std::uint16_t>(
                    SN_USB->EP2BUFOS & 0xFCu
                );

            case 3:
                return static_cast<std::uint16_t>(
                    SN_USB->EP3BUFOS & 0xFCu
                );

            case 4:
                return static_cast<std::uint16_t>(
                    SN_USB->EP4BUFOS & 0xFCu
                );

            default:
                return 0;
        }
    }

    std::size_t USB::getEndpointByteCount(std::uint8_t endpoint) noexcept
    {
        if (endpoint > MaxEndpoint)
            return 0;

        return endpointControl(endpoint) & 0x7Fu;
    }

    void USB::readFifoAt(const std::uint16_t offset, std::uint8_t *const buffer, const std::size_t size) noexcept
    {
        if (buffer == nullptr || size == 0u || offset >= FifoSize) {
            return;
        }

        const std::size_t available = FifoSize - offset;
        const std::size_t actualSize =
            size < available ? size : available;

        std::size_t destinationIndex = 0u;
        std::uint16_t currentOffset = offset;

        while (destinationIndex < actualSize) {
            const std::uint16_t alignedOffset =
                static_cast<std::uint16_t>(currentOffset & ~0x3u);

            const std::uint32_t word = readFifo32(alignedOffset);

            const std::size_t firstByte =
                static_cast<std::size_t>(currentOffset & 0x3u);

            for (
                std::size_t byteIndex = firstByte;
                byteIndex < 4u && destinationIndex < actualSize;
                ++byteIndex
            ) {
                buffer[destinationIndex] =
                    static_cast<std::uint8_t>(
                        word >> (byteIndex * 8u)
                    );

                ++destinationIndex;
                ++currentOffset;
            }
        }
    }

    void USB::writeFifoAt(const std::uint16_t offset, const std::uint8_t* const buffer, const std::size_t size) noexcept
    {
        if (buffer == nullptr || size == 0u || offset >= FifoSize) {
            return;
        }

        const std::size_t available = FifoSize - offset;
        const std::size_t actualSize =
            size < available ? size : available;

        std::size_t sourceIndex = 0u;
        std::uint16_t currentOffset = offset;

        while (sourceIndex < actualSize) {
            const std::uint16_t alignedOffset =
                static_cast<std::uint16_t>(currentOffset & ~0x3u);

            const std::size_t firstByte =
                static_cast<std::size_t>(currentOffset & 0x3u);

            const std::size_t remaining = actualSize - sourceIndex;
            const std::size_t bytesThisWord =
                remaining < (4u - firstByte)
                    ? remaining
                    : (4u - firstByte);

            std::uint32_t word;

            if (firstByte == 0u && bytesThisWord == 4u) {
                // Entire DWORD is being replaced, so no read is needed.
                word = 0u;
            } else {
                // Preserve bytes outside the requested write range.
                word = readFifo32(alignedOffset);
            }

            for (
                std::size_t byteIndex = 0u;
                byteIndex < bytesThisWord;
                ++byteIndex
            ) {
                const std::size_t wordByteIndex =
                    firstByte + byteIndex;

                const std::uint32_t shift =
                    static_cast<std::uint32_t>(
                        wordByteIndex * 8u
                    );

                word &= ~(0xFFu << shift);

                word |=
                    static_cast<std::uint32_t>(
                        buffer[sourceIndex + byteIndex]
                    ) << shift;
            }

            writeFifo32(alignedOffset, word);

            sourceIndex += bytesThisWord;
            currentOffset = static_cast<std::uint16_t>(
                currentOffset + bytesThisWord
            );
        }
    }

    void USB::writeEndpointFifo(
        const std::uint8_t endpoint,
        const std::uint8_t* buffer,
        const std::size_t size
    ) noexcept
    {
        if (endpoint > MaxEndpoint)
            return;

        writeFifoAt(
            getEndpointBufferOffset(endpoint),
            buffer,
            size
        );
    }

    void USB::readEndpointFifo(
        const std::uint8_t endpoint,
        std::uint8_t* buffer,
        const std::size_t size
    ) noexcept
    {
        if (endpoint > MaxEndpoint)
            return;

        readFifoAt(
            getEndpointBufferOffset(endpoint),
            buffer,
            size
        );
    }

    void USB::enableEndpoint(const std::uint8_t endpoint) noexcept
    {
        if (endpoint > MaxEndpoint) {
            return;
        }

        endpointControl(endpoint) = EndpointEnable;
    }

    void USB::disableEndpoint(const std::uint8_t endpoint) noexcept
    {
        if (endpoint > MaxEndpoint) {
            return;
        }

        endpointControl(endpoint) = 0u;
    }

    void USB::stallEndpoint(const std::uint8_t endpoint) noexcept
    {
        if (endpoint > MaxEndpoint) {
            return;
        }

        endpointControl(endpoint) = EndpointStall;
    }

    void USB::setEndpointDirection(std::uint8_t endpoint, bool out) noexcept
    {
        if (endpoint == 0u || endpoint > MaxEndpoint) {
            return;
        }

        const std::uint32_t directionBit =
            1u << (endpoint - 1u);

        if (out) {
            SN_USB->CFG |= directionBit;
        } else {
            SN_USB->CFG &= ~directionBit;
        }
    }

    void USB::armInEndpoint(const std::uint8_t endpoint, const std::uint16_t size) noexcept
    {
        if (endpoint > MaxEndpoint || size > 0x7Fu) {
            return;
        }

        endpointControl(endpoint) =
            EndpointAck | static_cast<std::uint32_t>(size);
    }

    void USB::armOutEndpoint(std::uint8_t endpoint) noexcept
    {
        if (endpoint > MaxEndpoint) {
            return;
        }

        endpointControl(endpoint) = EndpointAck;
    }

    void USB::setAddress(const std::uint8_t address) noexcept
    {
        SN_USB->ADDR = address & 0x7Fu;
    }

    void USB::teardownForBootloader() noexcept
    {
        NVIC_DisableIRQ(USB_IRQn);
        NVIC_ClearPendingIRQ(USB_IRQn);

        SN_USB->INTEN = 0u;

        disconnect();
        hal::HighResolutionTimer::waitMilliseconds(20);

        SN_USB->EP0CTL = 0u;
        SN_USB->EP1CTL = 0u;
        SN_USB->EP2CTL = 0u;
        SN_USB->EP3CTL = 0u;
        SN_USB->EP4CTL = 0u;

        SN_USB->ADDR = 0u;
        SN_USB->INSTSC = 0xFFFFFFFFu;
        SN_USB->SGCTL = 0u;
        SN_USB->CFG &= ~(
              (1u << 30) // PHY_EN
            | (1u << 29) // DPPU_EN
            | (1u << 28) // SIE_EN
            | (1u << 27) // ESD_EN
        );
    }
}