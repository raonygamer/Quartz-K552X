#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace quartz::usb::descriptors {
    inline constexpr std::array<std::uint8_t, 18> Device = {
        18,         // bLength
        0x01,       // bDescriptorType: Device

        0x00, 0x02, // bcdUSB: USB 2.00

        0xFF,       // bDeviceClass: vendor-specific for bring-up
        0x00,       // bDeviceSubClass
        0x00,       // bDeviceProtocol

        64,         // bMaxPacketSize0

        0x47, 0xb1, // idVendor: 0xb157
        0x31, 0x41, // idProduct: 0x4131

        0x00, 0x01, // bcdDevice: 1.00

        1,          // iManufacturer
        2,          // iProduct
        3,          // iSerialNumber

        1,          // bNumConfigurations
    };

    inline constexpr std::array<std::uint8_t, 25> Configuration = {
        // Configuration descriptor
        9,
        0x02,
        25, 0,      // wTotalLength
        1,          // bNumInterfaces
        1,          // bConfigurationValue
        0,          // iConfiguration
        0x80,       // bus-powered
        50,         // 100 mA

        // Vendor interface
        9,
        0x04,
        0,
        0,
        1,          // bNumEndpoints
        0xFF,
        0,
        0,
        0,

        // EP4 IN
        7,
        0x05,
        0x84,       // IN | endpoint 4
        0x02,       // Bulk
        32, 0,      // wMaxPacketSize
        0,
    };

    inline constexpr std::array<std::uint8_t, 4> Language = {
        4,
        0x03,       // String descriptor
        0x09, 0x04, // English (United States), 0x0409
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