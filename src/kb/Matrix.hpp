#pragma once
#include <cstdint>
#include "utils/BitSet.hpp"
#include "hal/gpio/GPIO.hpp"
#include "MatrixDefinitions.hpp"

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
        using ColumnBitSet = std::array<std::uint8_t, (MatrixDefinitions::Cols / 8)>;

        constexpr static GPIOPinSet RowPins[MatrixDefinitions::Rows] = {
            { hal::GPIOPort::C, hal::GPIOPin::PIN13 },
            { hal::GPIOPort::C, hal::GPIOPin::PIN15 },
            { hal::GPIOPort::D, hal::GPIOPin::PIN7  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN8  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN9  },
            { hal::GPIOPort::D, hal::GPIOPin::PIN10 },
            { hal::GPIOPort::D, hal::GPIOPin::PIN11 }
        };

        constexpr static GPIOPinSet ColPins[MatrixDefinitions::Cols] = {
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

        static std::uint32_t BeginScanTicks;
        static std::uint32_t ScanTicks;
        static std::uint32_t EndScanTicks;
        static std::uint32_t RowWaitingTicks;

        static std::size_t getKeyIndex(const uint8_t row, const uint8_t col) noexcept
        {
            return static_cast<std::size_t>(row) * MatrixDefinitions::Cols + col;
        }

        static utils::MatrixPosition getKeyPosition(const std::size_t index) noexcept
        {
            return utils::MatrixPosition {
                static_cast<std::uint8_t>(index / MatrixDefinitions::Cols),
                static_cast<std::uint8_t>(index % MatrixDefinitions::Cols)
            };
        }

        static void initialize() noexcept;
        static void setRowPinsMode(const hal::GPIOMode mode) noexcept;
        static void setRowPinValue(const std::uint8_t row, const bool high) noexcept;
        static void setAllRowPinsValue(const bool high) noexcept;
        static void setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept;
        [[gnu::always_inline]]
        inline static std::uint16_t readColPins() noexcept;
        static void begin() noexcept;
        static void scan() noexcept;
        static void end() noexcept;
    };
}