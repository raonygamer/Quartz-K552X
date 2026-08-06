#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/USB.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"

namespace quartz {
    [[noreturn]]
    static void Start() {
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);

        hal::System::initializeSystemTick();
        usb::Device::initialize();


        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);
        hal::GPIO::setPinHigh(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        for (std::uint32_t i = 0; i < 40; ++i) {
            hal::GPIO::togglePin(hal::GPIOPort::B, hal::GPIOPin::PIN14);
            debug::DebugEndpoint::printf("Toggling GPIO B14: %d\n", static_cast<int>(i));
            hal::Timer::wait(500);
        }


        hal::System::toBootloader();  
    }
} 

extern "C" int main()
{
    quartz::Start();
    __builtin_unreachable();
}