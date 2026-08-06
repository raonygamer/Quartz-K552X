#pragma once
#include <cstdint>
#include <cstddef>

namespace quartz::debug {
    class DebugEndpoint {
    public:
        static void reset() noexcept;
        static void setConfigured(bool configured) noexcept;
        static std::size_t write(const std::uint8_t* data, std::size_t size) noexcept;
        static std::size_t writeString(const char* text) noexcept;
        static void handleTransmitComplete() noexcept;
        static std::size_t printf(const char* const format, ...) noexcept;

        [[nodiscard]]
        static std::uint32_t getDroppedByteCount() noexcept;

    private:
        static constexpr std::uint8_t Endpoint = 4;
        static constexpr std::uint16_t BufferOffset = 0xE0;
        static constexpr std::size_t MaxPacketSize = 32;
        static constexpr std::size_t QueueSize = 256;

        static void pump() noexcept;

        static inline std::uint8_t queue[QueueSize]{};
        static inline std::uint8_t transmitPacket[MaxPacketSize]{};

        static inline volatile std::size_t head = 0;
        static inline volatile std::size_t tail = 0;
        static inline volatile std::size_t count = 0;

        static inline volatile std::size_t pendingSize = 0;

        static inline volatile bool configured = false;
        static inline volatile bool transmitting = false;

        static inline volatile std::uint32_t droppedBytes = 0;
    };
}