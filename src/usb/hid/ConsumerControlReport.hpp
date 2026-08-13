#pragma once
#include "usb/hid/HIDProtocol.hpp"
#include "usb/hid/KeyboardUsage.hpp"

namespace quartz::usb::hid
{
    struct [[gnu::packed]] ConsumerControlReport
    {
        ReportId Id = ReportId::Consumer;
        ConsumerUsage Usage = ConsumerUsage::None;
    };

    static_assert(sizeof(ConsumerControlReport) == 3);
}