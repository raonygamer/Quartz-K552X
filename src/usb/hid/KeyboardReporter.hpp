#pragma once
#include "usb/Device.hpp"
#include "BootKeyboardReport.hpp"

namespace quartz::usb::hid {
    inline BootKeyboardReport PendingReport {};
    inline bool ReportDirty = false;

    inline void updateReport(const BootKeyboardReport& report) noexcept
    {
        PendingReport = report;
        ReportDirty = true;
    }

    inline void service() noexcept
    {
        if (!ReportDirty)
            return;

        if (Device::isEndpointBusy(1))
            return;

        if (!Device::isConfigured())
            return;

        if (Device::sendKeyboardReport(PendingReport)) {
            ReportDirty = false;
        }
    }
}
