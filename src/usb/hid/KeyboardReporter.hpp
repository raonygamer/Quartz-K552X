#pragma once
#include "BootKeyboardReport.hpp"
#include "usb/Device.hpp"
#include "usb/hid/HIDProtocol.hpp"

namespace quartz::usb::hid
{
    inline BootKeyboardReport PendingBootKeyboard{};
    inline NKROKeyboardReport PendingReportKeyboard{};
    inline bool ReportDirty = false;
    inline auto LastProtocol = HIDProtocol::Boot;

    inline void markDirty() noexcept
    {
        ReportDirty = true;
    }

    inline std::span<const std::byte> rawCurrentKeyboardReport()
    {
        switch (Device::getProtocol())
        {
        case HIDProtocol::Boot:
            return std::span(reinterpret_cast<const std::byte*>(&PendingBootKeyboard), sizeof(PendingBootKeyboard));
        case HIDProtocol::Report:
            return std::span(reinterpret_cast<const std::byte*>(&PendingReportKeyboard), sizeof(PendingReportKeyboard));
        }
        return std::span(reinterpret_cast<const std::byte*>(&PendingBootKeyboard), sizeof(PendingBootKeyboard));
    }

    inline void service() noexcept
    {
        if (!Device::isConfigured())
            return;

        const auto protocol = Device::getProtocol();

        if (protocol != LastProtocol)
        {
            LastProtocol = protocol;
            ReportDirty = true;
        }

        if (!ReportDirty)
            return;
        switch (protocol)
        {
        case HIDProtocol::Boot:
            PendingBootKeyboard = kb::KeyboardState::buildBootReport();
            break;
        case HIDProtocol::Report:
            PendingReportKeyboard = kb::KeyboardState::buildNKROReport();
            break;
        }

        if (Device::sendKeyboard(rawCurrentKeyboardReport()))
            ReportDirty = false;
    }
}
