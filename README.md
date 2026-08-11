# Quartz K552X

> A custom firmware for the Redragon Kumara K552W RGB PRO.

Quartz K552X is a personal firmware rewrite for the SN32F248B found in the K552W RGB PRO.

The goal isn't to clone the original firmware, but to build a clean, modern firmware from scratch while learning how the
hardware actually works (and probably questioning my life choices every now and then).

This project is written **for my own keyboard**. It's public because someone else might find it useful or interesting,
but I do **not** intend to support other keyboards or MCU variants.

## Goals

- Clean codebase written from scratch
- Modern freestanding C++
- USB HID keyboard
- NKRO
- Media keys
- Configurable RGB effects
- USB configuration utility (`quartz-cli`)
- Lightweight architecture
- No vendor SDK dependencies

### Future ideas *(hardware permitting / no promises)*

These are features I'd *love* to implement, but first I need to figure out whether the hardware actually exposes enough
information.

- Configurable Hall-effect actuation
- Rapid Trigger
- Per-key calibration

Maybe the hardware can do it.

Maybe it can't.

## Current Status

🚧 Very early development.

Currently working on:

- Startup/runtime
- Linker
- USB device stack
- Keyboard matrix scanning
- RGB engine

## Why?

Mostly because I wanted to.

The SN32F248B has a built-in ROM bootloader that makes experimentation surprisingly forgiving. If the firmware
completely explodes (which it occasionally does), holding the **BOOT** pin low during power-up jumps straight into the
bootloader, letting the firmware be flashed again.

That makes it a fantastic little platform for learning bare-metal firmware development without immediately turning the
keyboard into an expensive paperweight.

## Warning

This firmware is **only** intended for the **Redragon Kumara K552W RGB PRO** using the **SN32F248B** MCU.

Although this MCU is fairly forgiving thanks to its ROM bootloader, flashing custom firmware is always done **at your
own risk**.

Please **don't** flash this onto random Sonix keyboards just because they look similar. Different MCUs may have
different bootloaders—or none at all—and recovery might require dedicated programming hardware.

If you somehow manage to brick your keyboard, I can't magically unbrick it through GitHub issues.

## Building

Requirements:

- arm-none-eabi-gcc / arm-none-eabi-g++
- CMake
- Ninja

```bash
cmake -B build -G Ninja
cmake --build build
```

## License

GPL-v3
