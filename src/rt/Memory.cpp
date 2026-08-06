#include <cstddef>
#include <cstdint>

extern "C" {
    void* memset(void* const destination, const int value, std::size_t count) noexcept
    {
        auto* output =
            static_cast<volatile std::uint8_t*>(destination);

        const auto byte =
            static_cast<std::uint8_t>(value);

        while (count != 0u) {
            *output++ = byte;
            --count;
        }

        return destination;
    }

    void* memcpy(void* const destination, const void* const source, std::size_t count) noexcept
    {
        auto* output =
            static_cast<volatile std::uint8_t*>(destination);

        const auto* input =
            static_cast<const volatile std::uint8_t*>(source);

        while (count != 0u) {
            *output++ = *input++;
            --count;
        }

        return destination;
    }

    void* memmove(void* const destination, const void* const source, std::size_t count) noexcept
    {
        auto* output =
            static_cast<volatile std::uint8_t*>(destination);

        const auto* input =
            static_cast<const volatile std::uint8_t*>(source);

        if (output < input) {
            while (count != 0u) {
                *output++ = *input++;
                --count;
            }
        } else if (output > input) {
            output += count;
            input += count;

            while (count != 0u) {
                *--output = *--input;
                --count;
            }
        }

        return destination;
    }

    int memcmp(const void* const left, const void* const right, std::size_t count) noexcept
    {
        const auto* first =
            static_cast<const volatile std::uint8_t*>(left);

        const auto* second =
            static_cast<const volatile std::uint8_t*>(right);

        while (count != 0u) {
            if (*first != *second) {
                return static_cast<int>(*first) -
                    static_cast<int>(*second);
            }

            ++first;
            ++second;
            --count;
        }

        return 0;
    }

}