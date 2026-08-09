#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal {
    class USB {
    public:
        static constexpr std::uint8_t MaxEndpoint = 4;
        static constexpr std::uint32_t FifoSize = 256;

        static void initialize() noexcept;
        static void connect() noexcept;
        static void disconnect() noexcept;
        static void resetBus() noexcept;

        static std::uint32_t getInterruptStatus() noexcept;
        static void clearInterruptStatus(std::uint32_t mask) noexcept;

        static std::uint32_t readFifo32(std::uint16_t offset) noexcept;
        static void writeFifo32(std::uint16_t offset, std::uint32_t value) noexcept;

        static std::uint16_t getEndpointBufferOffset(const std::uint8_t endpoint) noexcept;
        static std::size_t getEndpointByteCount(std::uint8_t endpoint) noexcept;
        static void readFifoAt(std::uint16_t offset, std::uint8_t* buffer, std::size_t size) noexcept;
        static void writeFifoAt(std::uint16_t offset, const std::uint8_t* buffer, std::size_t size) noexcept;

        static void writeEndpointFifo(
            const std::uint8_t endpoint,
            const std::uint8_t* buffer,
            const std::size_t size
        ) noexcept;

        static void readEndpointFifo(
            const std::uint8_t endpoint,
            std::uint8_t* buffer,
            const std::size_t size
        ) noexcept;

        static void enableEndpoint(std::uint8_t endpoint) noexcept;
        static void disableEndpoint(std::uint8_t endpoint) noexcept;
        static void stallEndpoint(std::uint8_t endpoint) noexcept;
        static void setEndpointDirection(std::uint8_t endpoint, bool out) noexcept;

        static void armInEndpoint(std::uint8_t endpoint, std::uint16_t size) noexcept;
        static void armOutEndpoint(std::uint8_t endpoint) noexcept;
        static void setAddress(std::uint8_t address) noexcept;
        static void teardownForBootloader() noexcept;

    private:
        static volatile std::uint32_t& endpointControl(std::uint8_t endpoint) noexcept;
        inline static volatile std::uint8_t* usbSram() noexcept
        {
            return reinterpret_cast<volatile std::uint8_t*>(
                reinterpret_cast<std::uintptr_t>(SN_USB) + 0x100u
            );
        }
    };
}