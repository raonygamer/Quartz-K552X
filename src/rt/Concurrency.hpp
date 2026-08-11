#pragma once
#include "cppmcu.h"

namespace quartz::rt
{
    class Concurrency
    {
    public:
        inline static void enterCriticalSection() noexcept
        {
            __disable_irq();
        }

        inline static void exitCriticalSection() noexcept
        {
            __enable_irq();
        }

        inline static void executeInCriticalSection(const auto& func) noexcept
        {
            const auto mask = __get_PRIMASK();
            enterCriticalSection();
            func();
            if ((mask & 0x1u) == 0u)
                exitCriticalSection();
        }
    };
}