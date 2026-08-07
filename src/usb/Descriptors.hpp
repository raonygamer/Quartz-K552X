#pragma once

#include <array>
#include <cstdint>

#include "usb/hid/BootKeyboardReport.hpp"

namespace quartz::usb::descriptors {
    struct HIDKeyboard {
        static constexpr std::uint8_t InterfaceNumber = 0;

        static constexpr std::uint8_t EndpointNumber  = 1;
        static constexpr std::uint8_t EndpointAddress = 0x81;

        static constexpr std::uint16_t MaxPacketSize =
            sizeof(hid::BootKeyboardReport);

        inline static constexpr std::array<std::uint8_t, 65>
            ReportDescriptor = {
            // Usage Page (Generic Desktop)
            0x05, 0x01,

            // Usage (Keyboard)
            0x09, 0x06,

            // Collection (Application)
            0xA1, 0x01,

            // Modifier byte
            0x05, 0x07, // Usage Page (Keyboard)
            0x19, 0xE0, // Usage Minimum (Left Control)
            0x29, 0xE7, // Usage Maximum (Right GUI)
            0x15, 0x00, // Logical Minimum (0)
            0x25, 0x01, // Logical Maximum (1)
            0x75, 0x01, // Report Size (1)
            0x95, 0x08, // Report Count (8)
            0x81, 0x02, // Input (Data, Variable, Absolute)

            // Reserved byte
            0x95, 0x01, // Report Count (1)
            0x75, 0x08, // Report Size (8)
            0x81, 0x01, // Input (Constant)

            // Keyboard LEDs
            0x95, 0x05, // Report Count (5)
            0x75, 0x01, // Report Size (1)
            0x05, 0x08, // Usage Page (LEDs)
            0x19, 0x01, // Usage Minimum (Num Lock)
            0x29, 0x05, // Usage Maximum (Kana)
            0x91, 0x02, // Output (Data, Variable, Absolute)

            // LED padding
            0x95, 0x01,
            0x75, 0x03,
            0x91, 0x01, // Output (Constant)

            // Six ordinary key usages
            0x95, 0x06,       // Report Count (6)
            0x75, 0x08,       // Report Size (8)
            0x15, 0x00,       // Logical Minimum (0)
            0x26, 0xA4, 0x00, // Logical Maximum (0xA4)
            0x05, 0x07,       // Usage Page (Keyboard)
            0x19, 0x00,       // Usage Minimum (0)
            0x2A, 0xA4, 0x00, // Usage Maximum (0xA4)
            0x81, 0x00,       // Input (Data, Array, Absolute)

            // End Collection
            0xC0,
        };

        inline static constexpr std::array<std::uint8_t, 9>
            Descriptor = {
            9,
            0x21,       // HID descriptor
            0x11, 0x01, // HID 1.11
            0x00,       // bCountryCode
            1,          // bNumDescriptors
            0x22,       // Report descriptor
            static_cast<std::uint8_t>(
                ReportDescriptor.size() & 0xFF
            ),
            static_cast<std::uint8_t>(
                ReportDescriptor.size() >> 8
            ),
        };
    };

    struct Debug {
        static constexpr std::uint8_t InterfaceNumber = 1;

        static constexpr std::uint8_t EndpointNumber  = 4;
        static constexpr std::uint8_t EndpointAddress = 0x84;

        static constexpr std::uint16_t MaxPacketSize = 32;
    };

    inline constexpr std::array<std::uint8_t, 18> Device = {
        18,
        0x01,

        0x00, 0x02, // USB 2.00

        // Class defined by individual interfaces.
        0x00,
        0x00,
        0x00,

        64,

        0x47, 0xB1, // VID: 0xB147
        0x31, 0x41, // PID: 0x4131

        0x00, 0x01,

        1,
        2,
        3,

        1,
    };

    inline constexpr std::array<std::uint8_t, 50> Configuration = {
        // ---------------------------------------------------------
        // Configuration
        // ---------------------------------------------------------
        9,
        0x02,
        50, 0, // wTotalLength
        2,     // bNumInterfaces
        1,
        0,
        0x80,
        50,

        // ---------------------------------------------------------
        // Interface 0: HID boot keyboard
        // ---------------------------------------------------------
        9,
        0x04,
        HIDKeyboard::InterfaceNumber,
        0,
        1,    // one endpoint
        0x03, // HID
        0x01, // Boot Interface Subclass
        0x01, // Keyboard protocol
        0,

        // HID descriptor
        9,
        0x21,
        0x11, 0x01,
        0,
        1,
        0x22,
        static_cast<std::uint8_t>(
            HIDKeyboard::ReportDescriptor.size() & 0xFF
        ),
        static_cast<std::uint8_t>(
            HIDKeyboard::ReportDescriptor.size() >> 8
        ),

        // EP1 IN - keyboard reports
        7,
        0x05,
        HIDKeyboard::EndpointAddress,
        0x03, // Interrupt
        static_cast<std::uint8_t>(
            HIDKeyboard::MaxPacketSize
        ),
        0,
        1, // poll every 1 ms

        // ---------------------------------------------------------
        // Interface 1: Quartz vendor/debug
        // ---------------------------------------------------------
        9,
        0x04,
        Debug::InterfaceNumber,
        0,
        1,
        0xFF,
        0,
        0,
        0,

        // EP4 IN
        7,
        0x05,
        Debug::EndpointAddress,
        0x02, // Bulk
        static_cast<std::uint8_t>(
            Debug::MaxPacketSize
        ),
        0,
        0,
    };

    inline constexpr std::array<std::uint8_t, 4> Language = {
        4,
        0x03,
        0x09, 0x04,
    };

    inline constexpr std::array<std::uint8_t, 14> Manufacturer = {
        14,
        0x03,

        'S', 0,
        'a', 0,
        't', 0,
        'u', 0,
        'r', 0,
        'n', 0,
    };

    inline constexpr std::array<std::uint8_t, 26> Product = {
        26,
        0x03,

        'Q', 0,
        'u', 0,
        'a', 0,
        'r', 0,
        't', 0,
        'z', 0,
        ' ', 0,
        'K', 0,
        '5', 0,
        '5', 0,
        '2', 0,
        'X', 0,
    };

    inline constexpr std::array<std::uint8_t, 58> SerialNumber = {
        58,
        0x03,

        'R', 0,
        'D', 0,
        'K', 0,
        '5', 0,
        '5', 0,
        '2', 0,
        'W', 0,
        '-', 0,
        'R', 0,
        'G', 0,
        'B', 0,
        '-', 0,
        'P', 0,
        'R', 0,
        'O', 0,
        'R', 0,
        'D', 0,
        '2', 0,
        '1', 0,
        '0', 0,
        '7', 0,
        '1', 0,
        '5', 0,
        '0', 0,
        '0', 0,
        '5', 0,
        '5', 0,
        '6', 0,
    };
}