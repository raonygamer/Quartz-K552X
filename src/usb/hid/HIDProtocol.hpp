#pragma once
#include <cstdint>

namespace quartz::usb::hid
{
    struct HIDRequest
    {
        static constexpr std::uint8_t GET_REPORT = 0x01;
        static constexpr std::uint8_t GET_IDLE = 0x02;
        static constexpr std::uint8_t GET_PROTOCOL = 0x03;
        static constexpr std::uint8_t SET_REPORT = 0x09;
        static constexpr std::uint8_t SET_IDLE = 0x0A;
        static constexpr std::uint8_t SET_PROTOCOL = 0x0B;
    };

    enum class HIDReportType : std::uint8_t
    {
        Input = 0x01,
        Output = 0x02,
        Feature = 0x03
    };

    enum class HIDProtocol : std::uint8_t
    {
        Boot = 0,
        Report = 1
    };

    enum class ControlOutType : std::uint8_t
    {
        None,
        HIDOutputReport,
        HIDFeatureReport
    };
}