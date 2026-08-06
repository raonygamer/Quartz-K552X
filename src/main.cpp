#include <stdint.h>

/* The HAL is still compiled as C, so its public functions need C linkage. */
extern "C" {
#include "SN32F240B.h"
#include "gpio.h"
#include "usb/usb.h"
#include "system.h"
#include "timer.h"
}

namespace {
    void gpio_init() noexcept
    {
    }
}

/* startup.S references the exact unmangled symbol "main". */
extern "C" int main()
{
    gpio_init();

    gpio_mode(GPIO1, 14, true);
    gpio_mode(GPIO1, 15, true);

    timer_init();
    //usb_init();

    bool value = false;

    for (uint32_t count = 0; count < 20; ++count) {
        value = !value;
        gpio_value(GPIO1, 14, value);
        wait(500);
    }

    reboot_to_bootloader();

    /* Defensive fallback if the bootloader jump unexpectedly returns. */
    for (;;) {
        __asm volatile("wfi");
    }
}