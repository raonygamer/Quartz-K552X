#pragma once
#include <stdint.h>

void SysTick_Handler();
void wait(uint32_t ms);
void timer_init();