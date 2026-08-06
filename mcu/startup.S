.syntax unified
.cpu cortex-m0
.thumb

/* ==========================================================================
 * External linker/runtime symbols
 * ========================================================================== */

.extern __initial_sp

.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss

.extern SystemInit
.extern SystemCoreClockUpdate
.extern __libc_init_array
.extern main

/* ==========================================================================
 * Interrupt vector table
 * ========================================================================== */

.section .vectors, "a", %progbits
.align 2

.global __Vectors
.global __Vectors_End
.global __Vectors_Size
.type __Vectors, %object

__Vectors:
    /* Cortex-M0 core exceptions */
    .word __initial_sp
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word 0
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler

    /* External interrupts: IRQ 0–31 */
    .word NDT_IRQHandler          /* IRQ 0  */
    .word USB_IRQHandler          /* IRQ 1  */
    .word 0                       /* IRQ 2  */
    .word 0                       /* IRQ 3  */
    .word 0                       /* IRQ 4  */
    .word 0                       /* IRQ 5  */
    .word SPI0_IRQHandler         /* IRQ 6  */
    .word 0                       /* IRQ 7  */
    .word 0                       /* IRQ 8  */
    .word 0                       /* IRQ 9  */
    .word I2C0_IRQHandler         /* IRQ 10 */
    .word 0                       /* IRQ 11 */
    .word UART0_IRQHandler        /* IRQ 12 */
    .word UART1_IRQHandler        /* IRQ 13 */
    .word UART2_IRQHandler        /* IRQ 14 */
    .word CT16B0_IRQHandler       /* IRQ 15 */
    .word CT16B1_IRQHandler       /* IRQ 16 */
    .word 0                       /* IRQ 17 */
    .word 0                       /* IRQ 18 */
    .word 0                       /* IRQ 19 */
    .word 0                       /* IRQ 20 */
    .word 0                       /* IRQ 21 */
    .word 0                       /* IRQ 22 */
    .word 0                       /* IRQ 23 */
    .word ADC_IRQHandler          /* IRQ 24 */
    .word WDT_IRQHandler          /* IRQ 25 */
    .word LVD_IRQHandler          /* IRQ 26 */
    .word 0                       /* IRQ 27 */
    .word P3_IRQHandler           /* IRQ 28 */
    .word P2_IRQHandler           /* IRQ 29 */
    .word P1_IRQHandler           /* IRQ 30 */
    .word P0_IRQHandler           /* IRQ 31 */

__Vectors_End:
.equ __Vectors_Size, __Vectors_End - __Vectors

.size __Vectors, . - __Vectors

/* ==========================================================================
 * Sonix fixed flash words
 *
 * Placement is controlled by linker.ld. Do not use .org here.
 * ========================================================================== */

.section .boot_7ffc, "a", %progbits
.align 2
.word 0xBF00BF00

.section .boot_8000, "a", %progbits
.align 2
.word 0xBF00BF00

.section .isp_flag, "a", %progbits
.align 2
.word 0xAAAA5555

/* ==========================================================================
 * Reset handler
 * ========================================================================== */

.section .text.Reset_Handler, "ax", %progbits
.align 2

.global Reset_Handler
.type Reset_Handler, %function
.thumb_func

Reset_Handler:
    ldr r0, =Runtime_Entry
    bx  r0

.size Reset_Handler, . - Reset_Handler

/* ==========================================================================
 * Minimal C/C++ runtime entry
 * ========================================================================== */

.section .text.Runtime_Entry, "ax", %progbits
.align 2

.global Runtime_Entry
.type Runtime_Entry, %function
.thumb_func

Runtime_Entry:
    /* Copy initialized .data from flash to RAM. */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata

.L_copy_data:
    cmp  r1, r2
    bcs  .L_zero_bss

    ldr  r3, [r0]
    str  r3, [r1]

    adds r0, #4
    adds r1, #4
    b    .L_copy_data

.L_zero_bss:
    /* Zero the .bss section. */
    ldr  r0, =_sbss
    ldr  r1, =_ebss
    movs r2, #0

.L_zero_bss_loop:
    cmp  r0, r1
    bcs  .L_initialize_runtime

    str  r2, [r0]
    adds r0, #4
    b    .L_zero_bss_loop

.L_initialize_runtime:
    /*
     * RAM now contains valid global state, so SystemInit may safely
     * access ordinary static/global variables if needed.
     */
    bl SystemInit

    /*
     * Update SystemCoreClock
     */
    bl SystemCoreClockUpdate

    /*
     * Run C++ global constructors registered in .preinit_array and
     * .init_array.
     */
    bl __libc_init_array

    /* Enter the application. */
    bl main

    /* main() must never return. */
.L_main_returned:
    b .L_main_returned

.size Runtime_Entry, . - Runtime_Entry

/* ==========================================================================
 * Default interrupt handler
 * ========================================================================== */

.section .text.Default_Handler, "ax", %progbits
.align 2

.global Default_Handler
.type Default_Handler, %function
.thumb_func

Default_Handler:
.L_default_handler:
    b .L_default_handler

.size Default_Handler, . - Default_Handler

/* ==========================================================================
 * Weak aliases
 *
 * Defining any handler with the same name in C replaces the alias.
 * ========================================================================== */

.macro WEAK_DEFAULT name
    .weak \name
    .thumb_set \name, Default_Handler
.endm

WEAK_DEFAULT NMI_Handler
WEAK_DEFAULT HardFault_Handler
WEAK_DEFAULT SVC_Handler
WEAK_DEFAULT PendSV_Handler
WEAK_DEFAULT SysTick_Handler

WEAK_DEFAULT NDT_IRQHandler
WEAK_DEFAULT USB_IRQHandler
WEAK_DEFAULT SPI0_IRQHandler
WEAK_DEFAULT I2C0_IRQHandler
WEAK_DEFAULT UART0_IRQHandler
WEAK_DEFAULT UART1_IRQHandler
WEAK_DEFAULT UART2_IRQHandler
WEAK_DEFAULT CT16B0_IRQHandler
WEAK_DEFAULT CT16B1_IRQHandler
WEAK_DEFAULT ADC_IRQHandler
WEAK_DEFAULT WDT_IRQHandler
WEAK_DEFAULT LVD_IRQHandler
WEAK_DEFAULT P3_IRQHandler
WEAK_DEFAULT P2_IRQHandler
WEAK_DEFAULT P1_IRQHandler
WEAK_DEFAULT P0_IRQHandler