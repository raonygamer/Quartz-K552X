#pragma once
#include "usb/Device.hpp"
#include "BootKeyboardReport.hpp"
#include "usb/hid/HIDProtocol.hpp"

namespace quartz::usb::hid {
    inline BootKeyboardReport PendingBootKeyboard {};
    inline NKROKeyboardReport PendingReportKeyboard {};
    inline bool ReportDirty = false;
    inline auto LastProtocol = Protocol::Boot;

    inline void markDirty() noexcept
    {
        ReportDirty = true;
    }

    inline void service() noexcept
    {
        // if (!Device::isConfigured())
        //     return;
        //
        // const auto protocol = Device::getHIDProtocol();
        //
        // if (protocol != LastProtocol) {
        //     LastProtocol = protocol;
        //     ReportDirty = true;
        // }
        //
        // if (!ReportDirty)
        //     return;
        //
        // if (Device::isEndpointBusy(descriptors::HIDKeyboard::EndpointNumber))
        //     return;
        //
        // bool sent = false;
        //
        // switch (protocol) {
        //     case Protocol::Boot:
        //         PendingBootKeyboard = kb::KeyboardState::buildBootReport();
        //         sent = Device::sendBootKeyboard(PendingBootKeyboard);
        //         break;
        //
        //     case Protocol::Report:
        //         PendingReportKeyboard = kb::KeyboardState::buildNKROReport();
        //         sent = Device::sendReportKeyboard(PendingReportKeyboard);
        //         break;
        // }
        //
        // if (sent)
        //     ReportDirty = false;
    }
}
