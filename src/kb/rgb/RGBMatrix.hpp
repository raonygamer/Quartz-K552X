#pragma once
#include "quartz/utils/Color32.hpp"
#include <cstddef>
#include <cstdint>

namespace quartz::kb::rgb
{
    class RGBMatrix
    {
    public:
        static constexpr std::size_t Rows = 7;
        static constexpr std::size_t Columns = 16;

        static void initialize() noexcept;

        static void clear() noexcept;
        static void fill(utils::Color32 color) noexcept;
        static void fill(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept;

        static void setPixel(std::uint8_t row, std::uint8_t column, utils::Color32 color) noexcept;
        static void setPixel(std::uint8_t row, std::uint8_t column, std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept;
        static void setFramebuffer(const utils::Color32* colors, std::size_t count) noexcept;

        static utils::Color32 getPixel(std::uint8_t row, std::uint8_t column) noexcept;

        static void pause() noexcept;
        static void resume() noexcept;
        static void handleInterrupt() noexcept;

        // Blank RGB immediately. Shared selector GPIOs may then be taken by Matrix.
        static void disable() noexcept;

        // Preload MR0..MR21 for CurrentColumn.
        // Does not touch shared selector GPIOs.
        static void preload() noexcept;

        // Claim shared selector GPIOs, select CurrentColumn and start PWM.
        static void enable() noexcept;

        static void advance() noexcept;

        static std::uint8_t currentColumn() noexcept
        {
            return CurrentColumn;
        }

        static void handOver() noexcept;
        static bool handedOver() noexcept;
        static void acquire() noexcept;

    private:
        static void configureSelectorPinsOutput() noexcept;
        static void deselectAllColumns() noexcept;
        static void selectColumn(std::uint8_t column) noexcept;
        static void startCurrentColumn() noexcept;
        static inline bool Running = false;
        inline static utils::Color32 Framebuffer[Rows][Columns]{};
        inline static std::uint8_t CurrentColumn = 0;
    };
}
