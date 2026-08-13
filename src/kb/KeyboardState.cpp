#include "KeyboardState.hpp"
#include "ElectricalMatrix.hpp"
#include "KeyMap.hpp"
#include "usb/hid/ConsumerControlReport.hpp"
#include "usb/hid/KeyboardReporter.hpp"

namespace quartz::kb
{
    LEDState KeyboardState::CurrentLEDState = {};
    utils::BitSet<MatrixDefinitions::Size> KeyboardState::CurrentKeyStates = {};
    utils::BitSet<MatrixDefinitions::Size> KeyboardState::LastKeyStates = {};
    static constexpr std::size_t MatrixBytes = (MatrixDefinitions::Size + 7U) / 8U;
    static utils::BitSet<MatrixDefinitions::Size> History[5]{};
    static std::uint8_t HistoryIndex = 0;

    void KeyboardState::debounce(const utils::BitSet<MatrixDefinitions::Size>& rawStates) noexcept
    {
        rawStates.cloneInto(History[HistoryIndex]);
        HistoryIndex = (HistoryIndex + 1) % 4;

        auto* current = CurrentKeyStates.data.data();
        const auto* h0 = History[0].data.data();
        const auto* h1 = History[1].data.data();
        const auto* h2 = History[2].data.data();
        const auto* h3 = History[3].data.data();

        for (std::size_t i = 0; i < MatrixBytes; ++i)
        {
            const std::uint8_t pressed = h0[i] & h1[i] & h2[i] & h3[i];
            const std::uint8_t released = static_cast<std::uint8_t>(~(h0[i] | h1[i] | h2[i] | h3[i]));
            current[i] = static_cast<std::uint8_t>((current[i] | pressed) & ~released);
        }
    }

    void KeyboardState::updateKeyStates(const utils::BitSet<MatrixDefinitions::Size>& rawStates) noexcept
    {
        CurrentKeyStates.cloneInto(LastKeyStates);
        debounce(rawStates);
    }

    bool KeyboardState::anyKeyChanged() noexcept
    {
        return !CurrentKeyStates.equals(LastKeyStates);
    }

    bool KeyboardState::isKeyDown(const usb::hid::KeyboardUsage usage)
    {
        const auto position = KeyMap::getMatrixPosition(usage);
        if (!position.isValid())
            return false;

        const auto index = Matrix::getKeyIndex(position.Row, position.Col);
        return CurrentKeyStates.test(index);
    }

    bool KeyboardState::isFunctionPressed() noexcept
    {
        const auto index = Matrix::getKeyIndex(kb::FnRow, kb::FnCol);
        return CurrentKeyStates.test(index);
    }

    KeyLayer KeyboardState::resolveLayer() noexcept
    {
        for (std::size_t index = 0; index < MatrixDefinitions::Size; ++index)
        {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::getAction(KeyLayer::Base, row, col);
            if (action.Type == KeyActionType::Layer)
                return action.Layer;
        }

        return KeyLayer::Base;
    }

    usb::hid::BootKeyboardReport KeyboardState::buildBootReport() noexcept
    {
        usb::hid::BootKeyboardReport report = {};
        const auto layer = resolveLayer();
        for (std::size_t index = 0; index < MatrixDefinitions::Size; ++index)
        {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::getAction(layer, row, col);
            if (action.Type == KeyActionType::HID)
                report.add(action.Usage);
        }

        return report;
    }

    usb::hid::NKROKeyboardReport KeyboardState::buildNKROReport() noexcept
    {
        usb::hid::NKROKeyboardReport report;
        const auto layer = resolveLayer();
        for (std::size_t index = 0; index < MatrixDefinitions::Size; ++index)
        {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::getAction(layer, row, col);
            if (action.Type == KeyActionType::HID)
                report.add(action.Usage);
        }

        return report;
    }

    usb::hid::ConsumerControlReport KeyboardState::buildConsumerReport() noexcept
    {
        usb::hid::ConsumerControlReport report;
        const auto layer = resolveLayer();

        for (std::size_t index = 0; index < MatrixDefinitions::Size; ++index)
        {
            if (!CurrentKeyStates.test(index))
                continue;

            const auto [row, col] = Matrix::getKeyPosition(index);
            const auto action = KeyMap::getAction(layer, row, col);
            if (action.Type == KeyActionType::Consumer)
            {
                report.Usage = action.ConsumerUsage;
                break;
            }
        }

        return report;
    }
}
