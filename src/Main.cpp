#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/timer/Timer.hpp"

namespace quartz {
    [[noreturn]]
    static void Start() {
        hal::System::initializeSystemTick();
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinHigh(hal::GPIOPort::B, hal::GPIOPin::PIN15);
        for (std::uint32_t i = 0; i < 20; ++i) {
            hal::GPIO::togglePin(hal::GPIOPort::B, hal::GPIOPin::PIN14);
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