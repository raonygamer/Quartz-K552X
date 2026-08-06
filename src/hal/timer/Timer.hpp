#pragma once
#include <cstdint>

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::hal {
    class Timer {
    public:
        static volatile std::uint32_t CurrentTick;
        
        static void tick() noexcept;
        static void wait(std::uint32_t ms) noexcept;
    };
}