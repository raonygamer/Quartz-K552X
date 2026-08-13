#pragma once
#include "kb/MatrixDefinitions.hpp"
#include "usb/hid/KeyboardUsage.hpp"
#include "utils/MatrixPosition.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace quartz::kb
{
    enum class KeyLayer : std::uint8_t
    {
        Base,
        Fn,
        Count
    };

    enum class KeyActionType : std::uint8_t
    {
        None,
        Transparent,
        HID,
        Consumer,
        Layer
    };

    struct KeyAction
    {
        KeyActionType Type = { KeyActionType::None };
        usb::hid::KeyboardUsage Usage = { usb::hid::KeyboardUsage::None };
        usb::hid::ConsumerUsage ConsumerUsage = { usb::hid::ConsumerUsage::None };
        KeyLayer Layer = { KeyLayer::Base };

        static constexpr KeyAction none() noexcept
        {
            return {};
        }

        static constexpr KeyAction transparent() noexcept
        {
            return { .Type = KeyActionType::Transparent };
        }

        static constexpr KeyAction hid(const usb::hid::KeyboardUsage usage) noexcept
        {
            return { .Type = KeyActionType::HID, .Usage = usage };
        }

        static constexpr KeyAction consumer(const usb::hid::ConsumerUsage usage) noexcept
        {
            return { .Type = KeyActionType::Consumer, .ConsumerUsage = usage };
        }

        static constexpr KeyAction layer(const KeyLayer layer) noexcept
        {
            return { .Type = KeyActionType::Layer, .Usage = usb::hid::KeyboardUsage::None, .Layer = layer };
        }
    };

    struct KeyMap
    {
        using U = usb::hid::KeyboardUsage;
        static constexpr std::size_t LayerCount = static_cast<std::size_t>(KeyLayer::Count);
        static_assert(MatrixDefinitions::Rows == 7);

        // The PCB rows are electrically ordered 1, 2, 3, 4, 5, 6, but physically read much more naturally as 1, 6, 5, 4, 3, 2.
        // This mapping is self-inverse, so it works in both directions.
        static constexpr std::array<std::uint8_t, MatrixDefinitions::Rows> RowMap{ 0, 1, 6, 5, 4, 3, 2 };
        static constexpr std::uint8_t toElectricalRow(const std::uint8_t virtualRow) noexcept
        {
            return RowMap[virtualRow];
        }

        static constexpr std::uint8_t toVirtualRow(const std::uint8_t electricalRow) noexcept
        {
            return RowMap[electricalRow];
        }

#define XX KeyAction::none()
#define TR KeyAction::transparent()
#define K(x) KeyAction::hid(U::x)
#define L(x) KeyAction::layer(KeyLayer::x)
#define C(x) KeyAction::consumer(usb::hid::ConsumerUsage::x)

        // clang-format off
        static constexpr KeyAction Layers[LayerCount][MatrixDefinitions::Rows][MatrixDefinitions::Cols] = {
            {
                { XX            , XX               , XX        , XX       , XX       , XX       , XX       , XX       , XX       , XX         , XX          , XX            , XX               , XX            , XX           , XX            }, // 0 - unused
                { K(Escape)     , K(F1)            , K(F2)     , K(F3)    , K(F4)    , K(F5)    , K(F6)    , K(F7)    , K(F8)    , K(F9)      , K(F10)      , K(F11)        , K(F12)           , K(PrintScreen), K(ScrollLock), K(Pause)      }, // 1 - function row
                { K(GraveAccent), K(Digit1)        , K(Digit2) , K(Digit3), K(Digit4), K(Digit5), K(Digit6), K(Digit7), K(Digit8), K(Digit9)  , K(Digit0)   , K(Minus)      , K(Equal)         , K(Insert)     , K(Home)      , K(PageUp)     }, // 2 - number row
                { K(Tab)        , K(Q)             , K(W)      , K(E)     , K(R)     , K(T)     , K(Y)     , K(U)     , K(I)     , K(O)       , K(P)        , K(LeftBracket), K(RightBracket)  , K(Delete)     , K(End)       , K(PageDown)   }, // 3 - QWERTY row
                { K(CapsLock)   , K(A)             , K(S)      , K(D)     , K(F)     , K(G)     , K(H)     , K(J)     , K(K)     , K(L)       , K(Semicolon), K(Apostrophe) , K(NonUSHash)     , K(Backspace)  , XX           , K(Enter)      }, // 4 - home row
                { K(LeftShift)  , K(NonUSBackslash), K(Z)      , K(X)     , K(C)     , K(V)     , K(B)     , K(N)     , K(M)     , K(Comma)   , K(Period)   , K(Slash)      , K(International1), XX            , K(UpArrow)   , K(RightShift) }, // 5 - bottom alpha row
                { K(LeftControl), K(LeftGUI)       , K(LeftAlt), XX       , XX       , XX       , K(Space) , XX       , XX       , K(RightAlt), L(Fn)       , K(Application), K(RightControl)  , K(LeftArrow)  , K(DownArrow) , K(RightArrow) }, // 6 - modifiers / space / arrows
            },
            {
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 0
                {
                    TR,
                    C(AudioPlayer),         // F1
                    C(VolumeDecrement),     // F2
                    C(VolumeIncrement),     // F3
                    C(Mute),                // F4
                    C(Stop),                // F5
                    C(ScanPreviousTrack),   // F6
                    C(PlayPause),           // F7
                    C(ScanNextTrack),       // F8
                    C(EmailReader),         // F9
                    C(Home),                // F10
                    C(Calculator),          // F11
                    C(Search),              // F12
                    TR,
                    TR,
                    TR
                }, // 1
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 2
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 3
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 4
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 5
                { TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR, TR }, // 6
            },
        };
        // clang-format on

#undef XX
#undef TR
#undef K
#undef L
#undef C

        static constexpr auto UsageToMatrixPosition = []
        {
            std::array<utils::MatrixPosition, 256> positions{};
            for (auto& position : positions)
                position = { -1, -1 };

            for (std::size_t virtualRow = 0; virtualRow < MatrixDefinitions::Rows; ++virtualRow)
            {
                for (std::size_t col = 0; col < MatrixDefinitions::Cols; ++col)
                {
                    const auto& action = Layers[static_cast<std::size_t>(KeyLayer::Base)][virtualRow][col];
                    if (action.Type != KeyActionType::HID)
                        continue;

                    positions[static_cast<std::uint8_t>(action.Usage)] = {
                        static_cast<int>(RowMap[virtualRow]),
                        static_cast<int>(col)
                    };
                }
            }

            return positions;
        }();

        static constexpr KeyAction getAction(const KeyLayer layer, const std::size_t electricalRow, const std::size_t col) noexcept
        {
            const auto virtualRow = toVirtualRow(static_cast<std::uint8_t>(electricalRow));
            const auto action = Layers[static_cast<std::size_t>(layer)][virtualRow][col];
            return action.Type == KeyActionType::Transparent ? Layers[static_cast<std::size_t>(KeyLayer::Base)][virtualRow][col] : action;
        }

        static constexpr utils::MatrixPosition getMatrixPosition(const U usage) noexcept
        {
            const auto index = static_cast<std::uint16_t>(usage);
            return index < UsageToMatrixPosition.size() ? UsageToMatrixPosition[index] : utils::MatrixPosition{ -1, -1 };
        }
    };
}
