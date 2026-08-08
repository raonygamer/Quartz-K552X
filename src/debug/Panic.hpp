#pragma once
#include <cstdint>
extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::debug {
    class Panic {
        struct State {
            static constexpr std::uint64_t Magic = 0x515A504E4E505A51;

            std::uint64_t MagicNumber;
            std::uint32_t ProgramCounter;
            std::uint32_t LinkRegister;
            std::uint32_t StackPointer;
            std::uint32_t PanicCount;
        };

        struct Controllers {
            static constexpr std::uint64_t Magic = 0x434F4E54544E4F43;

            std::uint64_t MagicNumber;
            bool NextRebootIsBootloader;
        };
    private:
        __attribute__((section(".noinit")))
        static inline State state;
        __attribute__((section(".noinit")))
        static inline Controllers controllers;

    public:
        static void markNextRebootIsBootloader() noexcept;
        static void captureState() noexcept;
        static void blinkDebuggingLeds(std::uint16_t delayMs, std::uint8_t count) noexcept;
        static void setDebuggingLedState(bool on) noexcept;
        static void triggerHardFault() noexcept;

        static bool isNextRebootBootloader() noexcept {
            if (controllers.MagicNumber != Controllers::Magic) {
                return false;
            }
            return controllers.NextRebootIsBootloader;
        }

        static void incrementPanicCount() noexcept {
            if (state.MagicNumber != State::Magic) {
                state.MagicNumber = State::Magic;
                state.PanicCount = 0;
            }
            state.PanicCount++;
        }

        static std::uint32_t getPanicCount() noexcept {
            if (state.MagicNumber != State::Magic) {
                return 0;
            }
            return state.PanicCount;
        }

        static std::uint32_t getProgramCounter() noexcept {
            if (state.MagicNumber != State::Magic) {
                return 0;
            }
            return state.ProgramCounter;
        }

        static std::uint32_t getLinkRegister() noexcept {
            if (state.MagicNumber != State::Magic) {
                return 0;
            }
            return state.LinkRegister;
        }

        static std::uint32_t getStackPointer() noexcept {
            if (state.MagicNumber != State::Magic) {
                return 0;
            }
            return state.StackPointer;
        }

        static void setProgramCounter(std::uint32_t pc) noexcept {
            state.MagicNumber = State::Magic;
            state.ProgramCounter = pc;
        }

        static void setLinkRegister(std::uint32_t lr) noexcept {
            state.MagicNumber = State::Magic;
            state.LinkRegister = lr;
        }

        static void setStackPointer(std::uint32_t sp) noexcept {
            state.MagicNumber = State::Magic;
            state.StackPointer = sp;
        }

        static void setPanicCount(std::uint32_t count) noexcept {
            state.MagicNumber = State::Magic;
            state.PanicCount = count;
        }

        static const State& getState() noexcept {
            return state;
        }

        static const Controllers& getControllers() noexcept {
            return controllers;
        }

        static void setNextRebootIsBootloader(bool isBootloader) noexcept {
            controllers.MagicNumber = Controllers::Magic;
            controllers.NextRebootIsBootloader = isBootloader;
        }
    };
}