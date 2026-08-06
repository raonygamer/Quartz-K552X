#include "timer.h"
#include "SN32F240B.h"

static volatile uint32_t g_SystemTick;

void SysTick_Handler() {
    g_SystemTick++;
}

void wait(uint32_t ms) {
    uint32_t start = g_SystemTick;
    while (g_SystemTick < start + ms) {
        __WFI();
    }
}

void timer_init()  {
    SysTick->CTRL = 0;
    SysTick->LOAD = 48000u - 1u;
    SysTick->VAL = 0;
    SysTick->CTRL =
            SysTick_CTRL_CLKSOURCE_Msk |
            SysTick_CTRL_TICKINT_Msk   |
            SysTick_CTRL_ENABLE_Msk;
}

