#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/USB.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/Matrix.hpp"
#include "kb/KeyboardState.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz {
    constexpr std::uint32_t ScanIntervalMicroseconds = 1000;
    constexpr std::uint32_t ScanPrintTimeMicroseconds = 2000000;

    inline static std::uint64_t NextScanTime = 0;
    inline static std::uint64_t NextScanTimePrintTime = 0;

    namespace kb {
        class Keyboard {
        public:
            inline static std::uint64_t LastScanTime = 0;
            inline static std::uint32_t ScanRate = 0;

            static void scanAndSend() {
                kb::Matrix::begin();
                kb::Matrix::scan();
                kb::Matrix::end();
                if (kb::KeyboardState::anyKeyChanged()) {
                    usb::hid::markDirty();
                    usb::hid::service();
                }

                const auto now = hal::HighResolutionTimer::nowMicroseconds();
                const auto elapsed = now - LastScanTime;

                if (LastScanTime != 0 && elapsed != 0) {
                    ScanRate = static_cast<std::uint32_t>(
                        1'000'000ULL / elapsed
                    );
                }

                LastScanTime = now;
            }
        };
    }

    [[noreturn]]
    static void Start() {
        kb::Matrix::initialize();
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN14, hal::GPIOMode::Output);
        hal::GPIO::setPinMode(hal::GPIOPort::B, hal::GPIOPin::PIN15, hal::GPIOMode::Output);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN14);
        hal::GPIO::setPinLow(hal::GPIOPort::B, hal::GPIOPin::PIN15);

        for (;;) {
            auto now = hal::HighResolutionTimer::nowMicroseconds();
            if (now >= NextScanTime) {
                NextScanTime += ScanIntervalMicroseconds;
                quartz::kb::Keyboard::scanAndSend();
            }

            if (now >= NextScanTimePrintTime) {
                NextScanTimePrintTime += ScanPrintTimeMicroseconds;
                quartz::debug::DebugEndpoint::printf(
                    "Begin scan time: %u us, Scan time: %u us, End scan time: %u us, Scan rate: %u Hz\n",
                    kb::Matrix::BeginScanTime,
                    kb::Matrix::ScanTime,
                    kb::Matrix::EndScanTime,
                    kb::Keyboard::ScanRate
                );
            }
            
            if (kb::KeyboardState::CurrentKeyStates.test(1) && kb::KeyboardState::CurrentKeyStates.test(2)) {
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