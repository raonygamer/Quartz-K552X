#pragma once
#include <cstdint>

namespace quartz::kb {
    class Keyboard {
    public:
        inline static std::uint32_t LastScanTicks = 0;

        static void scanAndSend();
    };
}