#include "DebugEndpoint.hpp"
#include "../hal/usb/USB.hpp"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::debug {
    namespace {
        class FormatBuffer {
        public:
            static constexpr std::size_t Capacity = 192u;

            void append(const char character) noexcept
            {
                if (size_ < Capacity) {
                    data_[size_++] = character;
                } else {
                    truncated_ = true;
                }
            }

            void append(const char* text) noexcept
            {
                if (text == nullptr) {
                    text = "(null)";
                }

                while (*text != '\0') {
                    append(*text++);
                }
            }

            [[nodiscard]]
            const std::uint8_t* data() const noexcept
            {
                return reinterpret_cast<const std::uint8_t*>(data_);
            }

            [[nodiscard]]
            std::size_t size() const noexcept
            {
                return size_;
            }

            [[nodiscard]]
            bool truncated() const noexcept
            {
                return truncated_;
            }

        private:
            char data_[Capacity]{};
            std::size_t size_ = 0u;
            bool truncated_ = false;
        };

        void appendUnsigned(
            FormatBuffer& output,
            std::uint32_t value,
            const std::uint32_t base,
            const bool uppercase
        ) noexcept
        {
            const char* digits = uppercase
                ? "0123456789ABCDEF"
                : "0123456789abcdef";

            char temporary[32];
            std::size_t length = 0u;

            do {
                temporary[length++] = digits[value % base];
                value /= base;
            } while (value != 0u);

            while (length != 0u) {
                output.append(temporary[--length]);
            }
        }

        void appendSigned(
            FormatBuffer& output,
            const std::int32_t value
        ) noexcept
        {
            if (value < 0) {
                output.append('-');

                /*
                * Avoid overflowing on INT32_MIN by converting through unsigned
                * arithmetic rather than evaluating -value directly.
                */
                const std::uint32_t magnitude =
                    0u - static_cast<std::uint32_t>(value);

                appendUnsigned(output, magnitude, 10u, false);
                return;
            }

            appendUnsigned(
                output,
                static_cast<std::uint32_t>(value),
                10u,
                false
            );
        }
    }


    namespace {
        [[nodiscard]]
        std::uint32_t enterCriticalSection() noexcept
        {
            const std::uint32_t previousState = __get_PRIMASK();
            __disable_irq();
            return previousState;
        }

        void leaveCriticalSection(
            const std::uint32_t previousState
        ) noexcept
        {
            if ((previousState & 1u) == 0u) {
                __enable_irq();
            }
        }
    }

    std::size_t DebugEndpoint::write(const std::uint8_t* const data, const std::size_t size) noexcept
    {
        if (data == nullptr || size == 0u) {
            return 0u;
        }

        const std::uint32_t interruptState =
            enterCriticalSection();

        std::size_t accepted = 0u;

        while (accepted < size && count < QueueSize) {
            queue[head] = data[accepted];

            head = (head + 1u) % QueueSize;
            ++count;
            ++accepted;
        }

        droppedBytes += static_cast<std::uint32_t>(
            size - accepted
        );

        pump();

        leaveCriticalSection(interruptState);

        return accepted;
    }

    std::size_t DebugEndpoint::writeString(const char* const text) noexcept
    {
        if (text == nullptr) {
            return 0u;
        }

        std::size_t length = 0u;

        while (text[length] != '\0') {
            ++length;
        }

        return write(
            reinterpret_cast<const std::uint8_t*>(text),
            length
        );
    }

    std::size_t DebugEndpoint::printf(
        const char* const format,
        ...
    ) noexcept
    {
        if (format == nullptr) {
            return 0u;
        }

        FormatBuffer output;

        std::va_list arguments;
        va_start(arguments, format);

        for (const char* current = format;
            *current != '\0';
            ++current) {
            if (*current != '%') {
                output.append(*current);
                continue;
            }

            ++current;

            if (*current == '\0') {
                output.append('%');
                break;
            }

            switch (*current) {
                case '%':
                    output.append('%');
                    break;

                case 'c': {
                    const int value = va_arg(arguments, int);
                    output.append(static_cast<char>(value));
                    break;
                }

                case 's': {
                    const char* value =
                        va_arg(arguments, const char*);

                    output.append(value);
                    break;
                }

                case 'd':
                case 'i': {
                    const int value = va_arg(arguments, int);

                    appendSigned(
                        output,
                        static_cast<std::int32_t>(value)
                    );
                    break;
                }

                case 'u': {
                    const unsigned int value =
                        va_arg(arguments, unsigned int);

                    appendUnsigned(
                        output,
                        static_cast<std::uint32_t>(value),
                        10u,
                        false
                    );
                    break;
                }

                case 'x':
                case 'X': {
                    const unsigned int value =
                        va_arg(arguments, unsigned int);

                    appendUnsigned(
                        output,
                        static_cast<std::uint32_t>(value),
                        16u,
                        *current == 'X'
                    );
                    break;
                }

                case 'p': {
                    const void* pointer =
                        va_arg(arguments, const void*);

                    output.append("0x");

                    appendUnsigned(
                        output,
                        static_cast<std::uint32_t>(
                            reinterpret_cast<std::uintptr_t>(pointer)
                        ),
                        16u,
                        false
                    );
                    break;
                }

                default:
                    // Preserve unknown specifiers visibly.
                    output.append('%');
                    output.append(*current);
                    break;
            }
        }

        va_end(arguments);

        return write(output.data(), output.size());
    }
    
    void DebugEndpoint::pump() noexcept
    {
        if (!configured || transmitting || count == 0u) {
            return;
        }

        const std::size_t packetSize =
            count < MaxPacketSize
                ? count
                : MaxPacketSize;

        std::size_t queueIndex = tail;

        for (std::size_t index = 0u;
            index < packetSize;
            ++index) {
            transmitPacket[index] = queue[queueIndex];
            queueIndex = (queueIndex + 1u) % QueueSize;
        }

        hal::USB::writeFifo(
            BufferOffset,
            transmitPacket,
            packetSize
        );

        pendingSize = packetSize;
        transmitting = true;

        hal::USB::armInEndpoint(
            Endpoint,
            static_cast<std::uint16_t>(packetSize)
        );
    }

    void DebugEndpoint::handleTransmitComplete() noexcept
    {
        if (!transmitting) {
            return;
        }

        tail = (tail + pendingSize) % QueueSize;
        count -= pendingSize;

        pendingSize = 0u;
        transmitting = false;

        pump();
    }

    void DebugEndpoint::reset() noexcept
    {
        const std::uint32_t interruptState =
            enterCriticalSection();

        head = 0u;
        tail = 0u;
        count = 0u;
        pendingSize = 0u;

        configured = false;
        transmitting = false;
        droppedBytes = 0u;

        leaveCriticalSection(interruptState);
    }

    void DebugEndpoint::setConfigured(const bool newConfigured) noexcept
    {
        configured = newConfigured;

        if (!configured) {
            transmitting = false;
            pendingSize = 0u;
            return;
        }

        pump();
    }

    std::uint32_t DebugEndpoint::getDroppedByteCount() noexcept
    {
        return droppedBytes;
    }
}