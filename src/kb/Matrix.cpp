#include "kb/Matrix.hpp"
#include "kb/KeyboardState.hpp"
#include "hal/timer/HighResolutionTimer.hpp"
#include "debug/DebugEndpoint.hpp"
#include "usb/hid/BootKeyboardReport.hpp"
#include "utils/MatrixPosition.hpp"
#include "usb/hid/KeyboardReporter.hpp"
#include "Matrix.hpp"

namespace quartz::kb {
    std::uint32_t Matrix::BeginScanTime = 0;
    std::uint32_t Matrix::ScanTime = 0;
    std::uint32_t Matrix::EndScanTime = 0;

    void Matrix::initialize() noexcept {
        BeginScanTime = 0;
        ScanTime = 0;
        EndScanTime = 0;
    }

    void Matrix::setRowPinsMode(const hal::GPIOMode mode) noexcept
    {
        hal::GPIO::setPortMode(hal::GPIOPort::C, 0b1010000000000000, mode); // C13, C15
        hal::GPIO::setPortMode(hal::GPIOPort::D, 0b0000111111100000, mode); // D7-D11
    }

    void Matrix::setRowPinValue(const std::uint8_t row, const bool high) noexcept
    {
        if (row >= Rows) {
            return;
        }

        const GPIOPinSet& pinSet = RowPins[row];
        hal::GPIO::setPinValue(pinSet.Port, pinSet.Pin, high);
    }

    void Matrix::setAllRowPinsValue(const bool high) noexcept
    {
        // C - 0b1010000000000000
        // D - 0b0000111111100000
        hal::GPIO::setPortValue(hal::GPIOPort::C, 0b1010000000000000, high);
        hal::GPIO::setPortValue(hal::GPIOPort::D, 0b0000111111100000, high);
    }

    void Matrix::setColPinsMode(const hal::GPIOMode mode, const hal::GPIOPull pull) noexcept
    {
        // constexpr static GPIOPinSet ColPins[Cols] = {
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN0  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN1  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN3  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN4  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN5  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN6  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN7  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN8  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN9  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN10 },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN11 },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN12 },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN6  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN7  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN8  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN9  }
        // };

        hal::GPIO::setPortMode(hal::GPIOPort::C, 0b0001111111111011, mode); // C0, C1, C3-C12
        hal::GPIO::setPortMode(hal::GPIOPort::B, 0b0000001111000000, mode); // B6-B9
        for (std::uint8_t col = 0; col < Cols; ++col) {
            const GPIOPinSet& pinSet = ColPins[col];
            hal::GPIO::setPinPull(pinSet.Port, pinSet.Pin, pull);
        }
    }

    void Matrix::readColPins(ColumnBitSet& states) noexcept
    {
        // constexpr static GPIOPinSet ColPins[Cols] = {
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN0  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN1  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN3  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN4  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN5  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN6  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN7  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN8  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN9  },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN10 },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN11 },
        //     { hal::GPIOPort::C, hal::GPIOPin::PIN12 },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN6  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN7  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN8  },
        //     { hal::GPIOPort::B, hal::GPIOPin::PIN9  }
        // };

        uint32_t maskC = 0b0001111111111011; // C0, C1, C3-C12
        uint32_t maskB = 0b0000001111000000; // B6-B9
        uint32_t portC = hal::GPIO::getPortValue(hal::GPIOPort::C) & maskC;
        uint32_t portB = hal::GPIO::getPortValue(hal::GPIOPort::B) & maskB;

        for (std::uint8_t col = 0; col < Cols; ++col) {
            const GPIOPinSet& pinSet = ColPins[col];
            bool isHigh = false;
            if (pinSet.Port == hal::GPIOPort::C) {
                isHigh = (portC & pinSet.getMask()) != 0;
            } else if (pinSet.Port == hal::GPIOPort::B) {
                isHigh = (portB & pinSet.getMask()) != 0;
            }
            if (!isHigh) {
                states[col / 8] |= (1 << (col % 8));
            }
        }
    }

    void Matrix::begin() noexcept
    {
        auto start = hal::HighResolutionTimer::nowMicroseconds();
        setRowPinsMode(hal::GPIOMode::Output);
        setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::PullUp);
        setAllRowPinsValue(true);
        BeginScanTime = static_cast<std::uint32_t>(hal::HighResolutionTimer::nowMicroseconds() - start);
    }

    void Matrix::scan() noexcept
    {
        auto start = hal::HighResolutionTimer::nowMicroseconds();
        utils::BitSet<Size> newKeyStates;
        for (std::uint8_t row = 0; row < Rows; ++row) {
            setRowPinValue(row, false);
            hal::HighResolutionTimer::waitMicroseconds(35); // Allow signals to stabilize
            ColumnBitSet colStates {};
            readColPins(colStates);
            newKeyStates.setUnsafe<2U>(colStates.data(), row * 2U);
            setRowPinValue(row, true);
        }

        ScanTime = static_cast<std::uint32_t>(hal::HighResolutionTimer::nowMicroseconds() - start);
        kb::KeyboardState::updateKeyStates(newKeyStates);
    }

    void Matrix::end() noexcept
    {
        auto start = hal::HighResolutionTimer::nowMicroseconds();
        setColPinsMode(hal::GPIOMode::Input, hal::GPIOPull::None);
        setRowPinsMode(hal::GPIOMode::Input);
        EndScanTime = static_cast<std::uint32_t>(hal::HighResolutionTimer::nowMicroseconds() - start);
    }
}