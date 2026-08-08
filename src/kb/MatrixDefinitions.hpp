#pragma once
#include <cstdint>

namespace quartz::kb {
    struct MatrixDefinitions {
        static constexpr std::uint8_t Rows = 7;
        static constexpr std::uint8_t Cols = 16;
        static constexpr std::uint8_t Size = Rows * Cols;
    };
}