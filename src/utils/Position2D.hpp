#pragma once

namespace quartz::utils {
    struct Position2D {
        int x = 0;
        int y = 0;

        constexpr Position2D() = default;
        constexpr Position2D(int x, int y) : x(x), y(y) {}
    };
}