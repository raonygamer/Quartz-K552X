#pragma once
#include <cstdint>
#include "utils/BitSet.hpp"
#include "hal/gpio/GPIO.hpp"

namespace quartz::kb {
    struct GPIOPinSet {
        hal::GPIOPort Port;
        hal::GPIOPin Pin;

        std::uint32_t getMask() const noexcept
        {
            return (1u << static_cast<std::uint32_t>(Pin));
        }
    };

    class Matrix {
    public:
        constexpr static std::uint8_t Rows = 7;
        constexpr static std::uint8_t Cols = 16;
        constexpr static std::uint8_t Size = Rows * Cols;
        using ColumnBitSet = std::array<std::uint8_t, (Cols / 8)>;

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

        static std::uint32_t BeginScanTime;
        static std::uint32_t ScanTime;
        static std::uint32_t EndScanTime;

        static std::size_t getKeyIndex(const uint8_t row, const uint8_t col) noexcept
        {
            return static_cast<std::size_t>(row) * Cols + col;
        }

        static utils::MatrixPosition getKeyPosition(const std::size_t index) noexcept
        {
            return utils::MatrixPosition {
                static_cast<std::uint8_t>(index / Cols),
                static_cast<std::uint8_t>(index % Cols)
            };
        }

        static void initialize() noexcept;
        static void setRowPinsMode(const hal::GPIOMode mode) noexcept;
        static void setRowPinValue(const std::uint8_t row, const bool high) noexcept;
        static void setAllRowPinsValue(const bool high) noexcept;
        static void setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept;
        static void readColPins(ColumnBitSet& states) noexcept;
        static void begin() noexcept;
        static void scan() noexcept;
        static void end() noexcept;
    };
}