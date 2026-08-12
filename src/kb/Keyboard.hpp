#pragma once
#include <cstdint>

namespace quartz::kb
{
    class Keyboard
    {
        inline static std::uint32_t LastScanTicks = 0;
    public:
        static void scanAndSend();
    };
}
