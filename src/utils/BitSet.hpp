#pragma once
#include "MatrixPosition.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace quartz::utils
{
    template <std::size_t Size>
    class BitSet
    {
    public:
        std::array<std::uint8_t, (Size + 7) / 8> data = {};

        void zero() noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                data[i] = 0u;
            }
        }

        void setUnsafeU16(const std::uint16_t src, const std::size_t index) noexcept
        {
            auto* bytePtr = this->data.data() + index;
            *reinterpret_cast<std::uint16_t*>(bytePtr) = src;
        }

        template <std::size_t N>
        void setUnsafe(const std::uint8_t* src, const std::size_t index) noexcept
        {
            auto* bytePtr = this->data.data() + index;
            for (std::size_t i = 0; i < N; ++i)
            {
                *bytePtr++ = src[i];
            }
        }

        void set(std::size_t index) noexcept
        {
            if (index >= Size)
            {
                return;
            }

            const std::size_t byteIndex = index / 8;
            const std::size_t bitIndex = index % 8;

            data[byteIndex] |= (1u << bitIndex);
        }

        void clear(std::size_t index) noexcept
        {
            if (index >= Size)
            {
                return;
            }

            const std::size_t byteIndex = index / 8;
            const std::size_t bitIndex = index % 8;

            data[byteIndex] &= ~(1u << bitIndex);
        }

        std::uint8_t get(std::size_t index) const noexcept
        {
            if (index >= Size)
            {
                return 0u;
            }

            const std::size_t byteIndex = index / 8;
            const std::size_t bitIndex = index % 8;

            return (data[byteIndex] >> bitIndex) & 1u;
        }

        std::size_t size() const noexcept
        {
            return Size;
        }

        bool test(std::size_t index) const noexcept
        {
            return get(index) != 0u;
        }

        bool equals(const BitSet<Size>& other) const noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                if (data[i] != other.data[i])
                {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const BitSet<Size>& other) const noexcept
        {
            return equals(other);
        }

        bool operator!=(const BitSet<Size>& other) const noexcept
        {
            return !equals(other);
        }

        bool any() const noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                if (data[i] != 0u)
                {
                    return true;
                }
            }
            return false;
        }

        void cloneInto(BitSet<Size>& other) const noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                other.data[i] = data[i];
            }
        }
    };
}