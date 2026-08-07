#pragma once
#include <cstdint>
#include "utils/BitSet.hpp"
#include "hal/gpio/GPIO.hpp"

namespace quartz::kb {
    struct GPIOPinSet {
        hal::GPIOPort Port;
        hal::GPIOPin Pin;
    };

    class Matrix {
    public:
        constexpr static std::uint8_t Rows = 7;
        constexpr static std::uint8_t Cols = 16;
        constexpr static std::uint8_t Size = Rows * Cols;
        using ColumnBitSet = utils::BitSet<Cols>;

        constexpr static GPIOPinSet RowPins[Rows] = {
            { hal::GPIOPort::C, hal::GPIOPin::PIN13 },
            { hal::GPIOPort::C, hal::GPIOPin::PIN15 },
            { hal::GPIOPort::D, hal::GPIOPin::PIN7  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN8  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN9  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN10 },
            { hal::GPIOPort::D, hal::GPIOPin::PIN11 }
        };

        constexpr static GPIOPinSet ColPins[Cols] = {
            { hal::GPIOPort::C, hal::GPIOPin::PIN0  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN1  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN3  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN4  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN5  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN6  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN7  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN8  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN9  },
            { hal::GPIOPort::C, hal::GPIOPin::PIN10 },
            { hal::GPIOPort::C, hal::GPIOPin::PIN11 },
            { hal::GPIOPort::C, hal::GPIOPin::PIN12 },
            { hal::GPIOPort::B, hal::GPIOPin::PIN6  },
            { hal::GPIOPort::B, hal::GPIOPin::PIN7  },
            { hal::GPIOPort::B, hal::GPIOPin::PIN8  },
            { hal::GPIOPort::B, hal::GPIOPin::PIN9  }
        };

        static utils::BitSet<Size> LastRawKeyStates;
        static utils::BitSet<Size> RawKeyStates;
        static utils::BitSet<Size> LastStableKeyStates;
        static utils::BitSet<Size> StableKeyStates;
        static std::array<std::uint8_t, Size> DebounceCounters;
        constexpr static std::uint8_t DebounceThreshold = 5;

        static std::size_t getKeyIndex(const uint8_t row, const uint8_t col) noexcept
        {
            return static_cast<std::size_t>(col) * Rows + row;
        }

        static void initialize() noexcept;
        static void setRowPinsMode(const hal::GPIOMode mode) noexcept;
        static void setRowPinValue(const std::uint8_t row, const bool high) noexcept;
        static void setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept;
        static ColumnBitSet readColPins() noexcept;
        static void scan() noexcept;
        static void debounce() noexcept;
    };
}