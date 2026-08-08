#pragma once

namespace quartz::utils {
    struct MatrixPosition {
        int Row = 0;
        int Col = 0;

        constexpr MatrixPosition() = default;
        constexpr MatrixPosition(int row, int col) : 
            Row(row), 
            Col(col) 
        {
        }

        constexpr bool isValid() const noexcept
        {
            return Row >= 0 && Col >= 0;
        }
    };
}