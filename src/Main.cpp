#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/USB.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/Matrix.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz {
    [[noreturn]]
    static void Start() {
        kb::Matrix::initialize();
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        for (;;) {
            if (kb::Matrix::StableKeyStates.test(1) && kb::Matrix::StableKeyStates.test(2)) {
                break;
            }
            usb::hid::service();
            __WFI();
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

    const auto& state = quartz::debug::Panic::getState();

    if (state.MagicNumber == state.Magic &&
        state.PanicCount >= MaxPanicCount)
    {
        quartz::debug::Panic::blinkDebuggingLeds(25, 10);
        quartz::debug::Panic::setDebuggingLedState(false);

        quartz::usb::Device::initialize();

        if (quartz::usb::Device::waitUntilConfigured(1000)) {
            quartz::debug::DebugEndpoint::printf(
                "Panic count exceeded maximum (%u), entering bootloader\n",
                MaxPanicCount
            );

            quartz::debug::DebugEndpoint::printf(
                "State: PC=0x%x LR=0x%x SP=0x%x\n",
                state.ProgramCounter,
                state.LinkRegister,
                state.StackPointer
            );
        }

        quartz::hal::System::toBootloader();
    }

    quartz::usb::Device::initialize();
    quartz::usb::Device::waitUntilConfigured();

    quartz::Start();
}