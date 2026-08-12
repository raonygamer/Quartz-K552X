#pragma once
#include "kb/Keyboard.hpp"

#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/Matrix.hpp"
#include "quartz/Profiling.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz::kb
{
    void Keyboard::scanAndSend()
    {
        const auto scanStart = hal::HighResolutionTimer::rawTicks();

        if (LastScanTicks != 0)
        {
            const auto scanPeriod = (scanStart - LastScanTicks) & profiling::RawTickMask;
            if (profiling::AverageScanPeriodTicks == 0)
                profiling::AverageScanPeriodTicks = scanPeriod;
            else
                profiling::AverageScanPeriodTicks = (profiling::AverageScanPeriodTicks * 31u + scanPeriod) / 32u;
        }

        LastScanTicks = scanStart;
        Matrix::scan();
        if (KeyboardState::anyKeyChanged())
        {
            usb::hid::markDirty();
        }

        const auto hidStart = hal::HighResolutionTimer::rawTicks();
        usb::hid::service();
        profiling::HIDTicks = (hal::HighResolutionTimer::rawTicks() - hidStart) & profiling::RawTickMask;
    }
}
