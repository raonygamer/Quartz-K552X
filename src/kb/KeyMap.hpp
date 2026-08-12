#pragma once
#include "usb/hid/KeyboardUsage.hpp"
#include "utils/MatrixPosition.hpp"
#include <cstddef>
#include <cstdint>
#include <array>

namespace quartz::kb
{
    enum class KeyActionType : std::uint8_t
    {
        None,
        HID,
        Fn,
    };

    struct KeyAction
    {
        KeyActionType Type{ KeyActionType::None };
        usb::hid::KeyboardUsage Usage{
            usb::hid::KeyboardUsage::None
        };

        static constexpr KeyAction none() noexcept
        {
            return {};
        }

        static constexpr KeyAction hid(
            const usb::hid::KeyboardUsage usage
        ) noexcept
        {
            return {
                KeyActionType::HID,
                usage,
            };
        }

        static constexpr KeyAction fn() noexcept
        {
            return {
                KeyActionType::Fn,
                usb::hid::KeyboardUsage::None,
            };
        }
    };

    struct KeyMap
    {
        static constexpr std::size_t Rows = 7;
        static constexpr std::size_t Cols = 16;

        using K = usb::hid::KeyboardUsage;

        inline static constexpr auto UsageToMatrixPosition = []
        {
            std::array<utils::MatrixPosition, 256> positions{};

            for (auto& position : positions)
            {
                position = { -1, -1 };
            }

            const auto set = [&positions](K usage, int row, int col) constexpr
            {
                positions[static_cast<std::uint8_t>(usage)] = { row, col };
            };

            // Row 1 - function row
            set(K::Escape, 1, 0);
            set(K::F1, 1, 1);
            set(K::F2, 1, 2);
            set(K::F3, 1, 3);
            set(K::F4, 1, 4);
            set(K::F5, 1, 5);
            set(K::F6, 1, 6);
            set(K::F7, 1, 7);
            set(K::F8, 1, 8);
            set(K::F9, 1, 9);
            set(K::F10, 1, 10);
            set(K::F11, 1, 11);
            set(K::F12, 1, 12);
            set(K::PrintScreen, 1, 13);
            set(K::ScrollLock, 1, 14);
            set(K::Pause, 1, 15);

            // Row 2 - modifiers / space / arrows
            set(K::LeftControl, 2, 0);
            set(K::LeftGUI, 2, 1);
            set(K::LeftAlt, 2, 2);
            set(K::Space, 2, 6);
            set(K::RightAlt, 2, 9);
            set(K::Application, 2, 11);
            set(K::RightControl, 2, 12);
            set(K::LeftArrow, 2, 13);
            set(K::DownArrow, 2, 14);
            set(K::RightArrow, 2, 15);

            // Row 3 - bottom alpha row
            set(K::LeftShift, 3, 0);
            set(K::NonUSBackslash, 3, 1);
            set(K::Z, 3, 2);
            set(K::X, 3, 3);
            set(K::C, 3, 4);
            set(K::V, 3, 5);
            set(K::B, 3, 6);
            set(K::N, 3, 7);
            set(K::M, 3, 8);
            set(K::Comma, 3, 9);
            set(K::Period, 3, 10);
            set(K::Slash, 3, 11);
            set(K::International1, 3, 12);
            set(K::UpArrow, 3, 14);
            set(K::RightShift, 3, 15);

            // Row 4 - home row
            set(K::CapsLock, 4, 0);
            set(K::A, 4, 1);
            set(K::S, 4, 2);
            set(K::D, 4, 3);
            set(K::F, 4, 4);
            set(K::G, 4, 5);
            set(K::H, 4, 6);
            set(K::J, 4, 7);
            set(K::K, 4, 8);
            set(K::L, 4, 9);
            set(K::Semicolon, 4, 10);
            set(K::Apostrophe, 4, 11);
            set(K::NonUSHash, 4, 12);
            set(K::Backspace, 4, 13);
            set(K::Enter, 4, 15);

            // Row 5 - QWERTY row
            set(K::Tab, 5, 0);
            set(K::Q, 5, 1);
            set(K::W, 5, 2);
            set(K::E, 5, 3);
            set(K::R, 5, 4);
            set(K::T, 5, 5);
            set(K::Y, 5, 6);
            set(K::U, 5, 7);
            set(K::I, 5, 8);
            set(K::O, 5, 9);
            set(K::P, 5, 10);
            set(K::LeftBracket, 5, 11);
            set(K::RightBracket, 5, 12);
            set(K::Delete, 5, 13);
            set(K::End, 5, 14);
            set(K::PageDown, 5, 15);

            // Row 6 - number row
            set(K::GraveAccent, 6, 0);
            set(K::Digit1, 6, 1);
            set(K::Digit2, 6, 2);
            set(K::Digit3, 6, 3);
            set(K::Digit4, 6, 4);
            set(K::Digit5, 6, 5);
            set(K::Digit6, 6, 6);
            set(K::Digit7, 6, 7);
            set(K::Digit8, 6, 8);
            set(K::Digit9, 6, 9);
            set(K::Digit0, 6, 10);
            set(K::Minus, 6, 11);
            set(K::Equal, 6, 12);
            set(K::Insert, 6, 13);
            set(K::Home, 6, 14);
            set(K::PageUp, 6, 15);

            return positions;
        }();

        [[nodiscard]] static constexpr utils::MatrixPosition getMatrixPosition(K usage) noexcept
        {
            const auto index = static_cast<std::uint16_t>(usage);
            return index < UsageToMatrixPosition.size() ? UsageToMatrixPosition[index] : utils::MatrixPosition{ -1, -1 };
        }

        inline static constexpr KeyAction Actions[Rows][Cols] = {
            // Row 0
            {
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
                KeyAction::none(),
            },

            // Row 1 - function row
            {
                KeyAction::hid(K::Escape),
                KeyAction::hid(K::F1),
                KeyAction::hid(K::F2),
                KeyAction::hid(K::F3),
                KeyAction::hid(K::F4),
                KeyAction::hid(K::F5),
                KeyAction::hid(K::F6),
                KeyAction::hid(K::F7),
                KeyAction::hid(K::F8),
                KeyAction::hid(K::F9),
                KeyAction::hid(K::F10),
                KeyAction::hid(K::F11),
                KeyAction::hid(K::F12),
                KeyAction::hid(K::PrintScreen),
                KeyAction::hid(K::ScrollLock),
                KeyAction::hid(K::Pause),
            },

            // Row 2 - modifiers / space / arrows
            {
                KeyAction::hid(K::LeftControl),  // 0
                KeyAction::hid(K::LeftGUI),      // 1
                KeyAction::hid(K::LeftAlt),      // 2
                KeyAction::none(),               // 3
                KeyAction::none(),               // 4
                KeyAction::none(),               // 5
                KeyAction::hid(K::Space),        // 6
                KeyAction::none(),               // 7
                KeyAction::none(),               // 8
                KeyAction::hid(K::RightAlt),     // 9
                KeyAction::fn(),                 // 10
                KeyAction::hid(K::Application),  // 11
                KeyAction::hid(K::RightControl), // 12
                KeyAction::hid(K::LeftArrow),    // 13
                KeyAction::hid(K::DownArrow),    // 14
                KeyAction::hid(K::RightArrow),   // 15
            },

            // Row 3 - bottom alpha row
            {
                KeyAction::hid(K::LeftShift),      // 0
                KeyAction::hid(K::NonUSBackslash), // 1  \ |
                KeyAction::hid(K::Z),              // 2
                KeyAction::hid(K::X),              // 3
                KeyAction::hid(K::C),              // 4
                KeyAction::hid(K::V),              // 5
                KeyAction::hid(K::B),              // 6
                KeyAction::hid(K::N),              // 7
                KeyAction::hid(K::M),              // 8
                KeyAction::hid(K::Comma),          // 9
                KeyAction::hid(K::Period),         // 10
                KeyAction::hid(K::Slash),          // 11 ; :
                KeyAction::hid(K::International1), // 12 / ?
                KeyAction::none(),                 // 13
                KeyAction::hid(K::UpArrow),        // 14
                KeyAction::hid(K::RightShift),     // 15
            },

            // Row 4 - home row
            {
                KeyAction::hid(K::CapsLock),
                KeyAction::hid(K::A),
                KeyAction::hid(K::S),
                KeyAction::hid(K::D),
                KeyAction::hid(K::F),
                KeyAction::hid(K::G),
                KeyAction::hid(K::H),
                KeyAction::hid(K::J),
                KeyAction::hid(K::K),
                KeyAction::hid(K::L),
                KeyAction::hid(K::Semicolon),  // Ç
                KeyAction::hid(K::Apostrophe), // ~ ^
                KeyAction::hid(K::NonUSHash),  // ] }
                KeyAction::hid(K::Backspace),
                KeyAction::none(),
                KeyAction::hid(K::Enter),
            },

            // Row 5 - QWERTY row
            {
                KeyAction::hid(K::Tab),
                KeyAction::hid(K::Q),
                KeyAction::hid(K::W),
                KeyAction::hid(K::E),
                KeyAction::hid(K::R),
                KeyAction::hid(K::T),
                KeyAction::hid(K::Y),
                KeyAction::hid(K::U),
                KeyAction::hid(K::I),
                KeyAction::hid(K::O),
                KeyAction::hid(K::P),
                KeyAction::hid(K::LeftBracket),  // ´ `
                KeyAction::hid(K::RightBracket), // [ {
                KeyAction::hid(K::Delete),
                KeyAction::hid(K::End),
                KeyAction::hid(K::PageDown),
            },

            // Row 6 - number row
            {
                KeyAction::hid(K::GraveAccent), // ' "
                KeyAction::hid(K::Digit1),
                KeyAction::hid(K::Digit2),
                KeyAction::hid(K::Digit3),
                KeyAction::hid(K::Digit4),
                KeyAction::hid(K::Digit5),
                KeyAction::hid(K::Digit6),
                KeyAction::hid(K::Digit7),
                KeyAction::hid(K::Digit8),
                KeyAction::hid(K::Digit9),
                KeyAction::hid(K::Digit0),
                KeyAction::hid(K::Minus),
                KeyAction::hid(K::Equal),
                KeyAction::hid(K::Insert),
                KeyAction::hid(K::Home),
                KeyAction::hid(K::PageUp),
            },
        };
    };
}