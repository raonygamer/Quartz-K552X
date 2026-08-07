#pragma once

#include <cstddef>
#include <cstdint>
#include "usb/hid/KeyboardUsage.hpp"

namespace quartz::kb {
    enum class KeyActionType : std::uint8_t {
        None,
        HID,
        Fn,
    };

    struct KeyAction {
        KeyActionType Type { KeyActionType::None };
        usb::hid::KeyboardUsage Usage {
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

    struct KeyMap {
        static constexpr std::size_t Rows = 7;
        static constexpr std::size_t Cols = 16;

        using K = usb::hid::KeyboardUsage;

        inline static constexpr KeyAction Actions[Rows][Cols] = {
            // Row 0
            {
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
                KeyAction::none(), KeyAction::none(),
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
                KeyAction::hid(K::LeftShift),        // 0
                KeyAction::hid(K::NonUSBackslash),   // 1  \ |
                KeyAction::hid(K::Z),                // 2
                KeyAction::hid(K::X),                // 3
                KeyAction::hid(K::C),                // 4
                KeyAction::hid(K::V),                // 5
                KeyAction::hid(K::B),                // 6
                KeyAction::hid(K::N),                // 7
                KeyAction::hid(K::M),                // 8
                KeyAction::hid(K::Comma),            // 9
                KeyAction::hid(K::Period),           // 10
                KeyAction::hid(K::Slash),            // 11 ; :
                KeyAction::hid(K::International1),   // 12 / ?
                KeyAction::hid(K::RightShift),       // 13
                KeyAction::hid(K::UpArrow),          // 14
                KeyAction::none(),                   // 15
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