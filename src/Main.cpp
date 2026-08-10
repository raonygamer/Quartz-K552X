#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/Controller.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/Matrix.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "kb/ElectricalMatrix.hpp"
#include "kb/KeyboardState.hpp"
#include "usb/hid/KeyboardReporter.hpp"
#include "kb/Keyboard.hpp"
#include "utils/Time.hpp"
#include "quartz/Profiling.hpp"

namespace quartz {
    constexpr std::uint32_t RawTickMask = 0x00FFFFFFu;
    constexpr std::uint32_t ScanIntervalTicks = utils::Time::microsecondsToTicks(1000);
    constexpr std::uint32_t ScanPrintIntervalTicks = utils::Time::millisecondsToTicks(1000);

    namespace kb {
        void Keyboard::scanAndSend()
        {
            const auto scanStart = hal::HighResolutionTimer::rawTicks();

            if (LastScanTicks != 0) {
                const auto scanPeriod = (scanStart - LastScanTicks) & RawTickMask;

                if (profiling::AverageScanPeriodTicks == 0)
                    profiling::AverageScanPeriodTicks = scanPeriod;
                else
                    profiling::AverageScanPeriodTicks = (profiling::AverageScanPeriodTicks * 31u + scanPeriod) / 32u;
            }

            LastScanTicks = scanStart;

            kb::Matrix::scan();

            const auto hidStart = hal::HighResolutionTimer::rawTicks();

            if (KeyboardState::anyKeyChanged()) {
                usb::hid::markDirty();
            }
            usb::hid::service();
            profiling::HIDTicks = (hal::HighResolutionTimer::rawTicks() - hidStart) & RawTickMask;
        }
    }

    [[noreturn]]
    static void Start()
    {
        kb::Matrix::initialize();
        kb::rgb::RGBMatrix::initialize();

        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        std::uint32_t previousRawTicks = hal::HighResolutionTimer::rawTicks();
        std::uint32_t softwareTicks = 0;
        std::uint32_t nextScanTicks = ScanIntervalTicks;

        kb::rgb::RGBMatrix::fill(0, 50, 50);
        kb::rgb::RGBMatrix::resume();
        for (;;) {
            const std::uint32_t rawTicks = hal::HighResolutionTimer::rawTicks();
            softwareTicks += (rawTicks - previousRawTicks) & RawTickMask;
            previousRawTicks = rawTicks;
            
            if (!kb::ElectricalMatrix::KeyScanPending && static_cast<std::int32_t>(softwareTicks - nextScanTicks) >= 0) {
                kb::ElectricalMatrix::KeyScanPending = true;
                nextScanTicks += ScanIntervalTicks;
                kb::rgb::RGBMatrix::handOver();
            }

            if (kb::ElectricalMatrix::KeyScanPending && kb::rgb::RGBMatrix::handedOver()) {
                kb::ElectricalMatrix::KeyScanPending = false;
                kb::Keyboard::scanAndSend();
            }

            if (kb::KeyboardState::isFunctionPressed() &&
                kb::KeyboardState::isKeyDown(kb::Key::LeftControl) &&
                kb::KeyboardState::isKeyDown(kb::Key::LeftShift) &&
                kb::KeyboardState::isKeyDown(kb::Key::Escape)) {
                break;
            }
        }

        hal::System::toBootloader();
        __builtin_unreachable();
    }
}

constexpr std::uint32_t MaxPanicCount = 3;
extern "C" int main()
{
    quartz::hal::HighResolutionTimer::initialize();
    quartz::hal::System::initializeSystemTick();

    if (quartz::debug::Panic::isNextRebootBootloader()) {
        quartz::debug::Panic::setNextRebootIsBootloader(false);
        quartz::hal::System::toBootloader();
        // Unreachable
    }

    auto& state = quartz::debug::Panic::getState();

    if (state.MagicNumber == state.Magic &&
        state.PanicCount >= MaxPanicCount)
    {
        state.PanicCount = 0;
        quartz::debug::Panic::blinkDebuggingLeds(25, 10);
        quartz::debug::Panic::setNextRebootIsBootloader(false);
        quartz::debug::Panic::setDebuggingLedState(false);
        quartz::hal::System::toBootloader();
    }

    quartz::usb::Device::reset();
    quartz::hal::usb::Controller::initialize();
    quartz::hal::usb::Controller::connect();
    quartz::usb::Device::waitUntilConfigured();
    quartz::Start();
}