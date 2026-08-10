#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>

extern "C" {
    #include "SN32F240B.h"
}

namespace quartz::debug {
    class Panic {
    public:
        enum class Reason : std::uint16_t {
            NONE                  = 0x0000,
            FAULT_TRIGGERED       = 0x0001,
            ENDPT_OFF_NALIGN      = 0x0002,
            ENDPT_SZ_NALIGN       = 0x0003,
            ENDPT_NIN_CFG         = 0x0004,
            ENDPT_NOUT_CFG        = 0x0005,
            USB_RREG_BUSY         = 0x0006,
            USB_WREG_BUSY         = 0x0007,
            ENDPT_READ_NUL_BUF    = 0x0008,
            ENDPT_WRITE_NUL_BUF   = 0x0009,
            ENDPT_READ_SZ_EXCEED  = 0x000A,
            ENDPT_WRITE_SZ_EXCEED = 0x000B,
            ENDPT_INARM_SZ_EXCEED = 0x000C,
            ENDPT_EP0_OFFSET      = 0x000D,
            ENDPT_INVALID_NUM     = 0x000E,
            ENDPT_SRAM_EXCEED     = 0x000F,
            ENDPT_SZ_EXCEED       = 0x0010,
            ENDPT_INVALID         = 0x0011,
            ENDPT_NON0_DIR_BOTH   = 0x0012,
            TRANS_COUT_BUF_ZLEN   = 0x0013,
            USB_CTRL_NOT_CONF     = 0x0014,
            ENDPT_NOT_IDLE        = 0x0015,
            TRANS_COUT_ACTIVE     = 0x0016,
            TRANS_COUT_NUL_BUF    = 0x0017,
            TRANS_CIN_ACTIVE      = 0x0018,
            TRANS_CIN_NUL_BUF     = 0x0019,
            TRANS_CIN_BUF_ZLEN    = 0x001A,
            TRANS_CIN_INACTIVE    = 0x001B,
            ENDPT_EP0_NOT_CTRL    = 0x001C,
            ENDPT_INVALID_DIREC   = 0x001D,
            CTRL_INVALID_STAGE    = 0x001E,
            CTRL_INVALID_DIREC    = 0x001F,
            CTRL_PLLESS_REM       = 0x0020,
            CTRL_INVALID_RECSZ    = 0x0021,
        };

        struct State {
            static constexpr std::uint64_t Magic = 0x515A504E4E505A51;

            std::uint64_t MagicNumber;
            std::uint32_t ProgramCounter;
            std::uint32_t LinkRegister;
            std::uint32_t StackPointer;
            std::uint32_t PanicCount;
            Reason PanicReason;
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

        static State& getState() noexcept {
            return state;
        }

        static const Controllers& getControllers() noexcept {
            return controllers;
        }

        static void setNextRebootIsBootloader(bool isBootloader) noexcept {
            controllers.MagicNumber = Controllers::Magic;
            controllers.NextRebootIsBootloader = isBootloader;
        }

        [[gnu::always_inline]]
        inline static void assertFailed() noexcept {
            setProgramCounter(reinterpret_cast<std::uint32_t>(__builtin_return_address(0)));
            setLinkRegister(reinterpret_cast<std::uint32_t>(__builtin_return_address(1)));
            setStackPointer(reinterpret_cast<std::uint32_t>(__builtin_frame_address(0)));
            incrementPanicCount();
            triggerHardFault();
        }

        [[gnu::always_inline]]
        inline static void assertFailed(const Reason reason) noexcept {
            state.PanicReason = reason;
            assertFailed();
        }
    };
}

#define HARD_ASSERTC(condition, reason)

#define HARD_ASSERT(condition)

namespace {
    using PanicReason = quartz::debug::Panic::Reason;
}