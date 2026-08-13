#pragma once
#include "BootKeyboardReport.hpp"
#include "ConsumerControlReport.hpp"
#include "NKROKeyboardReport.hpp"
#include "usb/Device.hpp"
#include "usb/hid/HIDProtocol.hpp"

namespace quartz::usb::hid
{
    inline BootKeyboardReport PendingBootKeyboard = {};
    inline NKROKeyboardReport PendingReportKeyboard = {};
    inline ConsumerControlReport PendingConsumer = {};

    inline bool KeyboardReportDirty = false;
    inline bool ConsumerReportDirty = false;
    inline auto LastProtocol = HIDProtocol::Boot;

    inline void markReportsDirty() noexcept
    {
        KeyboardReportDirty = true;
        ConsumerReportDirty = true;
    }

    template<typename T>
    std::span<const std::byte> rawReport(const T& report) noexcept
    {
        return { reinterpret_cast<const std::byte*>(&report), sizeof(report) };
    }

    inline void service() noexcept
    {
        if (!Device::isConfigured())
            return;

        const auto protocol = Device::getProtocol();

        if (protocol != LastProtocol)
        {
            LastProtocol = protocol;
            KeyboardReportDirty = true;
            ConsumerReportDirty = protocol == HIDProtocol::Report;
        }

        switch (protocol)
        {
        case HIDProtocol::Boot:
        {
            ConsumerReportDirty = false;

            if (!KeyboardReportDirty)
                return;

            PendingBootKeyboard = kb::KeyboardState::buildBootReport();
            if (Device::sendReport(rawReport(PendingBootKeyboard)))
                KeyboardReportDirty = false;

            break;
        }

        case HIDProtocol::Report:
        {
            if (KeyboardReportDirty)
            {
                PendingReportKeyboard = kb::KeyboardState::buildNKROReport();
                if (!Device::sendReport(rawReport(PendingReportKeyboard)))
                    return;

                KeyboardReportDirty = false;
            }

            if (ConsumerReportDirty)
            {
                PendingConsumer = kb::KeyboardState::buildConsumerReport();
                if (Device::sendReport(rawReport(PendingConsumer)))
                    ConsumerReportDirty = false;
            }

            break;
        }
        }
    }
}