#include "system.h"

void reboot_to_bootloader() {
    SN_SYS0->IVTM = 0;
    __asm volatile(
        "ldr r0, =0x1fff0301\n"
        "bx  r0\n"
        :
        :
        : "r0", "memory"
    );
    __builtin_unreachable();
}
