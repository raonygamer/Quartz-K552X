#pragma once
#include "kb/MatrixDefinitions.hpp"
#include "quartz/rpc/payloads/RowTimingProbePayload.hpp"
#include <cstdint>

namespace quartz::kb
{
    using SizedMatrixTimingProbeResult = rpc::payloads::MatrixTimingProbeResult<MatrixDefinitions::Rows - 1u>;
    class MatrixTimingProbe
    {
    public:
        static constexpr std::uint8_t NoColumn = 0xFFu;

        static SizedMatrixTimingProbeResult run(std::uint32_t captureMs = 3000u, std::uint32_t durationMs = 4000u) noexcept;

    private:
        struct ColumnProbe
        {
            volatile std::uint32_t* Data = nullptr;
            std::uint32_t Mask = 0;
        };

        static constexpr std::uint8_t StartingRow = 1u;
        static constexpr std::uint32_t RawTickMask = 0x00FFFFFFu;

        static void _waitRaw(std::uint32_t ticks) noexcept;
        static std::uint16_t _scanRow(std::uint8_t row, std::uint32_t settleTicks) noexcept;
        static bool _anyKeyDown(std::uint32_t settleTicks) noexcept;
        static std::uint8_t _firstPressedColumn(std::uint16_t states) noexcept;
        static ColumnProbe _columnProbe(std::uint8_t column) noexcept;
        static void _measureRow(std::uint8_t row, rpc::payloads::MatrixTimingProbeRowResult& result, std::uint32_t timeoutTicks) noexcept;
    };
}