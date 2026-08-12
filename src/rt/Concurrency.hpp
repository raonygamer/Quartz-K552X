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

        static void disableInterruptsAndExecute(const auto& func, const IRQn_Type type) noexcept
        {
            const bool enabled = NVIC_GetEnableIRQ(USB_IRQn) != 0;
            NVIC_DisableIRQ(type);
            func();
            if (enabled)
                NVIC_EnableIRQ(type);
        }
    };
}