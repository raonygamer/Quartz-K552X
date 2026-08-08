#include "usb/hid/KeyboardUsage.hpp"
#include "MatrixDefinitions.hpp"

namespace quartz::kb {
    using Key = usb::hid::KeyboardUsage;
    inline static constexpr Key UsageMap[MatrixDefinitions::Rows][MatrixDefinitions::Cols] = {
        // Row 0 - currently unused
        {
            Key::None, Key::None, Key::None, Key::None,
            Key::None, Key::None, Key::None, Key::None,
            Key::None, Key::None, Key::None, Key::None,
            Key::None, Key::None, Key::None, Key::None,
        },

        // Row 1
        {
            Key::Escape,      // 1:0
            Key::F1,          // 1:1
            Key::F2,          // 1:2
            Key::F3,          // 1:3
            Key::F4,          // 1:4
            Key::F5,          // 1:5
            Key::F6,          // 1:6
            Key::F7,          // 1:7
            Key::F8,          // 1:8
            Key::F9,          // 1:9
            Key::F10,         // 1:10
            Key::F11,         // 1:11
            Key::F12,         // 1:12
            Key::PrintScreen, // 1:13
            Key::ScrollLock,  // 1:14
            Key::Pause,       // 1:15
        },

        // Row 2
        {
            Key::LeftControl,  // 2:0
            Key::LeftGUI,      // 2:1
            Key::LeftAlt,      // 2:2
            Key::None,         // 2:3
            Key::None,         // 2:4
            Key::None,         // 2:5
            Key::Space,        // 2:6
            Key::None,         // 2:7
            Key::None,         // 2:8
            Key::RightAlt,     // 2:9
            Key::None,         // 2:10 Fn -- handled internally
            Key::Application,  // 2:11
            Key::RightControl, // 2:12
            Key::LeftArrow,    // 2:13
            Key::DownArrow,    // 2:14
            Key::RightArrow,   // 2:15
        },

        // Row 3
        {
            Key::LeftShift,       // 3:0
            Key::NonUSBackslash,  // 3:1  \ |
            Key::Z,               // 3:2
            Key::X,               // 3:3
            Key::C,               // 3:4
            Key::V,               // 3:5
            Key::B,               // 3:6
            Key::N,               // 3:7
            Key::M,               // 3:8
            Key::Comma,           // 3:9  , <
            Key::Period,          // 3:10 . >
            Key::Slash,           // 3:11 ; : on ABNT2
            Key::International1,  // 3:12 / ?
            Key::RightShift,      // 3:13
            Key::UpArrow,         // 3:14
            Key::None,            // 3:15
        },

        // Row 4
        {
            Key::CapsLock,     // 4:0
            Key::A,            // 4:1
            Key::S,            // 4:2
            Key::D,            // 4:3
            Key::F,            // 4:4
            Key::G,            // 4:5
            Key::H,            // 4:6
            Key::J,            // 4:7
            Key::K,            // 4:8
            Key::L,            // 4:9
            Key::Semicolon,    // 4:10 Ç
            Key::Apostrophe,   // 4:11 ~ ^
            Key::NonUSHash,    // 4:12 ] }
            Key::Backspace,    // 4:13
            Key::None,         // 4:14
            Key::Enter,        // 4:15
        },

        // Row 5
        {
            Key::Tab,          // 5:0
            Key::Q,            // 5:1
            Key::W,            // 5:2
            Key::E,            // 5:3
            Key::R,            // 5:4
            Key::T,            // 5:5
            Key::Y,            // 5:6
            Key::U,            // 5:7
            Key::I,            // 5:8
            Key::O,            // 5:9
            Key::P,            // 5:10
            Key::LeftBracket,  // 5:11 ´ `
            Key::RightBracket, // 5:12 [ {
            Key::Delete,       // 5:13
            Key::End,          // 5:14
            Key::PageDown,     // 5:15
        },

        // Row 6
        {
            Key::GraveAccent, // 6:0 ' "
            Key::Digit1,      // 6:1
            Key::Digit2,      // 6:2
            Key::Digit3,      // 6:3
            Key::Digit4,      // 6:4
            Key::Digit5,      // 6:5
            Key::Digit6,      // 6:6
            Key::Digit7,      // 6:7
            Key::Digit8,      // 6:8
            Key::Digit9,      // 6:9
            Key::Digit0,      // 6:10
            Key::Minus,       // 6:11 - _
            Key::Equal,       // 6:12 = +
            Key::Insert,      // 6:13
            Key::Home,        // 6:14
            Key::PageUp,      // 6:15
        },
    };

    inline static constexpr std::uint8_t FnRow = 2;
    inline static constexpr std::uint8_t FnCol = 10;
}