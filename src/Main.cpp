#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/USB.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"

namespace quartz {
    [[noreturn]]
    static void Start() {



        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinHigh(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        for (std::uint32_t i = 0; i < 40; ++i) {
            hal::GPIO::togglePin(hal::GPIOPort::B, hal::GPIOPin::PIN14);
            debug::DebugEndpoint::printf("Toggle B14, tick: %u\n", i);
            debug::DebugEndpoint::printf("B14 state: %d\n", static_cast<int>(hal::GPIO::getPinValue(hal::GPIOPort::B, hal::GPIOPin::PIN14)));
            debug::DebugEndpoint::printf("High resolution timer: %u\n", static_cast<uint32_t>(hal::HighResolutionTimer::nowMilliseconds()));
            debug::DebugEndpoint::printf(
                "TC=%u RIS=%u OVERFLOW=%u\n",
                static_cast<std::uint32_t>(SN_CT16B0->TC),
                static_cast<std::uint32_t>(SN_CT16B0->RIS),
                static_cast<std::uint32_t>(hal::HighResolutionTimer::getOverflowCount())
            );
            hal::HighResolutionTimer::waitMilliseconds(500);
        }

        debug::Panic::triggerHardFault();
    }
} 

constexpr std::uint32_t MaxPanicCount = 3;

extern "C" int main()
{
    quartz::hal::HighResolutionTimer::initialize();
    quartz::hal::System::initializeSystemTick();
    quartz::usb::Device::initialize();
    quartz::usb::Device::waitUntilConfigured();
    
    const auto& state = quartz::debug::Panic::getState();
    if (state.MagicNumber == state.Magic && state.PanicCount >= MaxPanicCount) {
        quartz::debug::Panic::blinkDebuggingLeds(25, 10);
        quartz::debug::Panic::setDebuggingLedState(false);

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

        quartz::hal::System::toBootloader();
        __builtin_unreachable();
    }
    
    quartz::Start();
    __builtin_unreachable();
}