#pragma once
#include <cstdint>
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::debug {
    class Panic {
        struct State {
            static constexpr std::uint32_t Magic = 0x515A504E;

            std::uint32_t MagicNumber;
            std::uint32_t ProgramCounter;
            std::uint32_t LinkRegister;
            std::uint32_t StackPointer;
            std::uint32_t PanicCount;
        };
    private:
        __attribute__((section(".noinit")))
        static inline State state;

    public:
        static void captureState() noexcept;
        static void blinkDebuggingLeds(std::uint16_t delayMs, std::uint8_t count) noexcept;
        static void setDebuggingLedState(bool on) noexcept;
        static void triggerHardFault() noexcept;

        static void incrementPanicCount() noexcept {
            state.PanicCount++;
        }
        
        static const State& getState() noexcept {
            return state;
        }

        static State& getMutableState() noexcept {
            return state;
        }
    };
}