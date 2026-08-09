#pragma once
#include <cstdint>

namespace quartz::utils {
    struct [[gnu::packed]] Color32 {
        std::uint8_t R;
        std::uint8_t G;
        std::uint8_t B;
    };
}