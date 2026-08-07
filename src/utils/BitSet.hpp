#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include "Position2D.hpp"

namespace quartz::utils {
    template <std::size_t Size>
    class BitSet {
    public:
        std::array<std::uint8_t, (Size + 7) / 8> data{};

        void zero() noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i) {
                data[i] = 0u;
            }
        }

        void set(std::size_t index) noexcept
        {
            if (index >= Size) {
                return;
            }

            const std::size_t byteIndex = index / 8;
            const std::size_t bitIndex = index % 8;

            data[byteIndex] |= (1u << bitIndex);
        }

        void clear(std::size_t index) noexcept
        {
            if (index >= Size) {
                return;
            }

            const std::size_t byteIndex = index / 8;
            const std::size_t bitIndex = index % 8;

            data[byteIndex] &= ~(1u << bitIndex);
        }

        std::uint8_t get(std::size_t index) const noexcept
        {
            if (index >= Size) {
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
            for (std::size_t i = 0; i < data.size(); ++i) {
                if (data[i] != other.data[i]) {
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
            for (std::size_t i = 0; i < data.size(); ++i) {
                if (data[i] != 0u) {
                    return true;
                }
            }
            return false;
        }

        void cloneInto(BitSet<Size>& other) noexcept
        {
            for (std::size_t i = 0; i < data.size(); ++i) {
                other.data[i] = data[i];
            }
        }

        std::size_t firstOnePositions(Position2D* buffer, std::size_t maxPositions, Position2D(*matrixConversor)(std::size_t)) const noexcept
        {
            if (!matrixConversor) {
                return 0;
            }

            std::size_t count = 0;
            for (std::size_t index = 0; index < Size && count < maxPositions; ++index) {
                if (test(index)) {
                    buffer[count++] = matrixConversor(index);
                }
            }
            return count;
        }
    };
}