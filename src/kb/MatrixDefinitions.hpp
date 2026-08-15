#pragma once
#include "utils/MatrixPosition.hpp"

#include <cstdint>

namespace quartz::kb
{
    struct MatrixDefinitions
    {
        static constexpr std::uint8_t Rows = 7;
        static constexpr std::uint8_t Cols = 16;
        static constexpr std::uint8_t Size = Rows * Cols;

        static constexpr std::size_t getKeyIndex(const uint8_t row, const uint8_t col) noexcept
        {
            return static_cast<std::size_t>(row) * Cols + col;
        }

        static constexpr utils::MatrixPosition getKeyPosition(const std::size_t index) noexcept
        {
            return utils::MatrixPosition{
                static_cast<std::uint8_t>(index / Cols),
                static_cast<std::uint8_t>(index % Cols)
            };
        }
    };
}