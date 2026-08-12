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
        static constexpr std::uint32_t PWMOutputMask = 0x003F7FFFu;
        static constexpr std::uint32_t SelectorMaskPortC = 0x1FFBu; // P2.0, P2.1, P2.3-P2.12
        static constexpr std::uint32_t SelectorMaskPortB = 0x03C0u; // P1.6-P1.9
        static constexpr std::uint32_t PWMPrescaler = 10u; // 17.04kHz
        static constexpr std::uint32_t PWMPeriod = 255u;

        template <std::size_t R, std::size_t C>
        using Color32Matrix = utils::Color32[R][C];
        template <std::size_t N>
        using FlatColor32Matrix = utils::Color32[N];
        using SizedColor32Matrix = Color32Matrix<MatrixDefinitions::Rows, MatrixDefinitions::Cols>;
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
        static void resume() noexcept;
        static void preload() noexcept;
        static void advance() noexcept;
        static std::uint8_t currentColumn() noexcept;
        static void handOver() noexcept;
        static bool handedOver() noexcept;
        static void acquire() noexcept;
        static void _handleInterrupt() noexcept;

    private:
        static void _configureSelectorPinsOutput() noexcept;
        static void _deselectAllColumns() noexcept;
        static void _selectColumn(std::uint8_t column) noexcept;
        static void _startCurrentColumn() noexcept;
        static inline bool Running = false;
        inline static volatile bool SwapPending = false;
        inline static SizedColor32Matrix* ReadingBuffer = nullptr;
        inline static SizedColor32Matrix* WritingBuffer = nullptr;
        inline static SizedColor32Matrix FrontBuffer = {};
        inline static SizedColor32Matrix BackBuffer = {};
        inline static std::uint8_t CurrentColumn = 0;
        inline static volatile std::uint32_t SlotStartedAt = 0u;
    };
}
