#pragma once
#include <cstdint>

namespace quartz::usb::hid
{
    enum class KeyboardUsage : std::uint8_t
    {
        None = 0x00,
        ErrorRollOver = 0x01,

        A = 0x04,
        B = 0x05,
        C = 0x06,
        D = 0x07,
        E = 0x08,
        F = 0x09,
        G = 0x0A,
        H = 0x0B,
        I = 0x0C,
        J = 0x0D,
        K = 0x0E,
        L = 0x0F,
        M = 0x10,
        N = 0x11,
        O = 0x12,
        P = 0x13,
        Q = 0x14,
        R = 0x15,
        S = 0x16,
        T = 0x17,
        U = 0x18,
        V = 0x19,
        W = 0x1A,
        X = 0x1B,
        Y = 0x1C,
        Z = 0x1D,

        Digit1 = 0x1E,
        Digit2 = 0x1F,
        Digit3 = 0x20,
        Digit4 = 0x21,
        Digit5 = 0x22,
        Digit6 = 0x23,
        Digit7 = 0x24,
        Digit8 = 0x25,
        Digit9 = 0x26,
        Digit0 = 0x27,

        Enter = 0x28,
        Escape = 0x29,
        Backspace = 0x2A,
        Tab = 0x2B,
        Space = 0x2C,

        Minus = 0x2D,
        Equal = 0x2E,
        LeftBracket = 0x2F,
        RightBracket = 0x30,
        Backslash = 0x31,
        NonUSHash = 0x32,
        Semicolon = 0x33,
        Apostrophe = 0x34,
        GraveAccent = 0x35,
        Comma = 0x36,
        Period = 0x37,
        Slash = 0x38,

        CapsLock = 0x39,

        F1 = 0x3A,
        F2 = 0x3B,
        F3 = 0x3C,
        F4 = 0x3D,
        F5 = 0x3E,
        F6 = 0x3F,
        F7 = 0x40,
        F8 = 0x41,
        F9 = 0x42,
        F10 = 0x43,
        F11 = 0x44,
        F12 = 0x45,

        PrintScreen = 0x46,
        ScrollLock = 0x47,
        Pause = 0x48,

        Insert = 0x49,
        Home = 0x4A,
        PageUp = 0x4B,
        Delete = 0x4C,
        End = 0x4D,
        PageDown = 0x4E,

        RightArrow = 0x4F,
        LeftArrow = 0x50,
        DownArrow = 0x51,
        UpArrow = 0x52,

        // ISO/ABNT2 key near Left Shift.
        NonUSBackslash = 0x64,

        Application = 0x65,

        // Explicitly specified by USB-IF for the Brazilian / ? key.
        International1 = 0x87,

        LeftControl = 0xE0,
        LeftShift = 0xE1,
        LeftAlt = 0xE2,
        LeftGUI = 0xE3,
        RightControl = 0xE4,
        RightShift = 0xE5,
        RightAlt = 0xE6,
        RightGUI = 0xE7,
    };

    enum class ConsumerUsage : std::uint16_t
    {
        None = 0x0000,

        AudioPlayer = 0x01C7,       // F1
        VolumeDecrement = 0x00EA,   // F2
        VolumeIncrement = 0x00E9,   // F3
        Mute = 0x00E2,              // F4
        Stop = 0x00B7,              // F5
        ScanPreviousTrack = 0x00B6, // F6
        PlayPause = 0x00CD,         // F7
        ScanNextTrack = 0x00B5,     // F8
        EmailReader = 0x018A,       // F9
        Home = 0x0223,              // F10
        Calculator = 0x0192,        // F11
        Search = 0x0221,            // F12
    };

    enum class KeyboardLED : std::uint8_t
    {
        NumLock = 1u << 0,
        CapsLock = 1u << 1,
        ScrollLock = 1u << 2,
        Compose = 1u << 3,
        Kana = 1u << 4,
    };
}

namespace quartz::kb
{
    using Key = quartz::usb::hid::KeyboardUsage;
}