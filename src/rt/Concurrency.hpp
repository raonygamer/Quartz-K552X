#pragma once
#include "cppmcu.h"

namespace quartz::rt
{
    class Concurrency
    {
    public:
        static void disableInterruptsAndExecute(const auto& func, const IRQn_Type type) noexcept
        {
            const bool enabled = NVIC_GetEnableIRQ(type) != 0;
            NVIC_DisableIRQ(type);
            func();
            if (enabled)
                NVIC_EnableIRQ(type);
        }
    };
}