ENTRY(Reset_Handler)

/* ==========================================================================
 * SN32F240B memory map
 * ========================================================================== */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
}

/* Main stack begins at the top of SRAM and grows downward. */
__initial_sp = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* ======================================================================
     * Flash-resident sections
     * ====================================================================== */

    .vectors ORIGIN(FLASH) :
    {
        . = ALIGN(4);

        KEEP(*(.vectors))
        KEEP(*(.isr_vector))

        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);

        *(.text)
        *(.text.*)

        *(.rodata)
        *(.rodata.*)

        *(.glue_7)
        *(.glue_7t)

        . = ALIGN(4);
    } > FLASH

        .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab*)
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        . = ALIGN(4);

        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;

        . = ALIGN(4);
    } > FLASH

    .preinit_array :
    {
        . = ALIGN(4);

        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        KEEP(*(.preinit_array.*))
        __preinit_array_end = .;

        . = ALIGN(4);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);

        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;

        . = ALIGN(4);
    } > FLASH

    /*
     * Initial .data contents are stored immediately after all normal
     * flash-resident runtime sections.
     */
    _data_load = ALIGN(
        LOADADDR(.init_array) + SIZEOF(.init_array),
        4
    );

    __data_load_address = _data_load;

    .boot_7ffc 0x00007FFC :
    {
        KEEP(*(.boot_7ffc))
    } > FLASH

    .boot_8000 0x00008000 :
    {
        KEEP(*(.boot_8000))
    } > FLASH

    .isp_flag 0x0000FFFC :
    {
        KEEP(*(.isp_flag))
    } > FLASH

    .data ORIGIN(RAM) : AT(_data_load)
    {
        . = ALIGN(4);
        _sdata = .;

        *(.data)
        *(.data.*)

        /*
         * Prevent an empty .data output section from receiving a bogus LMA.
         * Keep this after the real inputs so initialized objects stay aligned.
         */
        BYTE(0);

        . = ALIGN(4);
        _edata = .;
    } > RAM

    _sidata = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);

        _sbss = .;

        *(.bss)
        *(.bss.*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);

        _snoinit = .;
        *(.noinit)
        *(.noinit.*)
        _enoinit = .;

        . = ALIGN(4);
    } > RAM

    _end = .;
    __end__ = .;

    __heap_start = ALIGN(_end, 8);
    __heap_end   = __initial_sp;

    /* ======================================================================
     * Non-runtime metadata
     * ====================================================================== */

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }

    /* ======================================================================
     * Layout validation
     * ====================================================================== */

    ASSERT(
        (LOADADDR(.data) + SIZEOF(.data)) <= 0x00007FFC,
        "Firmware overlaps the fixed Sonix word at 0x7FFC"
    )

    ASSERT(
        _end <= (ORIGIN(RAM) + LENGTH(RAM)),
        "Firmware exceeds the available 8 KiB SRAM"
    )
}