#pragma once

inline void busy_wait(const uint32_t val) {
    uint32_t i = val * (80 >> SN_SYS0->AHBCP);
    while (i--) {}
}

inline void delay(uint32_t count)
{
    while (count--)
    {
        SN_WDT->FEED = 0x5AFA55AA;
        busy_wait(100);
    }
}