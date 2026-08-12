#include "debug/Panic.hpp"
#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "hal/usb/Controller.hpp"
#include "kb/ElectricalMatrix.hpp"
#include "kb/Keyboard.hpp"
#include "kb/KeyboardState.hpp"
#include "kb/MatrixTimingProbe.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "quartz/Profiling.hpp"
#include "quartz/rpc/RPC.hpp"
#include "usb/Device.hpp"
#include "usb/hid/KeyboardReporter.hpp"
#include "utils/Time.hpp"

namespace quartz
{
    constexpr std::uint32_t ScanIntervalTicks = utils::Time::microsecondsToTicks(2000);
    static bool matrixProbeChordLatched = false;

    static void sendMatrixTimingProbeResult(const kb::SizedMatrixTimingProbeResult& result) noexcept
    {
        rpc::RPC::send(
            rpc::PacketType::MatrixTimingProbeResult,
            0,
            std::as_bytes(std::span{ &result, 1 })
        );
    }

    [[noreturn]]
    static void Start()
    {
        NVIC_SetPriority(CT16B1_IRQn, 0);
        NVIC_SetPriority(USB_IRQn, 2);
        NVIC_SetPriority(CT16B0_IRQn, 3);
        kb::rgb::RGBMatrix::initialize();

        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        std::uint32_t previousRawTicks = hal::HighResolutionTimer::rawTicks();
        std::uint32_t softwareTicks = 0;
        std::uint32_t nextScanTicks = ScanIntervalTicks;

        kb::rgb::RGBMatrix::fill(255, 255, 0);
        kb::rgb::RGBMatrix::swapBuffers();
        kb::rgb::RGBMatrix::resume();
        for (;;)
        {
            const std::uint32_t rawTicks = hal::HighResolutionTimer::rawTicks();
            softwareTicks += (rawTicks - previousRawTicks) & profiling::RawTickMask;
            previousRawTicks = rawTicks;

            if (!kb::ElectricalMatrix::KeyScanPending && static_cast<std::int32_t>(softwareTicks - nextScanTicks) >= 0)
            {
                kb::ElectricalMatrix::KeyScanPending = true;
                nextScanTicks += ScanIntervalTicks;
                kb::rgb::RGBMatrix::handOver();
            }

            if (kb::ElectricalMatrix::KeyScanPending && kb::rgb::RGBMatrix::handedOver())
            {
                kb::ElectricalMatrix::KeyScanPending = false;
                kb::Keyboard::scanAndSend();
                const bool matrixProbeChord =
                    kb::KeyboardState::isFunctionPressed() &&
                    kb::KeyboardState::isKeyDown(kb::Key::LeftControl) &&
                    kb::KeyboardState::isKeyDown(kb::Key::LeftShift) &&
                    kb::KeyboardState::isKeyDown(kb::Key::F12);

                if (matrixProbeChord && !matrixProbeChordLatched)
                {
                    matrixProbeChordLatched = true;

                    // Matrix::scan() has already returned the bus to RGB,
                    // so request it back for the diagnostic.
                    kb::rgb::RGBMatrix::handOver();
                    while (!kb::rgb::RGBMatrix::handedOver())
                        __NOP();

                    const auto result = kb::MatrixTimingProbe::run();
                    kb::rgb::RGBMatrix::acquire();

                    sendMatrixTimingProbeResult(result);

                    // We just blocked the foreground scheduler for several seconds.
                    // Don't try to catch up thousands of missed deadlines.
                    previousRawTicks = hal::HighResolutionTimer::rawTicks();
                    softwareTicks = 0;
                    nextScanTicks = ScanIntervalTicks;
                    continue;
                }

                matrixProbeChordLatched = matrixProbeChord;
            }

            if (kb::KeyboardState::isFunctionPressed() &&
                kb::KeyboardState::isKeyDown(kb::Key::LeftControl) &&
                kb::KeyboardState::isKeyDown(kb::Key::LeftShift) &&
                kb::KeyboardState::isKeyDown(kb::Key::Escape))
            {
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

    if (quartz::debug::Panic::isNextRebootBootloader())
    {
        quartz::debug::Panic::setNextRebootIsBootloader(false);
        quartz::hal::System::toBootloader(false);
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
