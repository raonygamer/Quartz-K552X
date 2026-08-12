#pragma once
#include "kb/KeyMap.hpp"
#include "kb/MatrixDefinitions.hpp"
#include "quartz/utils/Color32.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace quartz::kb::rgb
{
    class RGBMatrix
    {
    public:
        template <std::size_t R, std::size_t C>
        using Color32Matrix = utils::Color32[R][C];
        template <std::size_t N>
        using FlatColor32Matrix = utils::Color32[N];

        static constexpr std::size_t Rows = MatrixDefinitions::Rows;
        static constexpr std::size_t Columns = MatrixDefinitions::Cols;
        using SizedColor32Matrix = Color32Matrix<Rows, Columns>;
        using SizedFlatColor32Matrix = FlatColor32Matrix<MatrixDefinitions::Size>;

        static void initialize() noexcept;

        static void clear() noexcept;
        static void fill(utils::Color32 color) noexcept;
        static void fill(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept;

        static void setPixel(std::uint8_t row, std::uint8_t column, utils::Color32 color) noexcept;
        static void setPixel(std::uint8_t row, std::uint8_t column, std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept;
        static bool setFramebuffer(const SizedFlatColor32Matrix colors) noexcept;
        static bool swapBuffers() noexcept;

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
        inline static volatile bool SwapPending = false;
        inline static SizedColor32Matrix* ReadingBuffer = nullptr;
        inline static SizedColor32Matrix* WritingBuffer = nullptr;
        inline static SizedColor32Matrix FrontBuffer = {};
        inline static SizedColor32Matrix BackBuffer = {};
        inline static std::uint8_t CurrentColumn = 0;
    };
}
