#include "hal/System.hpp"
#include "hal/gpio/GPIO.hpp"
#include "hal/usb/USB.hpp"
#include "hal/timer/Timer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/Device.hpp"
#include "debug/Panic.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "kb/Matrix.hpp"
#include "kb/rgb/RGBMatrix.hpp"
#include "kb/KeyboardState.hpp"
#include "usb/hid/KeyboardReporter.hpp"
#include "kb/Keyboard.hpp"
#include "utils/Time.hpp"

namespace quartz {
    constexpr std::uint32_t RawTickMask = 0x00FFFFFFu;
    constexpr std::uint32_t ScanIntervalTicks = static_cast<std::uint32_t>(utils::Time::microsecondsToTicks(1000));
    constexpr std::uint32_t ScanPrintIntervalTicks = static_cast<std::uint32_t>(utils::Time::millisecondsToTicks(1000));

    constexpr std::uint32_t AnimationIntervalTicks = static_cast<std::uint32_t>(utils::Time::millisecondsToTicks(16));

    namespace
    {
        struct TestBall
        {
            std::uint8_t Row;
            std::uint8_t Column;
            std::uint16_t Phase;
            std::uint16_t Speed;
        };

        constexpr std::size_t TestBallCount = 3;

        TestBall TestBalls[TestBallCount]{};
        std::uint32_t TestRandomState = 0x4B55258Du;

        std::uint32_t testRandom() noexcept
        {
            TestRandomState ^= TestRandomState << 13u;
            TestRandomState ^= TestRandomState >> 17u;
            TestRandomState ^= TestRandomState << 5u;
            return TestRandomState;
        }

        static std::uint32_t RandomState = 0xA341316Cu;

        static inline std::uint32_t random32() noexcept
        {
            std::uint32_t x = RandomState;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            RandomState = x;
            return x;
        }

        void randomizeTestBall(TestBall& ball) noexcept
        {
            ball.Row = static_cast<std::uint8_t>(testRandom() % kb::rgb::RGBMatrix::Rows);
            ball.Column = static_cast<std::uint8_t>(testRandom() % kb::rgb::RGBMatrix::Columns);
            ball.Speed = static_cast<std::uint16_t>(3u + (testRandom() % 4u));
        }

        void initializeTestAnimation() noexcept
        {
            TestRandomState ^= hal::HighResolutionTimer::rawTicks();

            for (std::size_t i = 0; i < TestBallCount; ++i) {
                randomizeTestBall(TestBalls[i]);
                TestBalls[i].Phase = static_cast<std::uint16_t>((i * 512u) / TestBallCount);
            }
        }

        std::uint32_t approximateDistanceQ8(const std::int32_t dx, const std::int32_t dy) noexcept
        {
            const std::uint32_t ax = static_cast<std::uint32_t>(dx < 0 ? -dx : dx);
            const std::uint32_t ay = static_cast<std::uint32_t>(dy < 0 ? -dy : dy);

            const std::uint32_t maximum = ax > ay ? ax : ay;
            const std::uint32_t minimum = ax > ay ? ay : ax;

            // Cheap rounded-distance approximation:
            // max + min / 2
            return maximum + (minimum >> 1u);
        }

        void updateTestAnimation() noexcept
        {
            for (auto& ball : TestBalls) {
                const std::uint16_t previousPhase = ball.Phase;
                ball.Phase = static_cast<std::uint16_t>((ball.Phase + ball.Speed) & 0x1FFu);

                // Reposition only once the ball has completely shrunk away.
                if (ball.Phase < previousPhase)
                    randomizeTestBall(ball);
            }

            for (std::uint8_t row = 0; row < kb::rgb::RGBMatrix::Rows; ++row) {
                for (std::uint8_t column = 0; column < kb::rgb::RGBMatrix::Columns; ++column) {
                    std::uint32_t brightness = 0;

                    for (const auto& ball : TestBalls) {
                        // 0 -> 255 -> 0 triangular envelope.
                        const std::uint32_t phase = ball.Phase;
                        const std::uint32_t growth = phase < 256u ? phase : 511u - phase;

                        // Q8 radius:
                        // 0..765 ~= 0..3 keyboard-key spacings.
                        const std::uint32_t radius = growth * 3u;

                        if (radius == 0u)
                            continue;

                        const std::int32_t dx = (static_cast<std::int32_t>(column) - static_cast<std::int32_t>(ball.Column)) << 8;
                        const std::int32_t dy = (static_cast<std::int32_t>(row) - static_cast<std::int32_t>(ball.Row)) << 8;

                        const std::uint32_t distance = approximateDistanceQ8(dx, dy);

                        if (distance >= radius)
                            continue;

                        // One-key-wide gradient at the edge.
                        // Q8 means 256 == one whole key spacing, conveniently
                        // mapping directly to the 0..255 brightness range.
                        const std::uint32_t depth = radius - distance;
                        const std::uint32_t contribution = depth >= 255u ? 255u : depth;

                        brightness += contribution;

                        if (brightness >= 255u) {
                            brightness = 255u;
                            break;
                        }
                    }

                    const auto value = static_cast<std::uint8_t>(brightness);
                    kb::rgb::RGBMatrix::setPixel(row, column, 0, value, 0);
                }
            }
        }
    }

    namespace kb {
        void Keyboard::scanAndSend()
        {
            const auto scanStart = hal::HighResolutionTimer::rawTicks();

            const auto elapsed = LastScanTicks != 0
                ? (scanStart - LastScanTicks) & RawTickMask
                : ScanIntervalTicks;

            LastScanTicks = scanStart;

            kb::rgb::RGBMatrix::pause();
            kb::Matrix::scan();
            kb::rgb::RGBMatrix::resume();

            const auto hidStart = hal::HighResolutionTimer::rawTicks();

            if (kb::KeyboardState::anyKeyChanged()) {
                usb::hid::markDirty();
                usb::hid::service();
            }

            HIDTicks = static_cast<std::uint32_t>((hal::HighResolutionTimer::rawTicks() - hidStart) & RawTickMask);

            const auto scanEnd = hal::HighResolutionTimer::rawTicks();
            const auto scanDuration = (scanEnd - scanStart) & RawTickMask;

            if (elapsed != 0) {
                ScanDurationTicks = scanDuration;
                ScanPeriodTicks = elapsed;
                ScanRate = static_cast<std::uint32_t>(SystemCoreClock / elapsed);
                CPUScanUsage = static_cast<std::uint32_t>((scanDuration * 10'000ULL) / elapsed);
            }
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
        std::uint32_t nextPrintTicks = ScanPrintIntervalTicks;
        std::uint32_t nextAnimationTicks = AnimationIntervalTicks;


        //kb::rgb::RGBMatrix::fill(255, 0, 0);
        initializeTestAnimation();
        updateTestAnimation();
        kb::rgb::RGBMatrix::resume();
        for (;;) {
            const std::uint32_t rawTicks = hal::HighResolutionTimer::rawTicks();
            softwareTicks += (rawTicks - previousRawTicks) & RawTickMask;
            previousRawTicks = rawTicks;

            if (static_cast<std::int32_t>(softwareTicks - nextScanTicks) >= 0) {
                nextScanTicks += ScanIntervalTicks;
                kb::Keyboard::scanAndSend();
            }

            if (static_cast<std::int32_t>(softwareTicks - nextAnimationTicks) >= 0) {
                nextAnimationTicks += AnimationIntervalTicks;
                updateTestAnimation();
            }

            if (static_cast<std::int32_t>(softwareTicks - nextPrintTicks) >= 0) {
                nextPrintTicks += ScanPrintIntervalTicks;

                debug::DebugEndpoint::printf(
                    "Begin: %u ticks, Scan: %u ticks, End: %u ticks, State: %u ticks, HID: %u ticks, Total: %u ticks, Period: %u ticks, CPU: %u.%u%%, Rate: %u Hz\n",
                    kb::Matrix::BeginScanTicks,
                    kb::Matrix::ScanTicks,
                    kb::Matrix::EndScanTicks,
                    kb::Keyboard::StateUpdateTicks,
                    kb::Keyboard::HIDTicks,
                    kb::Keyboard::ScanDurationTicks,
                    kb::Keyboard::ScanPeriodTicks,
                    kb::Keyboard::CPUScanUsage / 100,
                    kb::Keyboard::CPUScanUsage % 100,
                    kb::Keyboard::ScanRate
                );
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